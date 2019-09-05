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

namespace abtm {

    struct ABTM_NodeInfo{
        std::vector<int> order;
        NodeInterface* parent;
        ABTM_NodeInfo(std::vector<int>&& order = {0}, NodeInterface* parent = nullptr): order(order), parent(parent){};
        ABTM_NodeInfo(NodeInterface* me, NodeInterface* parent = nullptr) : parent(parent) {
            if(parent == nullptr) {
                order = {0};
            }
            else {
                reset_order(me);
            }
        }
        void reset_order(NodeInterface* me) {
            int i = 0;
            order = std::any_cast<ABTM_NodeInfo*>(parent->info)->order;
            for(auto c: parent->children) {
                if(c == me) {
                    order.push_back(i);
                    break;
                }
                i++;
            }
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
        enum ExtraCommands {
            LINK
        };
    protected:
        std::queue<sample> stash;
        std::priority_queue<NodeInterface*, std::vector<NodeInterface*>, ABTM_NodeCompare> tick_queue;
        dictOf <NodeInterface*> nodes;
        dictOf <std::unordered_set<NodeInterface*>> links;
        ExecutionType exec_type;
        ControlInterface* root;
        std::mutex mutex;
        Execution state;
    public:
        ABTM(ExecutionType executionType, MemoryInterface* memory = nullptr) : ExecutorInterface(memory),
        exec_type(executionType), root(nullptr), state(STOP) {}
        sample execute(sample const& s) override {
            std::lock_guard lockGuard(mutex);

            sample result;

            if(state == START || state == PAUSE) {
                stash.push(s);
            }
            if(state == START) {
                while(!stash.empty()) {
                    auto tmp = exec(stash.front());
                    for(auto const& [k,v]: tmp)
                        result.insert({k,v});
                    stash.pop();
                }
            }

            return result;
        };
    protected:

        sample exec(sample const& s) {
            auto [_com, _data] = unpack(s);
            auto [type, com] = _com;
            if(type != -1) {
                if(type == CommandType::Execute) {
                    state = ExecutorInterface::Execution(com);
                }
                else if(type == CommandType::Modify) {
                    auto data = std::any_cast<dictOf <std::string>>(_data);
                    std::string type = data["type"],
                    name = data["name"],
                    expr = data["expr"],
                    true_state = data["true_state"], false_state = data["false_state"],
                    parent = data["parent"],
                    before = data["before"],
                    instead = data["instead"];
                    if(name.empty())
                        return make_exception("no name in Modify call");
                    if(com == INSERT) {
                        if(parent.empty()) {
                            return make_exception("insert " + name + ": no parent");
                        }
                        
                        insert(new_node, parent, before);


                    }
                }
            }
            else {
                return callback(s);
            }
            return {};
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
                    auto new_node = new Leaf(name, *memory, node_function, from_string(true_state), from_string(false_state));
                    if(type == "condition") {
                        link(new_node, memory->used_vars(expr));
                    }
                    nodes[name] = new_node;
                    return {{name, (NodeInterface*)new_node}};
                }
                else {
                    return make_exception("create "+name + ": bad expression");
                }
            }
            else {
                if(type == "parallel") {
                    auto new_node = new Parallel(SUCCESS, name, *memory);
                    nodes[name] = new_node;
                    return {{name, (NodeInterface*)new_node}};
                }
                else if(type == "sequence" || type == "selector" || type == "skipper") {
                    NodeState return_state(RUNNING);
                    if(type == "sequence") return_state = SUCCESS;
                    else if(type == "selector") return_state = FAILURE;
                    else if(type == "skipper") return_state = RUNNING;
                    auto new_node = new Sequential(return_state, name, *memory);
                    nodes[name] = new_node;
                    return {{name, (NodeInterface*)new_node}};
                }
                else return make_exception("create "+name + ": unknown node type " + type);
            }
        }
        
        sample callback(sample const& s) {

        }
        void refresh_orders() {
            if(root)
                root->bfs_with_handler([](NodeInterface* node) {std::any_cast<ABTM_NodeInfo*>(node->info)->reset_order(node);});
        }

        void link(NodeInterface* node, keys vars) {
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
        void replace(std::string const& from, NodeInterface* to) {
            if(nodes.count(from)) {
                auto nfrom = nodes[from];
                dynamic_cast<ControlInterface*>(std::any_cast<ABTM_NodeInfo*>(nfrom->info)->parent)->replace(nfrom, to);
                delete_node(nodes[from]);
                refresh_orders();
            }
        }

        void erase(std::string const& node) {
            if(nodes.count(node)) {
                dynamic_cast<ControlInterface*>(std::any_cast<ABTM_NodeInfo*>(nodes[node]->info)->parent)->erase(nodes[node]);
                delete_node(nodes[node]);
                refresh_orders();
            }
        }

    };
}

#endif //ABTM2_ABTM_H
