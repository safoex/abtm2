//
// Created by safoex on 21.10.19.
//

#ifndef ABTM2_OFFLINETESTS_H
#define ABTM2_OFFLINETESTS_H

#include "core/common.h"
#include "io/IOInterface.h"

namespace abtm {
    class OfflineTest : public IOInterface {
    protected:
        YAML::Node yn_input;
        IOCenter* mimo;
        InputFunction start;
        YAML::Emitter* emit;
        std::map<std::string, std::string> out;


        sample parse_to_sample(YAML::Node const& yn) {
            sample s;
            if(yn.IsMap())
                for(auto const& kv: yn) {
                    auto const& key = kv.first.as<std::string>();
                    if(kv.second.IsScalar()) {
                        s[key] = kv.second.as<std::string>();
                    }
                    else throw std::runtime_error("Value of var" + key + " in sample should be either doulbe or string (json)");
                }
            else throw std::runtime_error("Sample should be a Map");
            return s;
        }

    public:
        OfflineTest(IOCenter* mimo) : IOInterface(ALL_CHANGED, ON_EVERY), mimo(mimo), emit(nullptr){
            start = mimo->registerIOchannel(this);
        };

        sample process(sample const& s) override {
            if(!emit)
                return {};
            for(auto const& kv: s) {
                try {
                    auto const& key = kv.first, val = std::any_cast<std::string>(kv.second);
                    out[key] = val;
                }
                catch(std::bad_any_cast &e) {
                    throw std::runtime_error("Value of var \"" + kv.first + "\" is not string");
                }
            }
            *emit << out;
            return {};
        }

        void apply_tests(YAML::Node const& input, std::ostream &output) {
            emit = new YAML::Emitter;
            if(input.IsSequence()) {
                *emit << YAML::BeginSeq;
                for(auto const& s: input) {
                    out.clear();
                    start(parse_to_sample(s));
                    if(out.empty())
                        out = {{"empty","response"}},
                                *emit << out;
                }
                *emit << YAML::EndSeq;
            }
            else {
                throw std::runtime_error("Input for offline test should be a sequence");
            }
            output << emit->c_str();
            delete emit;
            emit = nullptr;
        }

        void apply_tests(std::istream &input, std::ostream &output) {
            apply_tests(YAML::Load(input), output);
        }

        ~OfflineTest() override = default;
    };
}


#endif //ABTM2_OFFLINETESTS_H
