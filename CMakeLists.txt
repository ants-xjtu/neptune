cmake_minimum_required(VERSION 3.16)
project(neptune)

# neptune targets
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

set(GMEM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gmem)

add_custom_target(neptune-pass
    COMMAND make
    WORKING_DIRECTORY ${GMEM_DIR}
)
add_dependencies(neptune neptune-pass)

add_executable(MallocTest MallocTest.c)
target_link_libraries(MallocTest neptune)

add_executable(StackTest StackTest.c)
target_link_libraries(StackTest neptune)

add_executable(StackSwitchBench StackSwitchBench.c)
target_link_libraries(StackSwitchBench neptune)

add_executable(LoaderTest LoaderTest.c)
target_link_libraries(LoaderTest neptune)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED IMPORTED_TARGET libdpdk)

add_executable(RunMoon RunMoon.c)
target_link_libraries(RunMoon neptune PkgConfig::DPDK pcap)
target_link_options(RunMoon PUBLIC -Wl,--no-as-needed -L/usr/local/lib64 -lrte_net_ixgbe -lrte_mempool_ring)

# TianGou
add_library(TianGou SHARED TianGou.c)

# MOON targets
function(add_moon)
    set(MOON_NAME ${ARGV0})
    set(SOURCE ${ARGV1})
    set(BITCODE_OBJECTS ${ARGV})
    list(REMOVE_AT BITCODE_OBJECTS 0 1)

    set(LINK_BYTECODE_FILE ${CMAKE_BINARY_DIR}/${MOON_NAME}-link.bc)
    set(OPT_BYTECODE_FILE ${CMAKE_BINARY_DIR}/${MOON_NAME}-opt.bc)
    set(OBJ_FILE ${CMAKE_BINARY_DIR}/${MOON_NAME}.o)
    if(MOON_NAME MATCHES "NoSFI$")
        set(OPT_COMMAND cp ${LINK_BYTECODE_FILE} ${OPT_BYTECODE_FILE})
    else()
        set(OPT_COMMAND opt-10 
            -load ${GMEM_DIR}/obj/libGmem-pass.so -gmem
            -Gmem-rw=w
            -Gmem-check-method=ifelseall
            -Gmem-verify-external-call-args=false
            -Gmem-whitelist-section=safe_functions
            < ${LINK_BYTECODE_FILE}
            > ${OPT_BYTECODE_FILE}
        )
    endif()

    message("add moon " ${MOON_NAME} "(" ${SOURCE} ") + " ${BITCODE_OBJECTS})
    add_custom_command(OUTPUT ${OBJ_FILE}
        DEPENDS neptune-pass ${SOURCE}
        COMMAND clang-10 -fPIC -emit-llvm -c -o ${CMAKE_BINARY_DIR}/${MOON_NAME}-source.bc ${SOURCE}
        COMMAND llvm-link-10 ${CMAKE_BINARY_DIR}/${MOON_NAME}-source.bc ${GMEM_DIR}/obj/Auxiliary.o ${BITCODE_OBJECTS} > ${LINK_BYTECODE_FILE}
        COMMAND ${OPT_COMMAND}
        COMMAND clang-10 -c -fPIC ${OPT_BYTECODE_FILE} -o ${OBJ_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_library(${MOON_NAME} SHARED ${OBJ_FILE})
    set_target_properties(${MOON_NAME} PROPERTIES LINKER_LANGUAGE C)
    target_link_libraries(${MOON_NAME} PkgConfig::DPDK pcap)
endfunction(add_moon)

# moons name: Moon_CamelCase
add_moon(Moon_Test moon/test.c)
add_moon(Moon_TestNoSFI moon/test.c)

add_moon(Moon_Libnids moon/testLibnids.c moon/libnids/src/NIDS-link_pic.bc)
add_custom_target(libnids
    COMMAND make NIDS-link_pic.bc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/moon/libnids/src
)
add_dependencies(Moon_Libnids libnids)
target_link_libraries(Moon_Libnids gthread-2.0 nsl glib-2.0 net)
target_link_options(Moon_Libnids PUBLIC -pthread)

add_moon(Moon_LibnidsNoSFI moon/testLibnids.c moon/libnids/src/NIDS-link_pic.bc)
add_dependencies(Moon_LibnidsNoSFI libnids)
target_link_libraries(Moon_LibnidsNoSFI gthread-2.0 nsl glib-2.0 net)
target_link_options(Moon_LibnidsNoSFI PUBLIC -pthread)

add_moon(Moon_prads moon/testPrads.c moon/prads/src/prads-link.bc)
add_custom_target(prads
    COMMAND make prads-link.bc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/moon/prads/src
)
add_dependencies(Moon_prads prads) # just mean the sequence of building, not implying actual depenencies
target_link_libraries(Moon_prads pcap pcre resolv)

add_moon(Moon_pradsNoSFI moon/testPrads.c moon/prads/src/prads-link.bc)
add_dependencies(Moon_pradsNoSFI prads) # just mean the sequence of building, not implying actual depenencies
target_link_libraries(Moon_pradsNoSFI pcap pcre resolv)