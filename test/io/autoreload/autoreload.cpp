//
// Created by safoex on 07.10.19.
//


#include <io/IOCenter.h>
#include "io/load/Loaders.h"
#include "io/view/ViewRep.h"
#include "core/exec/ABTM.h"
#include "core/memory/double/MemoryDouble.h"
#include "core/memory/js/duktape/MemoryDuktape.h"
#include "yaml-cpp/yaml.h"
#include "../test/testing.h"
#include "io/ros/ROSLoader.h"
#include "io/ros/ROSCommandsConverter.h"
#include "io/IOExecutor.h"
#include "io/load/LoadFabric.h"

#include "io/ide/AutoReloader.h"

template<class Var, class Mem>
void import_file_to(std::string const& filename, abtm::LoadFabric<Var,Mem> & lf) {
    std::ifstream js_file(filename);
    std::string duktape_js_script   ( (std::istreambuf_iterator<char>(js_file) ),
                                      (std::istreambuf_iterator<char>()    ) );
    lf.memory->import(duktape_js_script);
}

int main() {
    using namespace abtm;

    LoadFabric<std::string, MemoryDuktape> lf(ABTM::Classic);

    AutoReloader AR(&lf, false, 100, 100);

    import_file_to("../src/core/memory/js/duktape/memory_duktape.js", lf);
    import_file_to("../../../libs/rosmsgjs/ros_embed_description.js", lf);
    import_file_to("../../../libs/rosmsgjs/ros_embed.js", lf);

    AR.files.emplace_back("../test/io/autoreload/templates.yaml");
    AR.files.emplace_back("../test/io/autoreload/nodes.yaml");
    AR.spinOnce();

    AR.spinForever();
}