cmake_minimum_required(VERSION 3.12)
project(abtm2)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)
set(CMAKE_MY_LIBS_PATH ../../libs)
include_directories(${CMAKE_MY_LIBS_PATH}/rapidjson/include)
include_directories(${CMAKE_MY_LIBS_PATH}/esprima-cpp)
include_directories(${CMAKE_MY_LIBS_PATH}/duktape-2.4.0/src)

set(MEMORY_DUKTAPE_LIBS ${CMAKE_MY_LIBS_PATH}/duktape-2.4.0/src/duktape.c ${CMAKE_MY_LIBS_PATH}/esprima-cpp/esprima.cpp)


add_executable(test_memory_double test/core/memory/memory_double.cpp)

add_executable(test_memory_duktape test/core/memory/memory_duktape.cpp ${MEMORY_DUKTAPE_LIBS})

add_executable(test_node test/core/nodes/test.cpp ${MEMORY_DUKTAPE_LIBS})

add_executable(test_tree test/core/exec/test_tree.cpp ${MEMORY_DUKTAPE_LIBS})