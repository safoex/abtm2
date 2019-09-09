//
// Created by safoex on 04.09.19.
//

#ifndef ABTM2_NODEINTERFACE_H
#define ABTM2_NODEINTERFACE_H

#include "core/common.h"
#include <list>
#include "core/memory/MemoryInterface.h"

namespace abtm {
    class NodeInterface {
    protected:
        std::string _id, _state_var;
        MemoryInterface& memory;
    public:
        typedef std::function<void(NodeInterface*)> Handler;
        bool visited, deactivation;
        std::any info;
        std::list<NodeInterface*> children;


        inline std::string const& id() const {
            return _id;
        };

        inline std::string const& state_var() const {
            return _state_var;
        };

        NodeState state() {
            return memory.get_state(state_var());
        }

        virtual NodeState tick() = 0;

        virtual NodeState evaluate() = 0;

        virtual void deactivate() = 0;

        void bfs_with_handler(Handler const& h) {
            std::list<NodeInterface*> current{this}, next{};
            while(!current.empty()) {
                for(auto node: current) {
                    h(node);
                    next.insert(next.end(), node->children.begin(), node->children.end());
                }
                current = std::move(next);
                next.clear();
            }
        }

        void dfs_with_handler(Handler const& h) {
            h(this);
            for(auto child: children) {
                child->dfs_with_handler(h);
            }
        }

        NodeInterface(std::string const& name, MemoryInterface& memory, bool deactivation = true) : _id(name), memory(memory), visited(false),
        _state_var("__STATE__" + name), deactivation(deactivation) {
            memory.add_state(state_var(), NodeState::UNDEFINED);
        };


        virtual ~NodeInterface() = default;
    };
}

#endif //ABTM2_NODEINTERFACE_H
