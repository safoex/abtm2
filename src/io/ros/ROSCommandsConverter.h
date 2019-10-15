//
// Created by safoex on 07.10.19.
//

#ifndef ABTM2_IOCOMMANDSROSCONVERTER_H
#define ABTM2_IOCOMMANDSROSCONVERTER_H

#include "core/common.h"

#define ROS_COMM_DEBUG false

namespace abtm {
    class ROSCommandsConverter : public IOInterface {
    public:
        ROSCommandsConverter() : IOInterface(ALL_CHANGED, ON_EVERY) {}

        sample process(sample const& s) {
            sample res;
//            for(auto [k,v]: s) {
//                std::cout << k << '\t';
//            }
//            std::cout << std::endl;
            if(s.count(EXCEPTION_WORD)) {
                std::string json_exception = "{\"data\": \"" + string_utils::escape_json(get_exception(s)) + "\"}";
                res[ROS_EXCEPTION_WORD] = json_exception;
                if(ROS_COMM_DEBUG)
                    std::cout << json_exception << std::endl;
            }
            if(s.count(YAML_TREE_WORD)) {
                auto ys = std::any_cast<std::string>(s.at(YAML_TREE_WORD));
                std::string ros_yaml_tree = "{\"data\": \"" + string_utils::escape_json(ys) + "\"}";
                res[ROS_YAML_TREE_WORD] = ros_yaml_tree;
                if(ROS_COMM_DEBUG)
                    std::cout << ros_yaml_tree << std::endl;

            }
            YAML::Node states;
            YAML::Node vars;
            bool any_state = false;
            bool any_var = false;
            for(auto const& [k, v]: s) {
                if(ROS_COMM_DEBUG)
                    std::cout << k << '\t' << is_state_var(k) << '\t';
                if(is_state_var(k)) {
                    try {
                        auto state_str = std::any_cast<std::string>(v);
                        states[k] = std::atof(state_str.c_str());
                    }
                    catch (std::bad_any_cast& e) {
                        states[k] = (int)std::any_cast<double>(v);
                    }
                    any_state = true;
                }
                if(!is_keyword(k) && !is_state_var(k)) {
                    try {
                        vars[k] = std::any_cast<std::string>(v);
                    }
                    catch (std::bad_any_cast& e) {
                        vars[k] = std::any_cast<double>(v);
                    }
                    any_var = true;
                }
            }
            if(any_state) {
                std::stringstream sstream;
                sstream << states;
                if(ROS_COMM_DEBUG)
                    std::cout << states << std::endl;
                res[ROS_STATE_CHANGES_WORD] = "{\"data\": \"" + string_utils::escape_json(sstream.str())+ "\"}";
            }
            if(any_var) {
                std::stringstream sstream;
                sstream << vars;
                if(ROS_COMM_DEBUG)
                    std::cout << vars << std::endl;
//                res[ROS_VAR_CHANGES_WORD] = "{\"data\": \"" + string_utils::escape_json(sstream.str())+ "\"}";
            }

            if(s.count(ROS_COMMAND_WORD)) {
                auto msg = std::any_cast<std::string>(s.at(ROS_COMMAND_WORD));
                rapidjson::Document d;
                d.Parse(msg.c_str());
                std::string command = d["data"].GetString();
                if(command == "start" || command == "stop" || command == "pause") {
                    ExecutorInterface::Execution com_value = ExecutorInterface::START;
                    if (command == "start")
                        com_value = ExecutorInterface::Execution::START;
                    if (command == "stop")
                        com_value = ExecutorInterface::Execution::STOP;
                    if (command == "pause")
                        com_value = ExecutorInterface::Execution::PAUSE;
                    auto com = ExecutorInterface::pack(ExecutorInterface::CommandType::Execute, com_value);
                    res = com;
                }
                else if(command == "vars") {
                    res = {{ROS_VAR_CHANGES_REQUEST_WORD, nullptr}};
                }
            }
            return res;
        }
    };
}

#endif //ABTM2_IOCOMMANDSROSCONVERTER_H
