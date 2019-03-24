project (dynamic_thread_pool)
cmake_minimum_required (VERSION 3.0)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -pedantic -O0 -g3 -std=c++11 -fPIC")
set (CMAKE_BINARY_DIR bin)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_library (THREAD_LIB	libpthread.so)

include_directories(include)

add_executable(BaseUsage0 examples/baseUsage0.cpp)
target_link_libraries (BaseUsage0 ${THREAD_LIB})

add_executable(BaseUsage1 examples/baseUsage1.cpp)
target_link_libraries (BaseUsage1 ${THREAD_LIB})

add_executable(DynamicityTest0 examples/dynamicityTest0.cpp)
target_link_libraries (DynamicityTest0 ${THREAD_LIB})

add_executable(DynamicityTest1 examples/dynamicityTest1.cpp)
target_link_libraries (DynamicityTest1 ${THREAD_LIB})

add_executable(WorkerThreadTest examples/workerThreadTest.cpp)
target_link_libraries (WorkerThreadTest ${THREAD_LIB})