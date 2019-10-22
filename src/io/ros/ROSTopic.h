//
// Created by safoex on 30.09.19.
//

#ifndef ABTM2_ROSTOPIC_H
#define ABTM2_ROSTOPIC_H

#include "rosbridge_ws_client.hpp"
#include "io/IOInterface.h"


#define ROS_DEBUG false

namespace abtm {
    class ROSTopic : public IOInterface {
    public:
        enum ROSTOPIC_TYPE {
            Publisher,
            Subscriber
        };
    protected:
        RosbridgeWsClient& rbc;
        std::string topic_name;
        std::string msg_key;
        std::string client;
        std::unordered_set<std::string> trigger_keys;
        ROSTOPIC_TYPE type;
    public:
        ROSTopic(RosbridgeWsClient& rbc,  ROSTOPIC_TYPE type, std::string const& topic_name, std::string const& msg_key,
                std::unordered_set<std::string> trigger_keys = {}) : rbc(rbc), topic_name(topic_name), IOInterface({},{}),
                trigger_keys(trigger_keys), msg_key(msg_key), type(type), client(topic_name) {

            if(trigger_keys.empty())
                trigger_keys = {msg_key};
            for(auto const& k: trigger_keys)
                trigger_vars[k] = double(0);
            required_vars = {{msg_key, double(0)}};
            rbc.addClient(client);
        };


        virtual ~ROSTopic() {
            rbc.removeClient(client);
            std::cout << "stopped and removed " + client << std::endl;
        };
    };


    class ROSPublisher : public ROSTopic {
    public:
        ROSPublisher (RosbridgeWsClient& rbc,  std::string const& topic_name, std::string const& msg_type,
                      std::string const& msg_key, std::unordered_set<std::string> const& trigger_keys = {})
                : ROSTopic(rbc, Publisher, topic_name, msg_key, trigger_keys){
            rbc.advertise(client, topic_name, msg_type);
        }

        sample process(sample const& output) override {
            std::string msg_str;
            if(ROS_DEBUG)
                std::cout << "ROS processing " << std::endl;
            if(output.count(msg_key)) {
                try {
                    msg_str = std::any_cast<std::string>(output.at(msg_key));
                }
                catch (std::bad_any_cast &e) {
                    throw std::runtime_error("Key " + msg_key + " in sample for topic " + topic_name +" does not contain a std::string");
                }
            }
            if(ROS_DEBUG)
                std::cout << msg_str << std::endl;
            rapidjson::Document d;
            d.Parse(msg_str.c_str());
            rbc.publish(topic_name, d);
            return {};
        };


    };

    class ROSSubscriber : public ROSTopic {
        InputFunction f;
    public:
        ROSSubscriber(RosbridgeWsClient& rbc, std::string const& topic_name, std::string const& msg_type,
                      std::string const& msg_key) : ROSTopic(rbc, Subscriber, topic_name, msg_key, {}) {
            rbc.addClient(client+"__a__");
            rbc.advertise(client+"__a__", topic_name, msg_type);
//            using std::chrono_literals::operator""ms;
//            std::this_thread::sleep_for(100ms);
            rbc.subscribe(client, topic_name, [this](
                    std::shared_ptr<WsClient::Connection> /* connection */, std::shared_ptr<WsClient::InMessage> message)
            {
                rapidjson::Document d;
                std::string msg(message->string());
                d.Parse(msg.c_str());

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                d["msg"].Accept(writer);

                if(ROS_DEBUG)
                    std::cout << buffer.GetString() << std::endl;
                if(this->f)
                    this->f({{this->msg_key, std::string(buffer.GetString())}});
            });
        }

        void set_subscriber(InputFunction const& from_mimo) {
            f = from_mimo;
        }

        ~ROSSubscriber() override {
            this->f = InputFunction();
            rbc.removeClient(client+"__a__");
        };
    };
}

#endif //ABTM2_ROSTOPIC_H
