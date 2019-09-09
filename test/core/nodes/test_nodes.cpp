//
// Created by safoex on 04.09.19.
//

#include "core/nodes/Nodes.h"
#include "core/memory/double/MemoryDouble.h"
#include "core/common.h"

#include "../test/testing.h"

int main() {

    using namespace abtm;

    bool test_ok = true;

    CalculatorMemory memory;

    Sequential root(SUCCESS, "root", memory);

    dictOf<double> vars = {{"x",0},{"b",0},{"a",0}};
    memory.add(make_sample<double>(vars));
    Leaf A1("A1", memory, memory.build_action("x:= a+2"));
    Leaf C1("C1", memory, memory.build_condtion("x = b"), SUCCESS, FAILURE);
    root.insert(&C1);
    root.insert(&A1);
    test_ok &= root.tick() == SUCCESS;
    TEST(test_ok, "1.0: tick()");
    auto changes = memory.changes();
    memory.flush();
    test_ok = !changes.count("a") && !changes.count("b");
    TEST(test_ok, "2.1: changes don't contain incorrect variables");
    test_ok = changes.count("x");
    TEST(test_ok, "2.2: changes contain correct variable");
    test_ok = from_sample<double>(changes)["x"] == 2;
    TEST(test_ok, "2.3: Sequence, Action, Condition, tick, changes");


    Parallel p(SUCCESS, "par", memory);
    Leaf C2("C2", memory, memory.build_condtion("x > b + 2.5"), SUCCESS, RUNNING);
    Leaf A2("A2", memory, memory.build_action("x:= b + 3"));
    Leaf A3("A3", memory, memory.build_action("b:= 0"));
    root.erase(&C1);
    root.insert(&p);
    root.replace(&A1, &A3);
    p.insert(&C2);
    p.insert(&A2);
    memory.set(make_sample<double>({{"a", -3}}));

    root.bfs_with_handler([](NodeInterface* n) {n->visited = false;});
    test_ok = root.tick() == RUNNING;

    root.bfs_with_handler([](NodeInterface* n) {n->visited = false;});
    test_ok &= root.tick() == SUCCESS;

    changes = memory.changes();
    memory.flush();
    test_ok &= !changes.count("b");
    test_ok &= changes.count("a") && from_sample<double >(changes)["a"] == -3;
    test_ok &= changes.count("x") && from_sample<double >(changes)["x"] == 3;
    TEST(test_ok, "3.0: run-time erase, replace, insert, Parallel");
    return 0;
}