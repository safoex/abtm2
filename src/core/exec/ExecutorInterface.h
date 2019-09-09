//
// Created by safoex on 05.09.19.
//

#ifndef ABTM2_EXECUTORINTERFACE_H
#define ABTM2_EXECUTORINTERFACE_H

#include <core/memory/MemoryInterface.h>
#include "core/common.h"
#include <tuple>

#define EXECUTOR_COMMAND_WORD "__EXECUTOR_COMMAND__"
#define COMMAND_TYPE_WORD "__COMMAND_TYPE__"
#define COMMAND_WORD "__COMMAND__"
#define DATA_WORD "__DATA__"

namespace abtm {
    class ExecutorInterface {
    public:
        enum CommandType {
            Execute,
            Modify
        };
        enum Execution {
            START,
            PAUSE,
            STOP,
            NOT_STARTED
        };
        enum Modification {
            INSERT,
            REPLACE,
            ERASE
        };
    protected:
        Execution state;
    public:
        typedef std::tuple<int, int, std::any, std::string> Command;
        MemoryInterface* memory;
        explicit ExecutorInterface(MemoryInterface* memory = nullptr) : memory(memory), state(NOT_STARTED) {}
        virtual sample execute(sample const& ) = 0;
        static sample pack(CommandType commandType, uint command, std::any const& data = std::any(), std::string const& ticket = "") {
            return {{EXECUTOR_COMMAND_WORD, Command{commandType, command, data, ticket}}};
        }
        static Command unpack(sample const& s) {
            if(s.size() == 1 && s.count((EXECUTOR_COMMAND_WORD))) {
                try {
                    return std::any_cast<Command>(s.at(EXECUTOR_COMMAND_WORD));
                }
                catch (std::bad_any_cast &e) {
                    return {-1, -1, std::any(), ""};
                }
            }
            return {-1, -1, std::any(), ""};
        }
        virtual ~ExecutorInterface() = default;
    };
}

#endif //ABTM2_EXECUTORINTERFACE_H
