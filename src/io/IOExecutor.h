//
// Created by safoex on 03.10.19.
//

#ifndef ABTM2_IOEXECUTOR_H
#define ABTM2_IOEXECUTOR_H

#include "IOInterface.h"
#include "core/exec/ExecutorInterface.h"

namespace abtm {
    class IOExecutor : public IOInterface {
    public:
        ExecutorInterface* executor;

        IOExecutor(ExecutorInterface* executor) : IOInterface(ALL_CHANGED, ON_EVERY), executor(executor) {}

        sample process(sample const& s) {
            return executor->execute(s);
        }
    };
}


#endif //ABTM2_IOEXECUTOR_H
