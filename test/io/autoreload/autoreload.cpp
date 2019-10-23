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
#include "io/addons/time/SystemTime.h"
#include "io/testing/OfflineTests.h"

#include "io/ide/AutoReloader.h"
#include "io/ide/AutoReloaderWithTests.h"


int main() {
    using namespace abtm;

    LoadFabric<std::string, MemoryDuktape> lf(ABTM::Classic);

    AutoReloader AR(&lf, "localhost:9090", 100, 100);

    AR.main_file = "../test/io/autoreload/nodes.yaml";

    AR.spinForever();
}