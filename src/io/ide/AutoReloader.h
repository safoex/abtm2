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
    using std::chrono_literals::operator""ms;

    template<class Var, class Mem>
    void import_file_to(std::string const& filename, abtm::LoadFabric<Var,Mem> & lf) {
        std::ifstream js_file(filename);
        std::string duktape_js_script   ( (std::istreambuf_iterator<char>(js_file) ),
                                          (std::istreambuf_iterator<char>()    ) );
        lf.memory->import(duktape_js_script);
    }

    class AutoReloader : public IOInterface {
    protected:
        int check_every_milliseconds;
        dictOf<fs::file_time_type> last_edited;
        dictOf<YAML::Node> last_versions;
        int tick_every_milliseconds;
        bool had_exception_on_load;
    public:
        std::list<std::string> files, mem_scripts;
        LoadFabric<std::string, MemoryDuktape>* lf;
        std::string main_file;

        bool resetHard;
        bool first_load;
        bool reload_mem;
        bool main_changed;
        std::mutex mutex;
        RosbridgeWsClient rbc;


        AutoReloader(LoadFabric<std::string, MemoryDuktape>* lf, std::string const& rosbridge_port = "localhost:9090",
                int check_every_milliseconds = 1, int tick_every_milliseconds = 0) :
                check_every_milliseconds(check_every_milliseconds), lf(lf), tick_every_milliseconds(tick_every_milliseconds),
                rbc(rosbridge_port) {

            trigger_vars.insert({EXCEPTION_WORD, nullptr});
            trigger_vars.insert({ROS_VAR_CHANGES_REQUEST_WORD, nullptr});
            trigger_vars.insert({ROS_CHANGE_FILE, nullptr});
            required_vars = ALL_CHANGED;
            first_load = true;
            main_changed = true;
            rbc.addClient("autoreloader");
            rbc.addClient("autoreloader3");
            rbc.advertise("autoreloader", "/abtm/exception", "std_msgs/String");
            rbc.advertise("autoreloader3", "/abtm/main_file", "std_msgs/String");
            std::this_thread::sleep_for(50ms);
            rbc.addClient("autoreloader2");
            rbc.subscribe("autoreloader2", "/abtm/main_file", [this](
                    std::shared_ptr<WsClient::Connection> /* connection */,
                    std::shared_ptr<WsClient::Message> message)
                {
                    std::lock_guard lockGuard(mutex);

                    rapidjson::Document d;
                    std::string msg(message->string());
                    d.Parse(msg.c_str());

                    auto new_main_file = d["msg"]["data"].GetString();
                    if(new_main_file != main_file) {
                        std::cout << main_file << std::endl;
                        main_changed = true;
                        last_edited.clear();
                        main_file = new_main_file;
                    }
                });
        }

        sample process(sample const& s) override {

            if(!get_exception(s).empty()) {
                std::cout << get_exception(s);
                had_exception_on_load = true;
                return {};
            }
            else if(s.count(ROS_VAR_CHANGES_REQUEST_WORD)) {
                return {{ROS_VAR_CHANGES_WORD, "{\"data\": \"" + string_utils::escape_json(lf->get_memory()) + "\"}"}};
            }
            else if(s.count(ROS_CHANGE_FILE)) {
                std::lock_guard lockGuard(mutex);
                auto msg = std::any_cast<std::string>(ROS_CHANGE_FILE);
                rapidjson::Document d;
                d.Parse(msg.c_str());
                if(d["data"].IsString()) {
                    std::string path = d["data"].GetString();
                    reset_files({path});
                }
            }
            return {};
//            return check_and_reload();
        }

        void spinOnce() {
            std::lock_guard lockGuard(mutex);
            check_and_reload();
        }

        void reset_files(std::list<std::string> new_files) {
            files = std::move(new_files);
        }

        void spinForever() {
            auto last_time = std::chrono::system_clock::now();
            while(true) {
                std::this_thread::sleep_for(std::chrono::milliseconds((long)check_every_milliseconds));
                auto current_time = std::chrono::system_clock::now();
                try {
                    spinOnce();
                }
                catch(std::exception &e) {
                    std::cout << e.what() << std::endl;
                }
                if(tick_every_milliseconds > 0)
                    if(current_time - last_time > std::chrono::milliseconds(tick_every_milliseconds)) {
                        last_time = current_time;
                        if(lf)
                            lf->io->process(TICK_SAMPLE, IO_OUTPUT, &lf->io->ioExecutor);
                    }
            }
        }

        void check_if_files_changed(bool& reload, bool& any_error) {
            for(auto const& f: files) {
                try {
                    if (fs::last_write_time(fs::path(f)) != last_edited[f]) {
                        reload = true;
                        break;
                    }
                }
                catch (fs::filesystem_error& e) {
                    any_error = true;
                    break;
                }
            }
        }

        bool check_if_file_changed(std::string const& f, bool& any_error) {
            try {
                return fs::last_write_time(fs::path(f)) != last_edited[f];
            }
            catch (fs::filesystem_error& e) {
                any_error = true;
                return false;
            }
        }

        dictOf <bool> try_load_files() {
            dictOf <bool> bad_yaml_files;
            for(auto const& f: files) {
                try {
                    if(fs::last_write_time(fs::path(f)) != last_edited[f]) {
                        last_versions[f] = YAML::LoadFile(f);
                        last_edited[f] = fs::last_write_time(fs::path(f));
                    }
                }
                catch (std::exception& e) {
                    bad_yaml_files[f] = true;
                }
            }
            return bad_yaml_files;
        }

        sample make_bad_yaml_exception(dictOf<bool> const& bad_files) {
            std::string ex ("broken .yaml files: ");
            int bad_files_left = bad_files.size();
            for(auto const& [k,v]: bad_files) {
                if(v) {
                    ex += k;
                    if(bad_files_left > 1)
                        ex += ", ";
                    bad_files_left--;
                }
            }
            return make_exception(ex);
        }

        void reload_memory() {
            if(reload_mem) {
                if(lf && lf->executor)
                    lf->executor->execute(ABTM::pack(ABTM::Execute, ABTM::STOP));
                delete lf->memory;
                std::cout << "deleted memory" << std::endl;
                lf->memory = new MemoryDuktape;
                std::cout << "created new memory" << std::endl;
                for(auto s: mem_scripts)
                    import_file_to(s, *lf);
            }
        }

        void get_import(YAML::Node const& mf) {
            reload_mem = true;
            mem_scripts.clear();
            if(mf["import"]) {
                YAML::Node f = mf["import"];
                if(f["memory"]) {
                    if(f["memory"]["reload"]) {
                        reload_mem = f["memory"]["reload"].as<std::string>() == "yes";
                    }
                    for(auto const& v: f["memory"]["scripts"]) {
                        mem_scripts.push_back(v.as<std::string>());
                    }
                }
                if(f["yaml"] || f["files"]) {
                    auto fs = f["yaml"] ? f["yaml"] : f["files"];

                    std::list<std::string> new_files;
                    for(auto const& new_f: fs)
                        new_files.push_back(new_f.as<std::string>() == "this" ? main_file : new_f.as<std::string>());

                    bool different = files.size() != new_files.size();
                    if(!different)
                        for(auto old_it = files.begin(), new_it = new_files.begin();
                            old_it != files.end() && new_it != new_files.end();
                            old_it++, new_it++) {
                            if(*old_it != *new_it) {
                                different = true;
                            }
                        }

                    if(different) {
                        files = new_files;
                        last_edited.clear();
                    }
                }
            }
            else {
                files.clear();
                files.push_back(main_file);
            }
        }

        sample check_and_reload() {
            bool reload = false;
            bool skip_this_cycle = false;

            if(main_file.empty()) {
                skip_this_cycle = true;
            }
            else {
                try {
                    YAML::LoadFile(main_file);
                }
                catch (std::exception& e) {
                    std::string ex = "bad main file \"" + main_file + "\"";
                    std::cout << ex  << std::endl;
                    rapidjson::Document d;
                    d.Parse(("{\"data\": \"" + string_utils::escape_json(ex) + "\"}").c_str());
                    rbc.publish("/abtm/exception", d);
                }

//                std::cout << "tried to load main_file" << std::endl;
                main_changed |= check_if_file_changed(main_file, skip_this_cycle);
            }

            if(skip_this_cycle) {
                main_changed = true;
                return {};
            }

            if(main_changed) {
                get_import(YAML::LoadFile(main_file));

                main_changed = false;
            }

            check_if_files_changed(reload, skip_this_cycle);
            if(reload && !skip_this_cycle) {
                dictOf <bool> bad_yaml_files = try_load_files();

                if(!bad_yaml_files.empty()) {
                    return make_bad_yaml_exception(bad_yaml_files);
                }
                reset();

                load_and_build();
            }
            return {};
        }

        void load_and_build() {
            had_exception_on_load = false;
            for(auto const& f: files) {
                load_yaml(YAML::Clone(last_versions[f]));
            }
            // hack delay because ~~ ROS does not load quickly
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if(!had_exception_on_load) {
                build_tree();
            }
        }

        void reset() {
            lf->io->unregisterIOchannel(this);
            std::cout << "unregistered this" << std::endl;
            if(lf->initialized)
                lf->deleteSoft();
            std::cout << "deleted lf" << std::endl;
            reload_memory();
            std::cout << "reloaded memory" << std::endl;
            lf->resetSoft();
            std::cout << "reseted lf" << std::endl;
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
