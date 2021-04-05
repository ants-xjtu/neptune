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

add_library(PrivateLoad 
    loader/NanoNF.h
    loader/NFinit.c
    loader/NFlink.h
    loader/NFmap.c
    loader/NFopen.c
    loader/NFreloc.c
    loader/NFsym.c
    loader/NFusage.c
    loader/NFversion.c
    PrivateLoad.c
)
target_link_libraries(PrivateLoad ${CMAKE_DL_LIBS})

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

add_executable(LoaderTest LoaderTest.c)
target_link_libraries(LoaderTest neptune)

# moons name: Moon_CamelCase
add_executable(Moon_Test moon/test.c)

add_executable(StackSwitchBench benchmark/StackSwitchBench.c)
target_link_libraries(StackSwitchBench neptune)

add_executable(Moon_Libnids moon/testLibnids.c)
# target_compile_options(Moon_Libnids PRIVATE -fno-stack-protector)
target_link_libraries(Moon_Libnids neptune)

add_executable(Moon_Libnids-nosfi moon/testLibnids-nosfi.c)
# target_compile_options(Moon_Libnids-nosfi PRIVATE -fno-stack-protector)
target_link_libraries(Moon_Libnids-nosfi neptune)
