//
// Created by safoex on 04.09.19.
//

#ifndef ABTM2_PARALLEL_H
#define ABTM2_PARALLEL_H

#include "ControlInterface.h"
namespace abtm {
    class Parallel : public ControlInterface {
    protected:
        NodeState break_state;
    public:

        // (Classical)  Parallel := Parallel(SUCCESS, ...)
        // At Least One Parallel := Parallel(FAILURE, ...)

        Parallel(NodeState break_state, std::string const& name, MemoryInterface& memory) : ControlInterface(RUNNING, name, memory),
                break_state(break_state) {}

        NodeState evaluate() override {

            std::vector<NodeState> children_states(children.size(), return_state);

            int i = 0;
            for(auto child: children) {
                children_states[i++] = child->tick();
            }

            auto result = NodeState(SUCCESS + FAILURE - break_state);
            for(auto child_state: children_states) {
                if(child_state == break_state) {
                    result = child_state;
                    break;
                }
                else if(child_state == return_state) {
                    result = return_state;
                    break;
                }
            }

            return result;
        }
    };
}


#endif //ABTM2_PARALLEL_H
