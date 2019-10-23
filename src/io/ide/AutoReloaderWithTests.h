//
// Created by safoex on 21.10.19.
//

#ifndef ABTM2_AUTORELOADERWITHTESTS_H
#define ABTM2_AUTORELOADERWITHTESTS_H

#include "AutoReloader.h"

namespace abtm {
    class AutoReloaderWithFilter : public AutoReloader {
    protected:
        LoadFabric<std::string, MemoryDuktape> *lf_tests;
        keys yaml_exclude;
    public:
        AutoReloaderWithFilter(ABTM::ExecutionType executionType, std::string const& rosbridge_port = "localhost:9090",
                              int check_every_milliseconds = 1, int tick_every_milliseconds = 0) :
                              AutoReloader(new LoadFabric<std::string, MemoryDuktape>(executionType),
                                      rosbridge_port, check_every_milliseconds, tick_every_milliseconds),
                              lf_tests(new LoadFabric<std::string, MemoryDuktape>(executionType)){
            yaml_exclude.insert("ros");
        }

        YAML::Node filter_excluded(YAML::Node const& yn) {
            YAML::Node result;
            for(auto const& p: yn) {
                if(!yaml_exclude.count(p.first.as<std::string>())) {
                    result[p.first.as<std::string>()] = p.second;
                }
            }
            return result;
        }

        dictOf<bool> try_load_files() override {
            dictOf <bool> bad_yaml_files;
            for(auto const& f: files) {
                try {
                    if(fs::last_write_time(fs::path(f)) != last_edited[f]) {
                        auto unfiltered = YAML::LoadFile(f);

                        last_versions[f] = filter_excluded(unfiltered);

                        last_edited[f] = fs::last_write_time(fs::path(f));
                    }
                }
                catch (std::exception& e) {
                    bad_yaml_files[f] = true;
                }
            }
            return bad_yaml_files;
        }


    };
}

#endif //ABTM2_AUTORELOADERWITHTESTS_H
