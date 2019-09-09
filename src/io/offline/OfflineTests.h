//
// Created by safoex on 06.09.19.
//

#ifndef ABTM2_OFFLINETESTS_H
#define ABTM2_OFFLINETESTS_H

#include "io/IOInterface.h"
#include "yaml-cpp/yaml.h"

#include "../test/testing.h"

#define NO_SAMPLE_STR "__NO_SAMPLE__"
#define NO_SAMPLE sample{{NO_SAMPLE_STR, nullptr}}

namespace abtm {
    template<class T>
    class OfflineTests : public IOInterface {
    protected:
        YAML::Node yn_input;
        IOCenter* io;
        InputFunction start;
        YAML::Emitter* emit;
        std::map<std::string, T> out;
        sample after;


        sample parse_to_sample(YAML::Node const& yn) {
            sample s;
            if(yn.IsMap())
                for(auto const& kv: yn) {
                    auto const& key = kv.first.as<std::string>();
                    try {
                        s[key] = kv.second.as<T>();
                    }
                    catch(YAML::Exception &e) {
                        throw std::runtime_error("Value of var" + key + " in sample should be either double or string (json)");
                    }
                }
            else throw std::runtime_error("Sample should be a Map");
            return s;
        }

    public:
        OfflineTests(IOCenter* io, sample after = NO_SAMPLE) : IOInterface(ALL_CHANGED, ON_EVERY), io(io), emit(nullptr),
        after(after) {
            start = io->registerIOchannel(this);
        };

        sample process(sample const& s) override {
            if(!emit)
                return {};
            for(auto const& kv: s) {
                try {
                    auto const& key = kv.first, val = std::any_cast<T>(kv.second);
                    out[key] = val;
                }
                catch(std::bad_any_cast &e) {
                    throw std::runtime_error("Value of var \"" + kv.first + "\" is not string");
                }
            }
            return {};
        }

        void apply_tests(YAML::Node const& input, std::ostream &output) {
            emit = new YAML::Emitter;
            if(input.IsSequence()) {
                *emit << YAML::BeginSeq;
                for(auto const& s: input) {
                    out.clear();
                    start(parse_to_sample(s));
                    if(!after.count(NO_SAMPLE_STR)) {
                        start(after);
                    }
                    if(out.empty())
                        *emit << std::map<std::string, std::string>({{"empty","response"}});
                    else
                        *emit << out;
                }
                *emit << YAML::EndSeq;
            }
            else {
                throw std::runtime_error("Input for offline test should be sequence");
            }
            output << emit->c_str();
            delete emit;
            emit = nullptr;
        }

        void apply_tests(std::istream &input, std::ostream &output) {
            apply_tests(YAML::Load(input), output);
        }

        ~OfflineTests() override = default;

    };
}

#endif //ABTM2_OFFLINETESTS_H
