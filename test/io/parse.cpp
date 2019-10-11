//
// Created by safoex on 09.09.19.
//

#include <io/IOCenter.h>
#include "io/load/Loaders.h"
#include "io/view/ViewRep.h"
#include "core/exec/ABTM.h"
#include "core/memory/double/MemoryDouble.h"
#include "yaml-cpp/yaml.h"
#include "../test/testing.h"
#include "io/ros/ROSLoader.h"
#include "io/ros/ROSCommandsConverter.h"
#include "io/IOExecutor.h"
#include "io/load/LoadFabric.h"




int main() {
    using namespace abtm;

    LoadFabric<double, CalculatorMemory> lf(ABTM::Classic);

    std::ifstream test("../test/io/test.yaml");
    YAML::Node file = YAML::Load(test);
    std::cout << measure<>::execution([&]() {
        lf.io->process({{TICKET_WORD, std::string()},
                        {LOAD_WORD,   file},
                        {FORMAT_WORD, YAML_WORD}}, IO_INPUT, nullptr);
        lf.io->process({{BUILD_TREE_WORD, std::string("")},
                        {TICKET_WORD,     std::string("")}}, IO_INPUT, nullptr);
    }) << std::endl;

    test.close();
    sleep(1);

    std::ifstream test2("../test/io/test.yaml");

    file = YAML::Load(test2);
    std::cout << measure<>::execution([&]() {
        lf.resetHard();
        lf.io->process({{TICKET_WORD, std::string()},
                        {LOAD_WORD,   file},
                        {FORMAT_WORD, YAML_WORD}}, IO_INPUT, nullptr);
    }) << std::endl;

    std::cout << measure<>::execution([&]() {
        lf.io->process({{BUILD_TREE_WORD, std::string("")},
                    {TICKET_WORD,     std::string("")}}, IO_INPUT, nullptr);
    }) << std::endl;

}