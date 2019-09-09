//
// Created by safoex on 05.09.19.
//

#include "core/exec/ExecutorInterface.h"
#include "core/exec/ABTM.h"
#include "core/memory/double/MemoryDouble.h"

#include "../test/testing.h"


int main(){

    using namespace abtm;

    CalculatorMemory memory;

    bool test_ok = true;
    ABTM tree(ABTM::Async, &memory);

    dictOf <std::string> root_node = {
            {"parent", ""},
            {"name", "root"},
            {"type", "sequence"}
    };
    auto s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT,  root_node));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "0.0: insert root (sequence)", s);

    dictOf<double> vars = {{"x",0},{"b",0},{"a",0}};
    memory.add(make_sample<double>(vars));

    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT,
        dictOf<std::string>({
                {"parent", "root"},
                {"name", "C1"},
                {"type", "condition"},
                {"expr", "x < b"},
                {"true_state", "SUCCESS"},
                {"false_state", "RUNNING"}
        })));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "0.1: insert C1 (condition)",s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT,
            dictOf<std::string>({
                  {"parent", "root"},
                  {"name", "A1"},
                  {"type", "action"},
                  {"expr", "x:= a+2"}
          })));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "0.2: insert A1 (action)",s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Execute, ExecutorInterface::START));

    test_ok = !s.count("x") && !s.count("a") && !s.count("b");
    TESTE(test_ok, "1.0: async mode, started",s);

    s = tree.execute({{"b", double(3)}});
//    UPDATED(memory, vars);

    test_ok = s.count("x") && from_sample<double>(s)["x"] == 2;
    TESTE(test_ok, "1.0: async mode, first sample",s);


    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::ERASE,
            dictOf<std::string>({
                    {"name", "C1"}
            })));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "2.0: massive change at run-time, ERASE", s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::REPLACE,
            dictOf<std::string>({
                    {"instead", "A1"},
                    {"name", "A3"},
                    {"type", "action"},
                    {"expr", "b:= 0"}
            })));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "2.1: massive change at run-time, REPLACE", s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT,
            dictOf<std::string>({
                    {"parent", "root"},
                    {"name", "par"},
                    {"type", "parallel"}
            })));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "2.2: massive change at run-time, INSERT 1", s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT,
            dictOf<std::string>({
                    {"parent", "par"},
                    {"name", "A2"},
                    {"type", "action"},
                    {"expr", "x:= b+3"}
            })));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "2.3: massive change at run-time, INSERT 2", s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT,
            dictOf<std::string>({
                    {"parent", "par"},
                    {"name", "C2"},
                    {"type", "condition"},
                    {"expr", "x > b + 2.5"},
                    {"true_state", "SUCCESS"},
                    {"false_state", "RUNNING"}
            })));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "2.4: massive change at run-time, INSERT 3", s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT,
            dictOf<std::string>({
                    {"parent", "root"},
                    {"name", "C3"},
                    {"type", "condition"},
                    {"expr", "x > a"},
                    {"true_state", "SUCCESS"},
                    {"false_state", "RUNNING"},
                    {"before", "A3"}
            })));

    test_ok = s.count(OK_WORD);
    TESTE(test_ok, "2.4: massive change at run-time, INSERT 4 (before)", s);

//    UPDATED(memory, vars);

    memory.set(make_sample<double>({{"x", 0}, {"__STATE__par", RUNNING}, {"__STATE__C2", RUNNING}}));

    s = tree.execute({{"a", -3.0}});
    auto output_stash = s;
    output_stash.insert(s.begin(), s.end());
    auto const& changes = output_stash;

    test_ok = changes.count("a") && from_sample<double >(changes)["a"] == -3;

//    UPDATED(memory, from_sample<double>(output_stash));

    TESTE(test_ok, "3.0: ticked changed tree", s);

    tree.execute(ABTM::pack(ExecutorInterface::Execute, ExecutorInterface::PAUSE));

    s = tree.execute({{"a", .0}, {"x", 2.0}});

//    UPDATED(memory, vars);

    test_ok = s.empty();

    TESTE(test_ok, "4.0: PAUSE", s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Execute, ExecutorInterface::START));
    s = tree.execute({});
    test_ok = s.count("x") && from_sample<double>(s)["x"] == 3;

//    UPDATED(memory, vars);

    TESTE(test_ok, "4.1: START (resume)", s);


}