//
// Created by safoex on 05.09.19.
//

#ifndef ABTM2_ABTM_H
#define ABTM2_ABTM_H

#include "ExecutorInterface.h"
#include "core/common.h"
#include "core/nodes/Nodes.h"
#include <queue>
#include <atomic>
#include "core/nodes/Leaf.h"

#define DROP_TICKS_ON_PAUSE
#undef DROP_TICKS_ON_PAUSE

namespace abtm {

    struct ABTM_NodeInfo{
        std::vector<int> order;
        NodeInterface* parent;
        explicit ABTM_NodeInfo(std::vector<int>&& order = {0}, NodeInterface* parent = nullptr): order(order), parent(parent){};
        explicit ABTM_NodeInfo(NodeInterface* me, NodeInterface* parent = nullptr) : parent(parent) {
            if(parent == nullptr) {
                order = {0};
            }
            else {
                reset_order(me);
            }
        }
        void reset_order(NodeInterface* me) {
            int i = 0;
            if(parent != nullptr) {
                order = std::any_cast<ABTM_NodeInfo *>(parent->info)->order;
                for(auto c: parent->children) {
                    if(c == me) {
                        order.push_back(i);
                        break;
                    }
                    i++;
                }
            }
            else
                order = std::vector<int>{0};
        }
    };

    struct ABTM_NodeCompare {
        bool operator()(NodeInterface* left, NodeInterface* right) {
            // Kleene-Brower order
            auto const& left_order = std::any_cast<ABTM_NodeInfo*>(left->info)->order;
            auto const& right_order = std::any_cast<ABTM_NodeInfo*>(right->info)->order;
            for(size_t i = 0; i < std::min(left_order.size(), right_order.size()); i++) {
                if( left_order[i] != right_order[i] )
                    return left_order[i] < right_order[i]; // left, top..
            }
            return left_order.size() > right_order.size(); // direct child first!!!
        }
    };

    class ABTM : public ExecutorInterface {
    public:
        enum ExecutionType {
            SyncTopDown,
            AsyncTopDown,
            AsyncBottomUp,
            Async = AsyncBottomUp,
            Classic = SyncTopDown
        };
    protected:
        std::queue<sample> stash;
        std::priority_queue<NodeInterface*, std::vector<NodeInterface*>, ABTM_NodeCompare> tick_queue;
        dictOf <std::unordered_set<NodeInterface*>> links;
        ExecutionType exec_type;
        sample output_stash;
        trace exec_trace;
        std::mutex mutex;
    public:
        dictOf <NodeInterface*> nodes;
        ControlInterface* root;
        explicit ABTM(ExecutionType executionType, MemoryInterface* memory = nullptr) : ExecutorInterface(memory),
        exec_type(executionType), root(nullptr) {}

        sample filter_keywords_except_for_tick(sample const& s) {
            sample res;
            for(auto const& [k,v] : s) {
                if(!is_keyword(k) || k == TICK_WORD) {
                    res[k] = v;
                }
            }
            return res;
        }

        sample execute(sample const& s) override {
            std::lock_guard lockGuard(mutex);

            sample result;
            auto [type, com, _data, ticket] = unpack(s);

            if(type == Execute) {
                if(state == NOT_STARTED) {
                    // start tree
                    if(ExecutorInterface::Execution(com) == START) {
                        state = ExecutorInterface::Execution(com);
                        if(exec_type == Async) {
                            return exec(TICK_SAMPLE);
                        }
                    }
                }
                else state = ExecutorInterface::Execution(com);
            }
            else if(type == Modify) {
                auto data = std::any_cast<dictOf <std::string>>(_data);
                return modify(Modification(com), data, ticket);
            }
            else {
                if (state == START || state == PAUSE) {
#ifdef DROP_TICKS_ON_PAUSE
                    if(!(state == PAUSE && s.count(TICK_WORD)))
#endif
                    stash.push(filter_keywords_except_for_tick(s));

                }
                if (state == START) {
                    while (!stash.empty()) {
                        auto tmp = exec(stash.front());
                        for (auto const&[k, v]: tmp)
                            result[k] = v;
                        stash.pop();
                    }
                }
                if (state == STOP) {

                }
            }
            return result;
        };
    protected:

        sample exec(sample const& s) {
            switch (exec_type) {
                case AsyncBottomUp: return callback_async_bottomup(s);
                case AsyncTopDown:  return callback_async_topdown(s);
                case SyncTopDown:   return callback_sync_topdown(s);
            }
        }

