//
// Created by safoex on 09.09.19.
//

#ifndef ABTM2_TEMPLATESLOADER_H
#define ABTM2_TEMPLATESLOADER_H

#include "Loader.h"
#include <regex>

#define TEMPLATE_DEBUG false

namespace abtm {
    class TemplatesLoader : public Loader {
    public:
        dictOf <std::string> templates;
        const std::string name_symbol = "~", var_symbol = "$", var_symbol_regex = "\\$";
        const std::string alias_prefixes[2] = {"template/", "t/"};

        TemplatesLoader(Loaders* loaders) : Loader(loaders) {

        }

        sample load_template(sample const& s, YAML::Node const& yn) {
            auto id = yn["name"].as<std::string>();
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            try {
                std::stringstream ss;
                ss << yn;
                if(yn["args"]) {
                    if((yn["args"]["required"] && yn["args"]["required"]["type"]) ||
                       (yn["args"]["optional"] && yn["args"]["optional"]["type"]))
                        throw std::runtime_error("Error in template " + id + ": you could not name argument \"type\"");
                }
                templates[id] = ss.str();
                }
            catch (std::exception& e) {
                return make_exception(std::string("Error occured while loading template ") + id + ": " + e.what(), ticket);
            }
            return {{RESPONSE_WORD, OK_WORD},{TICKET_WORD, ticket}};
        }

