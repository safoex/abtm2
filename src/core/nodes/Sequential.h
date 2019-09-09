//
// Created by safoex on 04.09.19.
//

#ifndef ABTM2_SEQUENTIAL_H
#define ABTM2_SEQUENTIAL_H

#include "ControlInterface.h"

namespace abtm {
    class Sequential : public ControlInterface {
    public:

        // Sequence := Sequential(SUCCESS, ...)
        // Selector := Sequential(FAILURE, ...)
        // Skipper  := Sequential(RUNNING, ...)

        Sequential(NodeState return_state, std::string const& name, MemoryInterface& memory, bool deactivation = true) :
        ControlInterface(return_state, name, memory, deactivation) {}

        NodeState evaluate() override {
            NodeState result = return_state;

            for(auto child: children) {
                auto child_state = child->tick();
                if(child_state != return_state) {
                    result = child_state;
                    break;
                }
            }
            return result;
        }
    };
}

#endif //ABTM2_SEQUENTIAL_H
