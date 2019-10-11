//
// Created by safoex on 09.09.19.
//

#ifndef ABTM2_VARSLOADER_H
#define ABTM2_VARSLOADER_H
#include "Loader.h"

#define VARS_LOADER_DEBUG true
namespace abtm {
    template<typename T>
    class VarsLoader : public Loader {
    public:
        VarsLoader(Loaders* loaders) : Loader(loaders) {};
        sample load_yaml(sample const& s, YAML::Node const& yn) override {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            if(yn.IsMap()) {
                sample result;
                sample vars;

                for(auto const& ynn: yn) {
                    auto key = ynn.first.as<std::string>();
                    if(VARS_LOADER_DEBUG)
                        std::cout << '\t' << "loading var " << key << std::endl;
                    T value;
                    try {
                        value = ynn.second.as<T>();
                    }
                    catch (std::exception &e) {
                        result = make_exception("value for key " + key + " of incorrect format", ticket);
                        break;
                    }
                    vars[key] = value;
                }
                if(get_exception(result).empty()) {
                    try {
                        loaders->executor->memory->add(vars);
                    }
                    catch (std::runtime_error & e) {
                        result = make_exception(std::string("exception occured while loading vars: ") + e.what(), ticket);
                        if(VARS_LOADER_DEBUG)
                            std::cout << get_exception(result) << std::endl;
                    }
                }
                result[RESPONSE_WORD] = result.empty() ? OK_WORD : EXCEPTION_WORD;
                result[TICKET_WORD] = ticket;
                return result;
            }
            return make_exception("this is not a valid YAML (top level structure expected to be dict", ticket);
        }
        std::string WORD() override {
            return "vars";
        }
    };
}

#endif //ABTM2_VARSLOADER_H
