//
// Created by safoex on 07.10.19.
//

#ifndef ABTM2_AUTORELOADER_H
#define ABTM2_AUTORELOADER_H

#include "io/load/LoadFabric.h"
#include <experimental/filesystem>

#define AUTORELOAD_DEBUG false

namespace abtm {
    namespace fs = std::experimental::filesystem;
    class AutoReloader : public IOInterface {
    protected:
        int check_every_milliseconds;
        dictOf<fs::file_time_type> last_edited;
        dictOf<YAML::Node> last_versions;
        int tick_every_milliseconds;
        bool had_exception_on_load;
    public:
        std::list<std::string> files;
        LoadFabricInterface* lf;
        bool resetHard;


        AutoReloader(LoadFabricInterface* lf, bool resetHard = true, int check_every_milliseconds = 1, int tick_every_milliseconds = 0) :
                check_every_milliseconds(check_every_milliseconds), lf(lf), resetHard(resetHard),tick_every_milliseconds(tick_every_milliseconds) {
            if(check_every_milliseconds < 0 && false) {
                // && false --> bug with self locking within call of io->process inside .process function
                trigger_vars = RELOAD_COMMAND;
                required_vars = RELOAD_COMMAND;
            }
            trigger_vars.insert({EXCEPTION_WORD, nullptr});
            required_vars.insert({EXCEPTION_WORD, nullptr});
        }

        sample process(sample const& s) override {
            if(!get_exception(s).empty()) {
                std::cout << get_exception(s);
                had_exception_on_load = true;
            }
            return {};
//            return check_and_reload();
        }

        void spinOnce() {
            check_and_reload();
        }

        void spinForever() {
            auto last_time = std::chrono::system_clock::now();
            while(true) {
                std::this_thread::sleep_for(std::chrono::milliseconds((long)check_every_milliseconds));
                auto current_time = std::chrono::system_clock::now();
                check_and_reload();
                if(tick_every_milliseconds > 0)
                    if(current_time - last_time > std::chrono::milliseconds(tick_every_milliseconds)) {
                        last_time = current_time;
                        lf->io->process(TICK_SAMPLE, IO_OUTPUT, &lf->io->ioExecutor);
                    }
            }
        }

        sample check_and_reload() {
            bool reload = false;
            bool skip_this_cycle = false;
            for(auto const& f: files) {
                try {
                    if(AUTORELOAD_DEBUG) {
                        auto ftime = fs::last_write_time(fs::path(f));
                        auto last_time = last_edited[f];
                        auto ft = decltype(ftime)::clock::to_time_t(ftime),
                                lt = decltype(ftime)::clock::to_time_t(last_time);

                        std::cout << f << '\n' << std::asctime(std::localtime(&ft)) << std::asctime(std::localtime(&lt))
                                  << std::endl;
                    }
                    if (fs::last_write_time(fs::path(f)) != last_edited[f]) {
                        if(AUTORELOAD_DEBUG)
                            std::cout << f << std::endl;
                        reload = true;
                        break;
                    }
                }
                catch (fs::filesystem_error& e) {
                    skip_this_cycle = true;
                    break;
                }
            }
            if(reload && !skip_this_cycle) {
                dictOf <bool> bad_yaml_files;

                int bad_files = 0;
                for(auto const& f: files) {
                    bad_yaml_files[f] = false;
                    try {
                        if(fs::last_write_time(fs::path(f)) != last_edited[f]) {
                            last_versions[f] = YAML::LoadFile(f);
                            last_edited[f] = fs::last_write_time(fs::path(f));
                        }
                    }
                    catch (std::exception& e) {
                        bad_yaml_files[f] = true;
                        bad_files ++;
                        if(AUTORELOAD_DEBUG)
                            std::cout << "bad file caught" << std::endl;
                    }
                }
                if(bad_files > 0) {
                    std::string ex ("broken .yaml files: ");
                    int bad_files_left = bad_files;
                    for(auto const& [k,v]: bad_yaml_files) {
                        if(v) {
                            ex += k;
                            if(bad_files_left > 1)
                                ex += ", ";
                            bad_files_left--;
                        }
                    }
                    return make_exception(ex);
                }
                else {
                    reset();
                    // hack delay because ~~ ROS does not load quickly
                    using std::chrono_literals::operator""ms;
                    std::this_thread::sleep_for(50ms);
                    had_exception_on_load = false;
                    for(auto const& f: files) {
                        if(AUTORELOAD_DEBUG)
                            std::cout << last_versions[f];
                        load_yaml(YAML::Clone(last_versions[f]));
                    }
                    if(!had_exception_on_load)
                        build_tree();
                }
            }
            return {};
        }

        void reset() {
            lf->io->unregisterIOchannel(this);
            if(resetHard)
                lf->resetHard();
            else
                lf->resetSoft();
            lf->io->registerIOchannel(this);
        }

        void load_yaml(YAML::Node const& f) {
            if(AUTORELOAD_DEBUG)
                std::cout << "reloaded yaml file" << std::endl << std::endl << f << std::endl;
            lf->io->process({{TICKET_WORD, std::string()},
                             {LOAD_WORD,   f},
                             {FORMAT_WORD, YAML_WORD}}, IO_INPUT, nullptr);
        }

        void build_tree() {
            lf->io->process({{BUILD_TREE_WORD, std::string("")},
                            {TICKET_WORD,     std::string("")}}, IO_INPUT, nullptr);
        }


    };
}

#endif //ABTM2_AUTORELOADER_H
