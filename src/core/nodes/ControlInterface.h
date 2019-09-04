//
// Created by safoex on 04.09.19.
//

#ifndef ABTM2_CONTROLINTERFACE_H
#define ABTM2_CONTROLINTERFACE_H

#include "NodeInterface.h"

namespace abtm {
    class ControlInterface : public NodeInterface {
    protected:
        NodeState return_state;
    public:
        ControlInterface(NodeState return_state, std::string const& name, MemoryInterface& memory) : NodeInterface(name, memory),
        return_state(return_state) {
        }

        NodeInterface* find(std::string const& name) {
            for(auto const& child: children) {
                if(child->id() == name)
                    return child;
            }
            return nullptr;
        }

        void insert(NodeInterface* node, NodeInterface* next = nullptr) {
            // if next == nullptr -> to the end
            // else before it
            children.insert(std::find(children.begin(), children.end(), node), node);
        };

        void replace(NodeInterface* from, NodeInterface* to) {
            auto from_pos = std::find(children.begin(), children.end(), from);
            auto next_pos = from_pos;
            next_pos++;
            children.insert(next_pos, to);
            children.erase(from_pos);
        }


        void erase(NodeInterface* node) {
            auto pos = std::find(children.begin(), children.end(), node);
            if(pos != children.end())
                children.erase(pos);
        }

        NodeState tick() override {
            if(visited)
                return state();

            NodeState result = evaluate();

            memory.set_state(state_var(), result);
            if(result != return_state) {
                deactivate();
            }

            visited = true;

            return result;
        }
        void deactivate() override {
            if(state() != UNDEFINED) {
                for (auto node: children) {
                    node->deactivate();
                }
                memory.set_state(state_var(), UNDEFINED);
            }
        }

        ~ControlInterface() override = default;
    };
}

#endif //ABTM2_CONTROLINTERFACE_H
