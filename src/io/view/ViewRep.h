//
// Created by safoex on 10.09.19.
//

#ifndef ABTM2_VIEWREP_H
#define ABTM2_VIEWREP_H

#include <core/exec/ExecutorInterface.h>
#include "io/IOInterface.h"
#include "yaml-cpp/yaml.h"
#include "core/exec/ABTM.h"
#include <cctype>
#include <iomanip>
#include <sstream>

#define VIEW_DEBUG false

namespace string_utils
{
    std::string escape(std::string str)
    {
        std::stringstream stream;

        stream << std::uppercase
               << std::hex
               << std::setfill('0');

        for(char ch : str)
        {
            int code = static_cast<unsigned char>(ch);

            if (std::isprint(code))
            {
                stream.put(ch);
            }
            else
            {
                stream << "\\x"
                       << std::setw(2)
                       << code;
            }
        }

        return stream.str();
    }

    std::string escape_json(std::string const& str) {
        std::stringstream stream;

        std::unordered_map<char, std::string> replace{
                {'\n', "\\n"},
                {'\t', "\\t"},
                {'\b', "\\b"},
                {'\\', "\\\\"},
                {'\r', "\\r"},
                {'\f', "\\f"},
                {'\"', "\\\""}
        };

        for(char ch : str)
        {
            if(replace.count(ch)) {
                stream << replace[ch];
            }
            else {
                stream << ch;
            }
        }

        return stream.str();
    }
}


namespace abtm {
    class ViewRep : public IOInterface {
    protected:
        tree_rep tree;
        dictOf<std::string> states;
        ExecutorInterface* exec;
        dictOf<std::pair<std::string, std::string>> colors;
        YAML::Node new_tree_rep;
    public:
        ViewRep(ExecutorInterface* exec) : IOInterface(), exec(exec) {
            trigger_vars = ALL_CHANGED;
            required_vars = ALL_CHANGED;
            colors =  {
                    {"sequence",{"white","->"}},
                    {"selector",{"white","?"}},
                    {"parallel",{"white","="}},
                    {"skipper", {"white","=>"}},
                    {"action",{"green",""}},
                    {"condition",{"orange",""}},
                    {"default", {"\"#9262d1\"", "<...>"}}
            };
        }

        sample process(sample const& s) {
            sample response = {};
            bool tree_changed = false;
            if(s.count(TREE_WORD)) {
                if(VIEW_DEBUG)
                    std::cout << "VIEW REP FOUND TREE_WORD" << std::endl;
                tree = std::any_cast<tree_rep>(s.at(TREE_WORD));
                tree_changed = true;
            }
            bool states_changed = false;
            for(auto const& [k,v]: s) {
                if(is_state_var(k)) {
                    states[k] = to_string(exec->memory->get_state(k));
                    states_changed = true;
                }
            }
            if(tree_changed) {
                auto dot_tree_description = make_dot_tree_description();
                response[DOT_TREE_WORD] = dot_tree_description;
                std::string ros_msg = "{\"data\":\"";
                ros_msg += string_utils::escape_json(dot_tree_description);
                ros_msg += "\"}";
                response[DOT_TREE_WORD_ROS] = ros_msg;
            }

            if(tree_changed || states_changed)
                response[DOT_RT_TREE_WORD] = make_dot_rt_tree_description();

            if(!response.empty()) {
                response[TICKET_WORD] = EMPTY_STR;
            }
            return response;
        }


        std::string make_dot_tree_description(bool rt = false) {

            std::string result;
            result += "digraph g {\n";
            result += "node [shape=rectangle, style=filled, color=white];\n";


            dictOf<std::pair<std::string, std::string>> gv_names;

            for (auto const& [k, n]: tree) {
                auto const& type = n["type"].as<std::string>();
                std::string desc;
                if(rt)
                    desc = states[k];
                else {
                    if(type == "action" || type == "condition") {
                        desc = string_utils::escape(n["expr"].as<std::string>());
                    }
                }

                //define node

                std::string gv_node_name;
                gv_node_name += "\"";
                std::pair<std::string, std::string> p2;
                if(colors.count(type))
                    p2 = colors.at(type);
                else
                    p2 = colors.at("default");
                auto [color, hat] = p2;
//                if(n.hide_further) color = "\"#16abc9\"";
                if(!hat.empty())
                    gv_node_name += hat + "\n";
                gv_node_name += "" + k + "";
                if(!desc.empty())
                    gv_node_name += "\n" + desc;
                gv_node_name += "\"";
                std::string fontcolor = "black";
                if(type != "condition" && desc == "RUNNING")
                    fontcolor = "red";
                gv_names[k] = {"\"" + k + "\"", "\""+k+"\"" + "[label=" +  gv_node_name + ", color=" + color
                                                          + ", fontcolor=" + fontcolor + "];\n"};
            }

            std::string root;
            for(auto [k,v]: tree) {
                if(v["parent"].as<std::string>().empty()) {
                    root= k;
                }
            }

            std::stack<std::string> _dfs;
            _dfs.push(root);
            std::string constraints;
            int n_in_dfs = 0;
            while (!_dfs.empty()) {
                n_in_dfs++;
                auto s = _dfs.top();
                auto& _n = tree[s];
                _dfs.pop();
                result += gv_names[s].second;

                std::string constraint = "{ rank = same;\n";
                if(_n["children"] ) {
                    for(auto const& k: _n["children"]) {
                        _dfs.push(k.as<std::string>());
                    }
                    std::string first;
                    bool first_assigned = false;
                    for(auto const& k: _n["children"]) {
                        auto child = k.as<std::string>();
                        if(!first_assigned) {
                            first = child;
                            first_assigned = true;
                        }
                        result += gv_names[s].first + " -> " + gv_names[child].first + ";\n";
                        if (child != first) {
                            constraint += " -> ";
                        }
                        constraint += gv_names[child].first;
                    }
                    constraint += "[style=invis];\n}\n";
                    if (_n["children"].size() > 1) constraints += constraint;
                }
            }


            result += constraints;
            result += "\n}";
            if(VIEW_DEBUG)
                std::cout << "Nodes in dfs: " << n_in_dfs << std::endl;
            return result;
        }

        std::string make_dot_rt_tree_description() {
            return make_dot_tree_description(true);
        };
    };
}

#endif //ABTM2_VIEWREP_H
