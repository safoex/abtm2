//
// Created by safoex on 03.09.19.
//

#ifndef ABTM2_MEMORYINTERFACE_H
#define ABTM2_MEMORYINTERFACE_H

#include "core/common.h"

namespace abtm {
    class MemoryInterface {
    public:
        MemoryInterface() = default;
        virtual void add(sample const&) = 0;
        virtual void set(sample const&) = 0;
        virtual void add_state(std::string const& state_var, NodeState init) = 0;
        virtual NodeState get_state(std::string const& state_var) = 0;
        virtual void set_state(std::string const& state_var, NodeState ns) = 0;
        virtual sample& update(sample&) = 0;
        virtual sample changes() = 0;
        virtual void flush() = 0;
        virtual BFunction build_action(std::string const& expression) = 0;
        virtual BFunction build_condtion(std::string const& expression) = 0;
        virtual bool test_expression(std::string const& expression) = 0;
        virtual ~MemoryInterface() = default;

        virtual sample update(sample const& s) {
            sample x(s);
            return update(x);
        };
        template<typename T> dictOf<T> update(dictOf<T> const& d) {
            return from_sample<T>(update(make_sample<T>(d)));
        }
        template<typename T> dictOf<T>& update(dictOf<T>& d) {
            return (d=update((dictOf<T>const&)d));
        }
    };
}

#endif //ABTM2_MEMORYINTERFACE_H
