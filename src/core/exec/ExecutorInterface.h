//
// Created by safoex on 05.09.19.
//

#ifndef ABTM2_EXECUTORINTERFACE_H
#define ABTM2_EXECUTORINTERFACE_H

#include <core/memory/MemoryInterface.h>
#include "core/common.h"

#define EXECUTOR_COMMAND "__EXECUTOR_COMMAND__"

namespace abtm {
    class ExecutorInterface {
    public:
        enum CommandType {
            Execute,
            Modify,
            Extra
        };
        enum Execution {
            START,
            PAUSE,
            STOP
        };
        enum Modification {
            INSERT,
            REPLACE,
            ERASE
        };
    protected:
        Execution state;
    public:
        typedef std::pair<std::pair<int, int>, std::any> Command;
        MemoryInterface* memory;
        ExecutorInterface(MemoryInterface* memory = nullptr) : memory(memory) {}
        virtual sample execute(sample const& ) = 0;
        static sample pack(CommandType commandType, uint command) {
            return {{EXECUTOR_COMMAND, Command{{commandType, command},std::any()}}};
        }
        static Command unpack(sample const& s) {
            if(s.size() == 1 && s.count((EXECUTOR_COMMAND))) {
                try {
                    return std::any_cast<Command>(s.at(EXECUTOR_COMMAND));
                }
                catch (std::bad_any_cast &e) {
                    return {{-1, -1},std::any()};
                }
            }
            return {{-1, -1},std::any()};
        }
        virtual ~ExecutorInterface() = default;
    };
}

#endif //ABTM2_EXECUTORINTERFACE_H