        bool propogate_once_bottomup() {

            // 0: get changed variables
            auto s = memory->changes();
            for(auto const& [k,v]: s)
                output_stash[k] = v;
            exec_trace.push(s);
            memory->flush();

            std::unordered_set <NodeInterface*> possibly_changed_conditions{};

            // 1: collect possibly changed conditions from links to changed variables
            for(auto const& [k,v]: s) {
                possibly_changed_conditions.insert(links[k].begin(), links[k].end());
            }

            // 2: check if state has really changed
            // 3: clear .visited field from these conditions
            sample changed_conditions;
            for(NodeInterface* node: possibly_changed_conditions) {
                if(node->state() != node->evaluate()) {
                    tick_queue.push(node);
                    changed_conditions[node->id()] = std::string(CHANGED_WORD);
                    while (node != nullptr) {
                        node->visited = false;
                        node = std::any_cast<ABTM_NodeInfo*>(node->info)->parent;
                    }
                }
            }
            exec_trace.push(changed_conditions);

            // 4:
            if(!tick_queue.empty()) {
                auto node = tick_queue.top();
                tick_queue.pop();
                if(node)
                    exec_trace.push({{node->id(), TICK_WORD}});
                if(node && node->state() != node->tick()) {
                    tick_queue.push(std::any_cast<ABTM_NodeInfo*>(node->info)->parent);
                }
            }

            // 5: return true if we have or potentially have nodes to tick
            return !tick_queue.empty() || !memory->changes().empty();
        }

        sample callback_async_bottomup(sample const& s) {
            // 0: set input changes       OR      initiate from root
            if(s.count(TICK_WORD)) {
                root->bfs_with_handler([](NodeInterface* n) {n->visited = false;});
                root->tick();
            }
            else memory->set(s);

            // 1: clear old output
            output_stash.clear();
            exec_trace = {};

            // 2: clear .visited for all nodes
            root->bfs_with_handler([](NodeInterface* n) {n->visited = false;});

            // 3: tick nodes while there is something to tick
            while(propogate_once_bottomup());

            // 4: return output
            output_stash[TRACE_WORD] = exec_trace;
            return output_stash;
        }

        sample callback_async_topdown(sample const& s) {
            memory->set(s);
            root->bfs_with_handler([](NodeInterface* n) {n->visited = false;});
            root->tick();
            output_stash = memory->changes();
            memory->flush();
            return output_stash;
        }

        sample callback_sync_topdown(sample const& s) {

            if(s.count(TICK_WORD)){
//                std::cout << from_sample<std::string>(memory->update({{"time", std::string("0")}}))["time"] << std::endl;
//                std::cout << from_sample<std::string>(memory->update({{"_timer_branch_start_time_var", std::string("0")}}))["_timer_branch_start_time_var"] << std::endl;
                root->bfs_with_handler([](NodeInterface* n) {n->visited = false;});
                root->tick();
                output_stash = memory->changes();
                memory->flush();
//                if(!output_stash.empty()) {
//                    for(auto const& [k,v]: output_stash) {
//                        std::cout << k << '\t';
//                    }
//                    std::cout << std::endl;
//                }
                return output_stash;
            }
            else {
                memory->set(s);
                return {};
            }
        }



        sample modify(Modification com, dictOf<std::string> data, std::string const& ticket = "") {
            std::string type = data["type"],
                    name = data["name"],
                    expr = data["expr"],
                    true_state = data["true_state"], false_state = data["false_state"],
                    parent = data["parent"],
                    before = data["before"],
                    instead = data["instead"];
            if(name.empty())
                return make_exception("no name in Modify call", ticket);
            if(com == INSERT) {
                if(parent.empty() && root != nullptr) {
                    return make_exception("insert " + name + ": no parent", ticket);
                }

                auto new_node = create_node(data);
                auto e = get_exception(new_node);
                if(e.empty()) {
                    insert(std::any_cast<NodeInterface*>(new_node[name]), parent, before);
                }
                else return new_node;
            }
            else if(com == REPLACE) {
                if(instead.empty()) {
                    return make_exception("replace with " + name + ": no instead", ticket);
                }
                else if(!nodes.count(instead)) {
                    return make_exception("replace " + instead + ": " + instead + " doesn't exist", ticket);
                }

                auto new_node = create_node(data);
                auto e = get_exception(new_node);
                if(e.empty()) {
                    replace(instead, std::any_cast<NodeInterface*>(new_node[name]));
                }
                else return new_node;
            }
            else if(com == ERASE) {
                if(!nodes.count(name)) {
                    return make_exception("replace " + name + ": " + name + " doesn't exist", ticket);
                }

                erase(name);
            }
            return {{RESPONSE_WORD, OK_WORD}, {TICKET_WORD, ticket}};
        }

