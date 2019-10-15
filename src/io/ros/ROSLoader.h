//
// Created by safoex on 30.09.19.
//

#ifndef ABTM2_ROSLOADER_H
#define ABTM2_ROSLOADER_H

#include "ROSSimpleActionClient.h"
#include "io/load/Loader.h"


namespace abtm {
    class ROSLoader : public Loader {
        friend class ROSTopic;
        friend class ROSPublisher;
        friend class ROSSubscriber;
    protected:
        RosbridgeWsClient rbc;
        std::string client;
        std::vector<IOInterface*> topics;
        IOCenter* mimo;
    public:
        ROSLoader(Loaders* loaders, IOCenter* mimo, std::string const& client_name, std::string const& rosbridge_port = "localhost:9090")
        : rbc(rosbridge_port), Loader(loaders), mimo(mimo), client(client_name) {
//            rbc.addClient(client);
        };

        sample load_yaml(sample const& s, YAML::Node const& yn) override {
            auto ticket = std::any_cast<std::string>(s.at(TICKET_WORD));
            sample result;
            if(yn.IsMap()) {
                for(auto const& p: yn) {
                    if(p.second["type"]) {
                        std::string type;
                        try{
                            type = p.second["type"].as<std::string>();
                        }
                        catch(std::exception &e) {
                            throw std::runtime_error("type should be a string in " + p.first.as<std::string>() + " in ROS description");
                        }
                        if(type == "subscriber" || type == "sub" || type == "publisher" || type == "pub") {
                            std::string msg_key, msg_type, trigger_key;
                            std::unordered_set<std::string> trigger_keys;
                            load<std::string>(p.second, "var", msg_key);
                            load<std::string>(p.second, "msg", msg_type);

                            if(p.second["trigger"] && p.second["trigger"].IsSequence())
                                for(auto const& tv: p.second["trigger"]) {
                                    trigger_keys.insert(tv.as<std::string>());
                                }
                            else
                                load<std::string>(p.second, "trigger", trigger_key);
                            if(trigger_keys.empty()) {
                                if(trigger_key.empty())
                                    trigger_keys.insert(msg_key);
                                else
                                    trigger_keys.insert(trigger_key);
                            }
                            if(ROS_DEBUG)
                                std::cout <<"ROS " << type << '\t' << p.first.as<std::string>() << '\t' << msg_key << std::endl;
                            if(type == "subscriber" || type == "sub") {
                                auto sub = new ROSSubscriber(rbc, p.first.as<std::string>(), msg_type, msg_key);
                                sub->set_subscriber(mimo->registerIOchannel(sub));
                                topics.push_back(sub);
                            }
                            else if(type == "publisher" || type == "pub") {
                                auto pub = new ROSPublisher(rbc, p.first.as<std::string>(), msg_type, msg_key, trigger_keys);
                                mimo->registerIOchannel(pub);
                                topics.push_back(pub);
                            }
                        }
                        else if(type == "simple_action_client" || type == "sac") {
                            std::string as_name, action, package;
                            std::string simple_status_var;
                            std::string goal_var, cancel_var, feedback_var, result_var;
                            std::string goal_id_var;
                            load<std::string>(p.second, "server", as_name);
                            load<std::string>(p.second, "package", package);
                            load<std::string>(p.second, "action", action);
                            load<std::string>(p.second, "goal", goal_var);
                            load<std::string>(p.second, "feedback", feedback_var);
                            load<std::string>(p.second, "result", result_var);
                            load<std::string>(p.second, "goal_id", goal_id_var);
                            load<std::string>(p.second, "status", simple_status_var);
                            load<std::string>(p.second, "cancel", cancel_var);
                            auto sac = new ROSSimpleActionClient(rbc, as_name, package, action, goal_var, cancel_var,
                                                                 feedback_var, result_var, simple_status_var, goal_id_var);
                            sac->set_process(mimo->registerIOchannel(sac));
                            topics.push_back(sac);
                        }
                        else throw std::runtime_error("Not supported handler \"" + type + "\" in ROS description");

                    }
                    else throw std::runtime_error("No type for " + p.first.as<std::string>() + " in ROS description");
                }
            }
            else throw std::runtime_error("ROS description should be a Map");

            result[RESPONSE_WORD] = result.empty() ? OK_WORD : EXCEPTION_WORD;
            result[TICKET_WORD] = ticket;

            return result;
        }

        std::string WORD() override {
            return "ros";
        }

        ~ROSLoader() override {
            for(auto t:topics)
                delete t;

//            rbc.stopClient(client);
//            rbc.removeClient(client);
        };
    };
}



#endif //ABTM2_ROSLOADER_H
