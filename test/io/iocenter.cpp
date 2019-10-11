//
// Created by safoex on 06.09.19.
//

#include "io/IOCenter.h"
#include "io/offline/OfflineTests.h"
#include "core/exec/ABTM.h"
#include "core/memory/double/MemoryDouble.h"
#include <fstream>

#include "../test/testing.h"

int main () {
    using namespace abtm;

    CalculatorMemory memory;

    bool test_ok = true;
    ABTM tree(ABTM::Classic, &memory);

    dictOf <std::string> root_node = {
            {"parent", ""},
            {"name", "root"},
            {"type", "sequence"}
    };
    auto s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT, root_node));

    test_ok &= ok_response(s);
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

    test_ok &= ok_response(s);
    TESTE(test_ok, "0.1: insert C1 (condition)",s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Modify, ExecutorInterface::INSERT,
                                dictOf<std::string>({
                                                            {"parent", "root"},
                                                            {"name", "A1"},
                                                            {"type", "action"},
                                                            {"expr", "x:= a+2"}
                                                    })));

    test_ok &= ok_response(s);
    TESTE(test_ok, "0.2: insert A1 (action)",s);

    s = tree.execute(ABTM::pack(ExecutorInterface::Execute, ExecutorInterface::START));
    s = tree.execute(TICK_SAMPLE);


    test_ok &= !s.count("x") && !s.count("a") && !s.count("b");
    TESTE(test_ok, "1.0: classic mode, first .tick()",s);

//    UPDATED(memory, vars);

    IOCenter io(&tree);

    OfflineTests<double> ot(&io, TICK_SAMPLE);

    std::string inputs("../test/io/input.yaml"),
                outputs("../test/io/output.yaml");
    std::ifstream offline_input(inputs);
    std::ofstream offline_output(outputs);
    ot.apply_tests(offline_input, offline_output);

    offline_output.close();

    std::ifstream offline_results(outputs);
    auto node = YAML::Load(offline_results);
    test_ok = node.size() == 3;
    TEST(test_ok, "2.0: correct size of output offline tests");

    if(!test_ok)
        return -1;

    test_ok = node[0].IsMap();
    if(test_ok) {
        test_ok &= node[0]["x"].as<double>() == 2;
    }
    TEST(test_ok, "2.1: first sample has correct x");

    test_ok = node[2].IsMap();
    if(test_ok) {
        test_ok &= node[2]["x"].as<double>() == 4;
    }
    TEST(test_ok, "2.2: third sample has correct x");
//    UPDATED(memory, vars);

}