        sample create_node(dictOf<std::string> const& data) {
            std::string type = data.at("type"), name = data.at("name"), expr = data.at("expr"),
                    true_state = data.at("true_state"), false_state = data.at("false_state");
            keys leaf_names{"action", "condition"};
            if(leaf_names.count(type)) {
                if(memory->test_expression(expr)) {
                    BFunction node_function = (type == "action" ? memory->build_action(expr) : memory->build_condtion(expr));
                    if(type == "action" && true_state.empty())
                        true_state = "SUCCESS";
                    auto new_node = new Leaf(name, *memory, node_function, from_string(true_state), from_string(false_state), exec_type == Async);
                    if(type == "condition") {
                        link(new_node, memory->used_vars(expr));
                    }
                    nodes[name] = new_node;
                    return {{name, (NodeInterface*)new_node}};
                }
                else {
                    return make_exception("create "+name + ": bad expression \"" + expr + "\"");
                }
            }
            else {
                if(type == "parallel") {
                    auto new_node = new Parallel(SUCCESS, name, *memory, exec_type == Async);
                    nodes[name] = new_node;
                    return {{name, (NodeInterface*)new_node}};
                }
                else if(type == "sequence" || type == "selector" || type == "skipper") {
                    NodeState return_state(RUNNING);
                    if(type == "sequence") return_state = SUCCESS;
                    else if(type == "selector") return_state = FAILURE;
                    else if(type == "skipper") return_state = RUNNING;
                    auto new_node = new Sequential(return_state, name, *memory, exec_type == Async);
                    nodes[name] = new_node;
                    return {{name, (NodeInterface*)new_node}};
                }
                else return make_exception("create "+name + ": unknown node type " + type);
            }
        }

        void refresh_orders() {
            if(root)
                root->bfs_with_handler([](NodeInterface* node) {std::any_cast<ABTM_NodeInfo*>(node->info)->reset_order(node);});
        }

        void link(NodeInterface* node, keys const& vars) {
            for(auto const& v: vars) {
                links[v].insert(node);
            }
        }

        void unlink(NodeInterface* node) {
            for(auto const& [k,v]: links) {
                if(v.count(node))
                    links[k].erase(node);
            }
        }

        void insert(NodeInterface* node, std::string const& parent_name = "", std::string const& before_name = "") {
            if(parent_name.empty() && root == nullptr) {
                root = dynamic_cast<ControlInterface*>(node);
                root->info = new ABTM_NodeInfo();
            }
            else if(nodes.count(parent_name)) {
                auto parent = dynamic_cast<ControlInterface*>(nodes[parent_name]);
                NodeInterface* before = nullptr;
                if(!before_name.empty()) {
                    for(auto n: parent->children) {
                        if(n->id() == before_name)
                            before = n;
                    }
                }
                parent->insert(node, before);
                node->info = new ABTM_NodeInfo(node, parent);
            }
            refresh_orders();
        }

        void delete_node(NodeInterface* n) {
            if(nodes.count(n->id())) {
                unlink(n);
                delete std::any_cast<ABTM_NodeInfo*>(n->info);
                nodes.erase(n->id());
                delete n;
            }
        }

        void delete_nodes_from(NodeInterface* n) {
            if(nodes.count(n->id())) {
                unlink(n);
                delete std::any_cast<ABTM_NodeInfo*>(n->info);
                nodes.erase(n->id());
                for(auto c: n->children) {
                    delete_nodes_from(c);
                }
                delete n;
            }
        }

        void replace(std::string const& from, NodeInterface* to) {
            if(nodes.count(from)) {
                auto nfrom = nodes[from];
                auto parent = std::any_cast<ABTM_NodeInfo*>(nfrom->info)->parent;
                dynamic_cast<ControlInterface*>(parent)->replace(nfrom, to);
                to->info = new ABTM_NodeInfo(to, parent);
                delete_nodes_from(nodes[from]);
                refresh_orders();
            }
        }

        void erase(std::string const& node) {
            if(nodes.count(node)) {
                dynamic_cast<ControlInterface*>(std::any_cast<ABTM_NodeInfo*>(nodes[node]->info)->parent)->erase(nodes[node]);
                delete_nodes_from(nodes[node]);
                refresh_orders();
            }
        }

    };
}

#endif //ABTM2_ABTM_H
