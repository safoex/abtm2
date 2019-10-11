//
// Created by safoex on 09.09.19.
//

#ifndef ABTM2_LOADER_H
#define ABTM2_LOADER_H

#include <core/exec/ExecutorInterface.h>
#include "io/IOInterface.h"
#include "yaml-cpp/yaml.h"
#include "core/exec/ABTM.h"


#define LOADER_DEBUG false

namespace abtm {
    class Loader;

    class Loaders {
    public:
        dictOf<Loader*> loaders;
        std::list<std::string> loader_queue;
        tree_rep tree_description;
    public:
        void add_loader(std::string const& name, Loader* loader, std::string const& before = "", std::string const& after = "") {
            loaders[name] = loader;
            if(!before.empty() && after.empty()) {
                loader_queue.insert(std::find(loader_queue.begin(), loader_queue.end(), before), name);
            }
            if(before.empty() && after.empty()) {
                loader_queue.push_front(name);
            }
            if(!after.empty() && before.empty()) {
                auto after_pos = std::find(loader_queue.begin(), loader_queue.end(), after);
                std::advance(after_pos, 1);
                loader_queue.insert(after_pos, name);
            }
        }
        ABTM* executor;
        Loaders(ABTM* executor = nullptr) : executor(executor) {} ;
    };

    class Loader : public IOInterface {
    public:
        keys dependencies, prerequisties, optional_prerequisties;
        dictOf <std::string> tickets;
        Loaders* loaders;
        Loader(Loaders* loaders) : loaders(loaders)   {
            trigger_vars = {{LOAD_WORD, nullptr}, {RESPONSE_WORD, nullptr}};
            required_vars = ALL_CHANGED;
        }

        sample process(sample const& _s) override {
            if(_s.count(WORD())) {
                auto s = _s;
                if(!s.count(FORMAT_WORD))
                    s[FORMAT_WORD] = std::string(YAML_WORD);
                if(!s.count(TICKET_WORD))
                    s[TICKET_WORD] = std::string("");
                return load(s);
            }
            else return {};
        };

        bool is_yaml_format(sample const& s) {
            auto const& format = std::any_cast<std::string>(s.at(FORMAT_WORD));
            return format == YAML_STR_WORD || format == YAML_WORD;
        }

        bool is_json_format(sample const& s) {
            auto const& format = std::any_cast<std::string>(s.at(FORMAT_WORD));
            return format == JSON_STR_WORD || format == JSON_WORD;
        }

        virtual sample load_yaml(sample const& s, YAML::Node const& yn) {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            if(yn.IsMap()) {
                sample result;
                for(auto const& l: loaders->loader_queue) {
                    if(yn[l]) {
                        auto L = loaders->loaders[l];
                        if(LOADER_DEBUG)
                            std::cout << "loading "+ L->WORD() << std::endl;
                        auto res = L->process({{L->WORD(), yn[l]}, {FORMAT_WORD, std::string(YAML_WORD)}, {KEY_WORD, l}/*, {TICKET_WORD, tickets[ticket]}*/});
                        if(!res.count(RESPONSE_WORD) || std::any_cast<std::string>(res.at(RESPONSE_WORD)) != OK_WORD) {
                            result = res;
                            if(LOADER_DEBUG)
                                std::cout << "FAIL" << std::endl;
                            break;
                        }
                    }
                }
                result[RESPONSE_WORD] = result.empty() ? OK_WORD : EXCEPTION_WORD;
                result[TICKET_WORD] = ticket;
                return result;
            }
            return make_exception("this is not a valid YAML (top level structure expected to be dict", ticket);
        }

        virtual sample load(sample const& s) {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            auto const& format = std::any_cast<std::string>(s.at(FORMAT_WORD));
            if(is_yaml_format(s)) {
                YAML::Node yn = get_yaml(s);
                // TODO: optimize
                return load_yaml(s, yn);
            }
            else if(is_json_format(s)) {
                // TODO: support JSON
                return make_exception("sorry, but JSON format is not yet supported", ticket);
            }
            else return make_exception("unknown format: " + format, ticket);
        }


        YAML::Node get_yaml(sample const& s) {
            auto const& format = std::any_cast<std::string>(s.at(FORMAT_WORD));
            if(format == YAML_STR_WORD)
                return  YAML::Load(std::any_cast<std::string>(s.at(WORD())));
            else if(format == YAML_WORD)
                return std::any_cast<YAML::Node>(s.at(WORD()));
            return YAML::Node();
        }

        virtual bool load_description(sample const& s) {

        }

        virtual bool is_correct(sample const& s) {
            return true;
        }

        virtual std::string WORD() {
            return LOAD_WORD;
        }


        template <typename T>
        static T load(const YAML::Node &node, std::string const &key) {
            if(node[key]) {
                try {
                    return node[key].as<T>();
                }
                catch(YAML::Exception &e){
                    throw YAML::Exception(YAML::Mark::null_mark(), std::string() + "Error while reading " + key + " " + e.what());
                }
            }
            return T();
        }

        template <typename T>
        static T& load(const YAML::Node &node, std::string const &key, T& to) {
            if(node[key]) {
                try {
                    to = node[key].as<T>();
                }
                catch(YAML::Exception &e){
                    throw YAML::Exception(YAML::Mark::null_mark(), std::string() + "Error while reading " + key + " " + e.what());
                }
            }
            return to;
        }
    };
}


#endif //ABTM2_LOADER_H