        sample load_template_node(sample const& s, YAML::Node const& yn) {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            auto id = yn["name"].as<std::string>();

                //if it's type has template alias:
                std::string template_class;
                auto const& type = yn["type"].as<std::string>();
                for(auto const& a: alias_prefixes) {
                    if(a.length() < type.length() && type.substr(0,a.length()) == a) {
                        template_class = type.substr(a.length());
                        break;
                    }
                }

                if(template_class.empty())
                    load<std::string>(yn, "class", template_class);

                if(template_class.empty())
                    return make_exception(std::string("No template class provided for templated node ") + id, ticket);

                auto t = std::regex_replace(templates[template_class], std::regex(name_symbol), "_" + id + "_");
                YAML::Node t_fixed = YAML::Load(t);

                if(TEMPLATE_DEBUG)
                    std::cout << "parse arguments" << std::endl;
                // parse arguments
                dictOf<YAML::Node> replace_args;
                YAML::Node replace_args_view;
                replace_args["name"] = id;
                if(t_fixed["args"]) {
                    auto const& t_args = t_fixed["args"];
                    std::unordered_set<std::string> view_exclude;
                    if(t_args["view_exclude"]) {
                        if(t_args["view_exclude"].IsSequence())
                            for(auto const& ve: t_args["view_exclude"])
                                if(ve.IsScalar())
                                    view_exclude.insert(ve.as<std::string>());
                    }
                    if(t_args["required"])
                        for(auto const& ra: t_args["required"]) {
                            std::string const& arg = ra.as<std::string>();
                            if(yn[arg]) {
                                replace_args[arg] = yn[arg];
                                if(!view_exclude.count(arg))
                                    replace_args_view[arg] = YAML::Clone(yn[arg]);
                            }
                            else
                                return make_exception("No argument " + arg + " by templated node " + id, ticket);
                        }
                    if(t_args["optional"]) {
                        if(!t_args["optional"].IsMap())
                            return make_exception("template " + type + ": optional args should be a Map");
                        for(auto const& oa: t_args["optional"]) {
                            std::string const& arg = oa.first.as < std::string >();
                            YAML::Node arg_node;
                            if(yn[arg]) {
                                arg_node = yn[arg];
                                if(!view_exclude.count(arg))
                                    replace_args_view[arg] = YAML::Clone(arg_node);
                            }
                            else
                                arg_node = oa.second.as< std::string >();
                            replace_args[arg] = arg_node;

                        }
                    }
                    if(t_args["construct"]) {
                        for(auto const& _a: t_args["construct"]) {
                            auto const &arg = _a.first.as<std::string>();
                            auto const &a = _a.second;
                            try {
                                // extract source
                                YAML::Node source;
                                if(a["from"]) {
                                    if(a["from"].IsMap() || a["from"].IsSequence())
                                        source = a["from"];
                                    else if(a["from"].IsScalar()) {
                                        if(replace_args.count(a["from"].as<std::string>()))
                                            source = replace_args[a["from"].as<std::string>()];
                                    }
                                    else return make_exception("Source for arg "+arg +" for node " + id +"is not a Map, Sequence or other arg", ticket);
                                }
                                else return make_exception("No source (field \"from\" provided for arg " +arg +" for node " + id, ticket);

                                // apply transform
                                std::string K, V;
                                load<std::string>(a, "K", K);
                                load<std::string>(a, "V", V);
                                YAML::Node new_arg;
                                if(V.empty()) {
                                    V = var_symbol + "V";
                                }
                                if(K.empty()) {
                                    K = var_symbol + "K";
                                }
                                if(source.IsSequence()) {
                                    int k = 0;
                                    for(auto const& _v: source) {
                                        std::string new_V = V;
                                        auto const& v = _v.as<std::string>();
                                        new_V = std::regex_replace(new_V, std::regex(var_symbol_regex+"V"), v);
                                        new_V = std::regex_replace(new_V, std::regex(var_symbol_regex+"K"), std::to_string(k));
                                        new_arg.push_back(new_V);
                                        k++;
                                    }
                                }
                                if(source.IsMap()) {
                                    for(auto const& kv: source) {
                                        std::string new_V = V, new_K = K;
                                        std::string const& k = kv.first.as<std::string>(),
                                                v = kv.second.as<std::string>();
                                        new_V = std::regex_replace(new_V, std::regex(var_symbol_regex+"V"), v);
                                        new_V = std::regex_replace(new_V, std::regex(var_symbol_regex+"K"), k);
                                        new_arg.push_back(new_V);
                                    }
                                }

                                // set to required_args
                                replace_args[arg] = new_arg;
                            }
                            catch(std::exception& e) {
                                return make_exception("Error while constructing argument " + arg + " for node " + id, ticket);
                            }
                        }
                    }
                }
                if(TEMPLATE_DEBUG) {
                    std::cout << "args are: " << std::endl;
                    for (auto const &kv: replace_args) {
                        std::cout << '\t' << kv.first << '\t' << kv.second << std::endl;
                    }
                    std::cout << std::endl;

                    // replace args with their subs

                    std::cout << "first, unpack all Map/Sequence variables:" << std::endl;
                    // first, unpack all Map/Sequence variables:
                }
                YAML::Node t_unpacked = t_fixed;
                if(t_fixed["unpack"]) {
                    for(auto const& p: t_fixed["unpack"]) {
                        auto const& var = p.first.as<std::string>();

                        int _k = 0;
                        YAML::Node cell_unpacked;
                        for(auto const& kv: replace_args[var]) {
                            std::stringstream _cell;
                            _cell << p.second;
                            std::string cell(_cell.str());
                            std::string k,v;
                            if(replace_args[var].IsSequence()) {
                                k = std::to_string(_k);
                                v = kv.as<std::string>();
                            }
                            else {
                                k = kv.first.as<std::string>();
                                v = kv.second.as<std::string>();
                            }
                            cell = std::regex_replace(cell, std::regex(var_symbol_regex+"V"), v);
                            cell = std::regex_replace(cell, std::regex(var_symbol_regex+"K"), k);
                            cell_unpacked = (YAML::Load(cell));
//                            std::cout << cell_unpacked << std::endl;
                            for(auto const& p: cell_unpacked) {
                                auto const& key = p.first.as<std::string>();
                                for(auto const& p2 : p.second)
                                    t_unpacked[key][p2.first.as<std::string>()] = YAML::Clone(p2.second);
                            }
                        }

                    }
                }
                if(TEMPLATE_DEBUG) {
                    std::cout << "unpacked " << std::endl;
                    std::cout << "-----------------------------" << std::endl;
                    std::cout << t_unpacked << std::endl << std::endl;
                    std::cout << "-----------------------------" << std::endl;

                    std::cout << "second, replace Map/Sequence variables with their subs:" << std::endl;
                }
                // second, replace Map/Sequence variables with their subs:

                t_unpacked = reqursively_replace(t_unpacked, replace_args);
                if(TEMPLATE_DEBUG)
                    std::cout << "third, find-and-replace usual variables:" << std::endl;
                // third, find-and-replace usual variables:
                std::stringstream ss;
                ss << t_unpacked;
                auto t_str = ss.str();

                for(auto const& a: replace_args) {
                    if(a.second.IsScalar()) {
                        auto const &to = a.second.as<std::string>();
                        auto const &arg = a.first;
                        auto from = var_symbol_regex + arg;

                        //std::cout << "from: " << from << " to: "<< to << std::endl << t_str << std::endl << std::endl;
                        t_str = std::regex_replace(t_str, std::regex(from), to);
                    }
                }

                // now we ready to build templated nodes

                t_fixed = YAML::Load(t_str);

                if(TEMPLATE_DEBUG) {
                    std::cout << "finally unpacked and blahblah" << std::endl;
                    std::cout << "-----------------------------" << std::endl;
                    std::cout << t_fixed << std::endl << std::endl;
                    std::cout << "-----------------------------" << std::endl;
                }


                if(TEMPLATE_DEBUG)
                    std::cout << t_fixed << std::endl;

                if(t_fixed["children"])
                    t_fixed["nodes"][id]["view_children"] = t_fixed["children"];

                auto resp = loaders->loaders[LOAD_WORD]->load_yaml(s, t_fixed);
                return resp;



//                // get fake children for visualization
//                std::vector<std::string> view_children;
//                if(t_fixed["children"])
//                    for(auto const& c: t_fixed["children"])
//                        view_children.push_back(c.as<std::string>());
//                builder.view_graph[id]["children"] = view_children;
//                builder.view_graph[id]["type"] = type;
//
//                // get hide parameter for visualization (if true, then hide children)
//                if(yaml_node["hide"])
//                    builder.view_graph[id]["hide"] = load<int>(yaml_node, "hide");
//
//                std::string default_template_color = "\"#9262d1\"";
//                builder.view_graph[id]["color"] = load(yaml_node, "color", default_template_color);
//
//                std::stringstream ss_classifier;
//                ss_classifier << type + '\n' << replace_args_view;
//                std::string classifier = ss_classifier.str();
//
//                builder.view_graph[id]["class"] = classifier;
//
//                for(auto const& n: t_fixed["nodes"]) {
//                    auto const& node = n.first.as<std::string>();
//                    bool is_in_view_children = false;
//                    for(auto const& vc: view_children)
//                        if(node == vc)
//                            is_in_view_children = true;
//                    if(!is_in_view_children && node != id)
//                        builder.view_graph.erase(node);
//                }


        }

