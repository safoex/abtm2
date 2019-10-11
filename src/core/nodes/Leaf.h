//
// Created by safoex on 04.09.19.
//

#ifndef ABTM2_LEAF_H
#define ABTM2_LEAF_H

#include "core/common.h"
#include "NodeInterface.h"

#define LEAF_DEBUG true

namespace abtm {
    class Leaf : public NodeInterface {
    protected:
        NodeState trueState, falseState;
        BFunction leaf_function;
    public:
        Leaf(std::string const& name, MemoryInterface& memory, BFunction const& leaf_function,
                NodeState trueState = SUCCESS, NodeState falseState = SUCCESS, bool deactivation = true) :
                NodeInterface(name, memory, deactivation), leaf_function(leaf_function),
                trueState(trueState), falseState(falseState) {}

        NodeState tick() override {
            NodeState result = evaluate();

            memory.set_state(state_var(), result);

            return result;
        }

        NodeState evaluate() override {
            return leaf_function() ? trueState : falseState;
        }

        void deactivate() override {}

        ~Leaf() override = default;
    };
}

#endif //ABTM2_LEAF_H
