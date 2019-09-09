//
// Created by safoex on 09.09.19.
//

#ifndef ABTM2_NODESLOADER_H
#define ABTM2_NODESLOADER_H

#include "Loader.h"
namespace abtm {
    class NodesLoader : public Loader {
    public:
        NodesLoader() {
            optional_prerequisties = {"vars", "templates"};
        }

        sample load_yaml(sample const& s, YAML::Node const& yn) override {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            if(yn.IsMap()) {
                sample result;

                for(auto const& ynn: yn) {
                    dictOf<std::string> node_info;

                    node_info["name"] = ynn.first.as<std::string>();
                    auto type = ynn.second["type"].as<std::string>();
                    if(type.substr(0,2) == "t/" || type.substr(0, 9) == "template/") {
                        auto ynm = yn;
                        ynm["name"] = node_info["name"];
                        auto response = loaders["templates"]->load_yaml(s, ynm);
                        if(!get_exception(response).empty()) {
                            result = response;
                            break;
                        }
                    }
                    else {
                        if (ynn.second.IsMap())
                            for (auto const &ynn_params: ynn.second) {
                                node_info[ynn_params.first.as<std::string>()] = ynn_params.second.as<std::string>();
                            }
                        else return make_exception("node " + node_info["name"] + "is not a map", ticket);


                        auto response = executor->execute(ABTM::pack(ABTM::Modify, ABTM::INSERT, node_info, ticket));
                        if (!get_exception(response).empty()) {
                            result = response;
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
        std::string WORD() override {
            return "nodes";
        }
    };
}

#endif //ABTM2_NODESLOADER_H
