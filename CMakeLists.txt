cmake_minimum_required(VERSION 3.16)
project(neptune)

add_library(dlmalloc dlmalloc.c)
target_compile_definitions(dlmalloc PUBLIC 
    USE_DL_PREFIX
    MORECORE_CANNOT_TRIM
    HAVE_MMAP=0
    MORECORE=PrivateMoreCore
)
set_target_properties(dlmalloc PROPERTIES POSITION_INDEPENDENT_CODE True)
add_library(PrivateHeap INTERFACE)
target_link_libraries(PrivateHeap INTERFACE dlmalloc)

enable_language(ASM)
add_library(PrivateStack PrivateStack2.c StackSwitch.S)
# TODO
add_library(PrivateLoad 
    Loader/NanoNF.h
    Loader/NFinit.c
    Loader/NFlink.h
    Loader/NFmap.c
    Loader/NFopen.c
    Loader/NFreloc.c
    Loader/NFsym.c
    Loader/NFusage.c
    Loader/NFversion.c
)
target_link_libraries(PrivateLoad dl)

add_library(neptune INTERFACE)
target_link_libraries(neptune INTERFACE 
    PrivateHeap 
    PrivateStack
    PrivateLoad
)

add_custom_target(neptune-pass
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gmem_pass
)
add_dependencies(neptune neptune-pass)

add_executable(MallocTest MallocTest.c)
target_link_libraries(MallocTest neptune)

add_executable(StackTest StackTest.c)
target_link_libraries(StackTest neptune)

# moons name: Moon_CamelCase
add_executable(Moon_Test moon/test.c)

add_executable(StackSwitchBench benchmark/StackSwitchBench.c)
target_link_libraries(StackSwitchBench neptune)

SET(REMOVE_ORQ "-fno-stack-clash-protection")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${REMOVE_ORQ}")
add_executable(testLibnids LoaderTest/testLibnids.c)
target_link_libraries(testLibnids neptune)

add_executable(testLibnids-nosfi LoaderTest/testLibnids-nosfi.c)
target_link_libraries(testLibnids-nosfi neptune)
target_link_libraries(testLibnids-nosfi /home/hypermoon/neptune-yh/binary/libnids-nosfi.so.1.25)