        std::string replace_args_in_string( std::string init,
                                            dictOf<YAML::Node> const& replace_args,
                                            bool scalars = true) {

            for(auto const& a: replace_args) {
                auto const &arg = a.first;
                if(init.find(var_symbol + arg) != std::string::npos) {
                    std::string to;
                    std::stringstream ss;
                    ss << YAML::Clone(a.second);
                    to = ss.str();
                    auto from = var_symbol_regex + arg;
                    auto old = init;
                    init = std::regex_replace(init, std::regex(from), to);
//                    std::cout << "replaced from " + old + " to " + init << std::endl;
//                    std::cout << "\t with args: \"from\": " + from + '\n' + '\t' + "\"to\": " + to << std::endl;
                }

            }
            return init;
        }


        YAML::Node reqursively_replace(YAML::Node const& yn, dictOf<YAML::Node> const& rep_args) {
            YAML::Node result;
            if(TEMPLATE_DEBUG)
                std::cout << yn << std::endl << std::endl;
            if(yn.IsMap() || yn.IsSequence()) {
                if (yn.IsMap())
                    for (auto const &p: yn)
                        result[p.first.as<std::string>()] = reqursively_replace(p.second, rep_args);

                if (yn.IsSequence())
                    for (auto const &p: yn)
                        result.push_back(reqursively_replace(p, rep_args));

                return result;
            }
            else if(yn.IsScalar()) {
                auto s = yn.as<std::string>();
                if((s.substr(0, var_symbol.length()) == var_symbol) && rep_args.count(s.substr(1))) {
                    result = YAML::Clone(rep_args.at(s.substr(1)));
                }
                else {
                    bool has_vars_inside = false;

                    for(auto const& [k,v]: rep_args) {
                        has_vars_inside |= s.find(var_symbol + k) != std::string::npos;
                    }

                    if(has_vars_inside) {
                        result = replace_args_in_string(s, rep_args, false);
                    }
                    else result = yn;
                }
                if(TEMPLATE_DEBUG) {
                    std::cout << "!!!!" << s << " was replaced with " << result << std::endl << std::endl;
                    std::cout << s.substr(1) << ' ' << (s.substr(0, var_symbol.length()) == var_symbol) << std::endl;
                }
                return result;
            }
            else return yn;
        }

        sample load_yaml(sample const& s, YAML::Node const& yn) override {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            if(yn.IsMap()) {
                sample result;
                sample vars;

                if(!yn[LOAD_WORD]) {
                        for(auto const& n: yn) {
                            auto node = n.second;
                            node["name"] = n.first.as<std::string>();
                            result = load_template(s, node);
                            if(!get_exception(result).empty()) {
                                break;
                            }
                        }
                }
                else {
                    result = load_template_node(s, yn);
                }
                if(get_exception(result).empty()) {
                    try {
                        loaders->executor->memory->add(vars);
                    }
                    catch (std::runtime_error & e) {
                        result = make_exception(std::string("exception occured while loading vars: ") + e.what(), ticket);
                    }
                }
                if(!result.count(RESPONSE_WORD))
                    result[RESPONSE_WORD] = result.empty() ? OK_WORD : EXCEPTION_WORD;
                result[TICKET_WORD] = ticket;
                return result;
            }
            return make_exception("this is not a valid YAML (top level structure expected to be dict", ticket);
        }
        std::string WORD() override {
            return "templates";
        }




    };
}

#endif //ABTM2_TEMPLATESLOADER_H
