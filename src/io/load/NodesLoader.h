//
// Created by safoex on 09.09.19.
//

#ifndef ABTM2_NODESLOADER_H
#define ABTM2_NODESLOADER_H

#include "Loader.h"

#define NODES_DEBUG false

namespace abtm {
    class NodesLoader : public Loader {

    protected:
        dictOf<std::string> get_node_info(std::string const& name, YAML::Node const& node) {
            dictOf<std::string> node_info;

            node_info["name"] = name;
            if(!node["type"])
                throw std::runtime_error("Node " + name + " does not have a type field!");
            auto type = node["type"].as<std::string>();
            if (node.IsMap())
                for (auto const &kv: node) {
                    auto const& k = kv.first.as<std::string>();
                    if(kv.second.IsScalar())
                        node_info[k] = kv.second.as<std::string>();
                }
            else
                return {};
            return node_info;
        }

        keys new_nodes;
        YAML::Node tree_rep;
        bool tree_built;


        std::string find_root() {
            if(loaders->executor->root != nullptr) {
                return loaders->executor->root->id();
            }
            else {
                for(auto const& [k,n]: loaders->tree_description) {
                    if (NODES_DEBUG)
                        std::cout << k << std::endl;
                    if(n["parent"] && (n["parent"].IsNull() || (n["parent"].IsScalar() && n["parent"].as<std::string>().empty()))) {
                        return k;
                    }
                }
            }
        }

        std::queue<std::string> make_building_queue() {
            auto node = find_root();
            if(NODES_DEBUG)
                for(auto const& [k,n]: loaders->tree_description)
                    std::cout << k << std::endl;
            std::vector<std::string> next{node}, next2;
            std::queue<std::string> to_insert;
            while(!next.empty()) {
                for (auto const &n: next) {
                    if (new_nodes.count(n)) {
                        to_insert.push(n);
                    }
                    if (loaders->tree_description[n]["children"]) {
                        for (auto const &nc: loaders->tree_description[n]["children"]) {
                            next2.push_back(nc.as<std::string>());
                        }
                    }
                }
                next = std::move(next2);
                next2.clear();
            }
            return to_insert;
        }
    public:
        NodesLoader(Loaders* loaders) : Loader(loaders), tree_built(false) {
            optional_prerequisties = {"vars", "templates"};
            trigger_vars = {{BUILD_TREE_WORD, std::string()}, {ROS_GET_TREE_WORD, std::string()}};
            required_vars = ALL_CHANGED;
        }

        sample process(sample const& _s) override {
            auto s = _s;
            if(s.count(WORD())) {
                if(!s.count(FORMAT_WORD))
                    s[FORMAT_WORD] = std::string(YAML_WORD);
                if(!s.count(TICKET_WORD))
                    s[TICKET_WORD] = std::string("");
                return load(s);
            }
            else if(s.count(BUILD_TREE_WORD)) {
                auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
                auto result = build_tree(s);
                result[RESPONSE_WORD] = result.empty() ? OK_WORD : EXCEPTION_WORD;
                result[TICKET_WORD] = ticket;
                return result;
            }
            else if(s.count(ROS_GET_TREE_WORD)) {
                sample result;
                std::stringstream sstream;
                std::cout << ROS_GET_TREE_WORD << std::endl;
                if(tree_built) {
                    sstream << tree_rep;
                    result[YAML_TREE_WORD] = sstream.str();
                }
                return result;
            }
            else {
                return {};
            }
        }

        sample load_yaml(sample const& s, YAML::Node const& yn) override {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            sample result;
            if(NODES_DEBUG)
                std::cout << keys_of(s) << std::endl;

            bool not_a_map = false, has_no_build_word = false;

            if(yn.IsMap()) {
                if(NODES_DEBUG)
                    std::cout << yn << std::endl;
                for (auto const &ynn: yn) {
                    auto name = ynn.first.as<std::string>();
                    if(NODES_DEBUG)
                        std::cout << '\t' << "loading node " << name << std::endl;
                    auto type = ynn.second["type"].as<std::string>();
                    if (type.substr(0, 2) == "t/" || type.substr(0, 9) == "template/") {
                        auto ynm = ynn.second;
                        ynm["name"] = name;
                        ynm[LOAD_WORD] = LOAD_WORD;
                        tree_rep[name] = ynn.second;
                        tree_rep[name]["template_type"] = ynn.second["type"];
                        auto response = loaders->loaders["templates"]->load_yaml(s, ynm);
                        if (!get_exception(response).empty()) {
                            result = response;
                            break;
                        }
                    } else {
                        if(tree_rep[name]) {
                            auto type = tree_rep[name]["type"].as<std::string>();
                            if(type.substr(0, 2) == "t/" || type.substr(0, 9) == "template/") {
                                for(auto const& p: ynn.second)
                                    tree_rep[name][p.first.as<std::string>()] = p.second;
                            }
                        }
                        else tree_rep[name] = ynn.second;
                        loaders->tree_description[name] = ynn.second;
                        if(NODES_DEBUG)
                            std::cout << name << std::endl;
                        new_nodes.insert(name);
                    }
                }
            }
            else not_a_map = true;

            if(s.count(BUILD_TREE_WORD)) {
                result = build_tree(s);
            }
            else has_no_build_word = true;

            if(has_no_build_word && not_a_map) {
                return make_exception("this is not a valid YAML (top level structure expected to be dict) \
                                      and not a build request!", ticket);
            }
            else {
                result[RESPONSE_WORD] = result.empty() ? OK_WORD : EXCEPTION_WORD;
                result[TICKET_WORD] = ticket;
                return result;
            }
        }

        void fix_parent_for_children_of(std::string const& name) {
            auto &td = loaders->tree_description;
            if(td[name]["children"]) {
                for(auto const& p: td[name]["children"]) {
                    auto const &child = p.as<std::string>();
                    if(td.count(child))
                        td[child]["parent"] = name;
                }
            }
        }

        sample build_tree(sample const& s) {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            sample result;
            auto q = make_building_queue();
            if(NODES_DEBUG)
                std::cout << "building queue size " << q.size() << std::endl;
            while (!q.empty()) {
                auto next = q.front();
                q.pop();
                fix_parent_for_children_of(next);
                auto const& node_info = get_node_info(next, loaders->tree_description[next]);
                auto response = loaders->executor->execute(ABTM::pack(ABTM::Modify, ABTM::INSERT, node_info, ticket));
                if (!get_exception(response).empty()) {
                    result = response;
                    break;
                } // else return make_exception("node " + next + " is not a map", ticket);
            }
            if(get_exception(result).empty()) {
                result[TREE_WORD] = loaders->tree_description;
                std::stringstream sstream;
                sstream << tree_rep;
                result[YAML_TREE_WORD] = sstream.str();
                tree_built = true;
                if(NODES_DEBUG)
                    std::cout << "TREE ADDED" << std::endl;
            }
            else if(NODES_DEBUG)
                std::cout << get_exception(result) << std::endl;

            return result;
        }

        std::string WORD() override {
            return "nodes";
        }
    };
}

#endif //ABTM2_NODESLOADER_H
