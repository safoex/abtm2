//
// Created by safoex on 03.09.19.
//
#include "core/memory/MemoryInterface.h"
#include "core/memory/double/MemoryDouble.h"

#define TEST(ok, name) std::cout << "TEST " << name << (ok ? " PASSED" : " FAILED") << std::endl


int main() {
    using namespace abtm;


    CalculatorMemory memory;
    auto new_vars = make_sample<double>({{"a", 0}, {"b", 1}, {"c", 2}});

    memory.add(new_vars);
    memory.set(make_sample<double>({{"a", 3}}));
    auto ch1 = memory.changes();
    auto ch2 = memory.changes();
    memory.flush();
    auto ch3 = memory.changes();
    bool test_ok = ch1.size() == 1 && ch1.count("a") && std::any_cast<double>(ch1.at("a")) == 3;
    test_ok &= ch2.size() == 1 && ch2.count("a") && std::any_cast<double>(ch2.at("a")) == 3;
    test_ok &= ch3.empty();
    TEST(test_ok, ".add, .set, .get_changes, .flush");

    test_ok &= !memory.test_expression("x & y");
    test_ok &= memory.test_expression("a | b & c");
    test_ok &= memory.test_expression("a:= b + c");
    test_ok &= !memory.test_expression("x:= b * c & d");
    TEST(test_ok, ".test_expression");

    auto A1 = memory.build_action("b:= c+ a");
    auto C1 = memory.build_condtion("c & a");

    auto expected = memory.update<double>({{"a",0.0},{"c",0.0}});
    A1();
    expected["b"] = memory.update<double>({{"b",0}})["b"];
    test_ok &= expected["b"] == expected["a"] + expected["c"];
    test_ok &= C1() == ( bool(expected["a"]) && bool(expected["c"]));
    memory.set({{"a",0.0}});
    memory.update<double>(expected);
    test_ok &= C1() == ( bool(expected["a"]) && bool(expected["c"]));
    TEST(test_ok, "memory Action and Condition");

    memory.add_state("__STATE__A", NodeState::UNDEFINED);
    memory.set_state("__STATE__A", NodeState::RUNNING);
    test_ok &= memory.get_state("__STATE__A") == NodeState::RUNNING;
    TEST(test_ok, "get_state & set_state");
    
    auto uv = memory.used_vars("A & B | C = D");
    test_ok &= uv.size() == 4;
    test_ok &= uv.count("A");
    test_ok &= uv.count("B");
    test_ok &= uv.count("C");
    test_ok &= uv.count("D");
    TEST(test_ok, ".used_vars()");
    return 0;
}