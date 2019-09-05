//
// Created by safoex on 03.09.19.
//
#include "core/memory/MemoryInterface.h"
#include "core/memory/js/duktape/MemoryDuktape.h"

#define TEST(ok, name) std::cout << "TEST " << name << (ok ? " PASSED" : " FAILED") << std::endl


int main() {
    using namespace abtm;

    MemoryDuktape memory;
    std::ifstream duktape_js_file("../src/core/memory/js/duktape/memory_duktape.js");
    std::string duktape_js_script   ( (std::istreambuf_iterator<char>(duktape_js_file) ),
                                    (std::istreambuf_iterator<char>()    ) );
    memory.import(duktape_js_script);
    auto new_vars = make_sample<std::string>({{"a", "0"}, {"b", "1"}, {"c", "2"}});

    memory.add(new_vars);
    memory.set(make_sample<std::string>({{"a", "3"}}));
    auto ch1 = memory.changes();
    auto ch2 = memory.changes();
    memory.flush();
    auto ch3 = memory.changes();
    bool test_ok = ch1.size() == 1 && ch1.count("a") && std::any_cast<std::string>(ch1.at("a")) == "3";
    test_ok &= ch2.size() == 1 && ch2.count("a") && std::any_cast<std::string>(ch2.at("a")) == "3";
    test_ok &= ch3.empty();
    TEST(test_ok, ".add, .set, .get_changes, .flush");

    test_ok &= !memory.test_expression("x && y");
    test_ok &= memory.test_expression("a || b && c");
    test_ok &= memory.test_expression("a = b + c");
    test_ok &= !memory.test_expression("x= b * c & d");
    TEST(test_ok, ".test_expression");

    auto A1 = memory.build_action("b = c+ a");
    auto C1 = memory.build_condtion("a == c");

    auto expected = memory.update<std::string>({{"a","0"},{"c","0"}});
    A1();
    expected["b"] = memory.update<std::string>({{"b","0"}})["b"];
    test_ok &= expected["b"] == "5";
    TEST(test_ok, "memory Action");
    test_ok &= !C1();
    memory.set({{"a","c"}});
    memory.update<std::string>(expected);
    test_ok &= C1();
    TEST(test_ok, "memory Condition");

    auto C2 = memory.build_condtion("a && c");
    test_ok &= C2();
    memory.set(make_sample<std::string>({{"a", "0"}}));
    test_ok &= !C2();
    TEST(test_ok, "strange condition");

    memory.add_state("__STATE__A", NodeState::UNDEFINED);
    memory.set_state("__STATE__A", NodeState::RUNNING);
    test_ok &= memory.get_state("__STATE__A") == NodeState::RUNNING;
    TEST(test_ok, "get_state & set_state");

    auto uv = memory.used_vars("A && B || C + D.a");
    test_ok &= uv.size() == 4;
    test_ok &= uv.count("A");
    test_ok &= uv.count("B");
    test_ok &= uv.count("C");
    test_ok &= uv.count("D");
    TEST(test_ok, ".used_vars()");
    return 0;
}