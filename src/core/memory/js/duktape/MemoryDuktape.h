//
// Created by safoex on 04.09.19.
//

#ifndef ABTM2_MEMORYDUKTAPE_H
#define ABTM2_MEMORYDUKTAPE_H

#include "core/common.h"
#include "core/memory/MemoryInterface.h"
#include "core/memory/js/JSSymbolsFinder.h"
#include <duktape.h>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <iomanip>

#define MEM_DUKTAPE_DEBUG false

namespace abtm {

    std::string get_from_any(std::any const& a) {
        std::string s;
        try {
            s = std::any_cast<std::string>(a);
        }
        catch(std::bad_any_cast &e) {
            try {
                s = std::any_cast<const char*>(a);
            }
            catch(std::bad_any_cast &e) {
                try{
                    std::stringstream ss;
                    ss << std::fixed;
                    ss << std::setprecision(9);
                    ss << std::any_cast<double>(a);
                    s = ss.str();
                }
                catch (std::bad_any_cast &e) {
                    throw std::runtime_error("Error: argument get_from_any() is not a double, string or const char*");
                }
            }
        }
        return s;
    }

    class MemoryDuktape : public MemoryInterface {
    protected:
        duk_context* ctx;
        std::mutex mutex;
        keys vars;
        void eval_no_lock(std::string const& eval_str, std::string const& error_str = "") {
            duk_push_string(ctx, eval_str.c_str());
            if(MEM_DUKTAPE_DEBUG)
                std::cout << eval_str << std::endl;
            if(duk_peval(ctx) != 0) {
                if(error_str.empty())
                    throw std::runtime_error("Error! Bad expression: " + eval_str);
            }
        }
        void eval(std::string const& eval_str, std::string const& error_str = "", bool _lock = true) {
            if(_lock) {
                std::lock_guard lock(mutex);
                eval_no_lock(eval_str, error_str);
            }
            else eval_no_lock(eval_str, error_str);
        }

        void clear_duk_stack() {
            while(duk_get_top(ctx) > 1000) {
                duk_pop(ctx);
            }
        }

        std::any get_var(std::string const& key, bool lock = true) {
            std::string cmd = R"(JSON.stringify(get_var(")" + key + R"(")))";
            if(lock) {
                std::lock_guard lockGuard(mutex);
                eval(cmd, "", false);
                return std::string(duk_get_string(ctx, -1));
            }
            else {
                eval(cmd, "", false);
                return std::string(duk_get_string(ctx, -1));
            }
        }

    public:
        using MemoryInterface::update;

        MemoryDuktape() : MemoryInterface()
        {
            ctx = duk_create_heap_default();
        }

        void import(std::string const& script) {
            eval(script);
        }

        void add(sample const & s) override {
            std::lock_guard lockGuard(mutex);
            for(auto const& [k,v]: s) {
                std::stringstream cmd;
                std::string init_str = get_from_any(v);
                cmd << "add('" << k << "\', " << init_str << ");";
                eval_no_lock(cmd.str());
                cmd.clear();
                vars.insert(k);
            }

            if(MEM_DUKTAPE_DEBUG && false) {
                eval("log_window();", "", false);
                std::string log = duk_get_string(ctx, -1);

                std::cout << "WINDOW: " << log << std::endl;
            }
        };

        void set(sample const & s) override {
            std::lock_guard lockGuard(mutex);

            for(auto const& [k,v]: s) {
                std::string json_view = get_from_any(v);
                eval_no_lock(k + " = " + json_view + ";");

            }
            clear_duk_stack();
        };

        sample& update(sample &s) override {
            std::lock_guard lockGuard(mutex);
            for(auto &[k,v]: s) {
                if(vars.count(k))
                    s[k] = get_var(k, false);
            }
            clear_duk_stack();
            return s;
        };

        void add_state(std::string const& state_var, NodeState init) override {
            add(make_sample<std::string>({{state_var, std::to_string(init)}}));
        };

        NodeState get_state(std::string const& state_var) override {
            std::lock_guard lockGuard(mutex);
            std::string cmd = R"(get_var(")" + state_var + R"("))";
            eval(cmd, "", false);
            return NodeState(duk_get_int(ctx, -1));
        }

        void set_state(std::string const& state_var, NodeState ns) override {
            eval(state_var + " = " + std::to_string(int(ns)));
        };

        std::string dump_memory() {
            std::lock_guard lockGuard(mutex);
            eval("log_window();", "", false);
            std::string log = duk_get_string(ctx, -1);
            clear_duk_stack();
            return log;
        }

        sample changes() override {
            std::lock_guard lockGuard(mutex);
            eval("poll_changes();", "", false);
            eval("get_changes();", "", false);
            rapidjson::Document d;
            std::string changes = duk_get_string(ctx, -1);
            if(MEM_DUKTAPE_DEBUG)
                std::cout << "CHANGES : " << changes<< std::endl;
            d.Parse(changes.c_str());
            sample result;
            for(auto k = d.MemberBegin(); k != d.MemberEnd(); ++k) {
                std::string var = k->name.GetString(), val =  k->value.GetString();
                result[var] = val;
            }
            if(MEM_DUKTAPE_DEBUG && false) {
                eval("log_window();", "", false);
                std::string log = duk_get_string(ctx, -1);

                std::cout << "WINDOW: " << log << std::endl;
            }
            clear_duk_stack();
            return result;
        };

        void flush() override {
            eval("flush()");
        };

        BFunction build_action(std::string const &expression) override {
            return [this, expression]() -> bool {
                this->eval(expression, "Error: runtime_error in " + expression);
                return true;
            };
        };

        BFunction build_condtion(std::string const &expression) override {
            auto new_expression = "(" + expression + ") != false";
            return [this, new_expression]() -> bool {
                std::lock_guard lockGuard(this->mutex);
                eval(new_expression, "Error: runtime_error in " + new_expression, false);
                return (bool)duk_get_boolean(ctx, -1);;
            };
        };

        bool test_expression(std::string const &expression) override {
            std::lock_guard lockGuard(mutex);
            duk_push_string(ctx, expression.c_str());
            bool test = duk_peval(ctx) == 0;
            eval("poll_changes();", "", false);
            eval("restore_changes();", "", false);
            return test;
        };
        
        keys used_vars(std::string const& expression) override {
            std::lock_guard lockGuard(mutex);
            return abtm_js::get_used_vars_from_expr(expression);
        }

        ~MemoryDuktape() override {
            duk_destroy_heap(ctx);
        }
    };
}

#endif //ABTM2_MEMORYDUKTAPE_H
