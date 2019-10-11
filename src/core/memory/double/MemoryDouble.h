//
// Created by safoex on 03.09.19.
//

#ifndef ABTM2_MEMORYDOUBLE_H
#define ABTM2_MEMORYDOUBLE_H

#include "core/common.h"
#include "core/memory/MemoryInterface.h"
#include <mutex>
#include "SimpleCalculator.h"

namespace abtm {

    class CalculatorMemory : public MemoryInterface {

    protected:
        dictOf <double> m;
        sample changed;
        std::mutex mutex;
        std::string eq_sign_str;
    public:
        using MemoryInterface::update;
        CalculatorMemory() : MemoryInterface(), eq_sign_str(":=") {}

        void add(sample const& s) override {
            std::lock_guard lockGuard(mutex);
            for (auto const& [k,v]: s) {
                try {
                    if(!m.count(k))
                        m[k] = std::any_cast<double>(v);
                }
                catch(std::bad_any_cast &e) {
                    throw std::runtime_error("Error: type of value for key \"" + k + "\" is not a double");
                }
            }
        }
        void set(sample const& s) override {
            std::lock_guard lockGuard(mutex);
            for (auto const& [k,v]: s) {
                try {
                    auto new_v = std::any_cast<double>(v);
                    if(m[k] != new_v) {
                        m[k] = new_v;
                        changed[k] = m[k];
                    }
                }
                catch(std::bad_any_cast &e) {
                    throw std::runtime_error("Error: type of value for key \"" + k + "\" is not a double");
                }
            }
        };
        sample& update(sample& s) override {
            std::lock_guard lockGuard(mutex);
            for (auto const& [k,v]: s) {
                if(m.count(k))
                    s[k] = m.at(k);
            }
            return s;
        };

        void add_state(std::string const& state_var, NodeState init) override {
            add(make_sample<double>({{state_var, init}}));
        };

        NodeState get_state(std::string const& state_var) override {
            std::lock_guard lockGuard(mutex);
            return NodeState(m[state_var]);
        };

        void set_state(std::string const& state_var, NodeState ns) override {
            set({{state_var, double(ns)}});
        };

        sample changes() override {
            std::lock_guard lockGuard(mutex);
            return changed;
        };
        void flush() override {
            std::lock_guard lockGuard(mutex);
            changed.clear();
        };
        BFunction build_action(std::string const& expression) override {
            auto eq_sign = expression.find(eq_sign_str);
            std::string lvalue, rvalue;
            if(eq_sign != std::string::npos) {
                lvalue = expression.substr(0, eq_sign);
                rvalue = expression.substr(eq_sign + 2);
            }
            return [this, lvalue, rvalue]() -> bool {
                double new_value = calc::calc(rvalue, this->m);
                
                if(this->m[lvalue] != new_value)
                    changed[lvalue] = new_value;
                
                this->m[lvalue] = new_value;
                return true;
            };
        }
        BFunction build_condtion(std::string const& expression) override{
            if(expression == "default") {
                return []() -> bool {
                    return true;
                };
            }
            else if (expression.empty()) {
                return []() -> bool {
                    return false;
                };
            }
            else {
                return [this, expression]() -> bool {
                    return bool(calc::calc(expression, this->m));
                };
            }
        };

        bool test_expression(std::string const& expression) override{
            std::lock_guard lockGuard(mutex);
            auto eq_sign = expression.find(eq_sign_str);
            bool all_keys_present = true;
            if(eq_sign != std::string::npos) {
                all_keys_present = m.count(expression.substr(0, eq_sign));
            }
            // hack - npos == -1
            for(auto const& k: calc::find_vars(expression.substr(eq_sign + 1 + (eq_sign != std::string::npos)))) {
                all_keys_present &= m.count(k);
            }
            return all_keys_present;
        };

        keys used_vars(std::string const& expression) override {
            std::lock_guard lockGuard(mutex);
            return calc::find_vars(expression);
        }
    };
}

#endif //ABTM2_MEMORYDOUBLE_H
