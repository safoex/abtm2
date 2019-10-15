//
// Created by safoex on 06.09.19.
//

#ifndef ABTM2_IOINTERFACE_H
#define ABTM2_IOINTERFACE_H

#include "core/common.h"

#include <uuid/uuid.h>

namespace abtm {
    class IOInterface {
    public:
        sample required_vars, trigger_vars;
        bool time_to_die;
        IOInterface(sample const& required_vars, sample const& trigger_vars)
                : required_vars(required_vars), trigger_vars(trigger_vars) {
            time_to_die = false;
        };

        IOInterface() {}

        virtual sample process(sample const& output) {
            return {};
        };



        virtual ~IOInterface() = default;
    };
}

#endif //ABTM2_IOINTERFACE_H
