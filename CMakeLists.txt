cmake_minimum_required(VERSION 3.16)
project(neptune)

# config
enable_language(ASM)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED IMPORTED_TARGET libdpdk)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# get rid of LLVM for we don't need to deal with SFI
# find_package(LLVM REQUIRED CONFIG)

# neptune targets
# neptune target - support library
add_library(dlmalloc Vendor/dlmalloc.c)
target_compile_definitions(dlmalloc PRIVATE 
    USE_DL_PREFIX
    MORECORE_CANNOT_TRIM
    HAVE_MMAP=0
    MORECORE=PrivateMoreCore
)
target_include_directories(dlmalloc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(dlmalloc PROPERTIES POSITION_INDEPENDENT_CODE True)
add_library(PrivateHeap INTERFACE)
target_link_libraries(PrivateHeap INTERFACE dlmalloc)

add_library(PrivateStack PrivateStack2.c StackSwitch.S)

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
    PrivateLoad.c
)
target_link_libraries(PrivateLoad ${CMAKE_DL_LIBS})

add_library(neptune INTERFACE)
target_link_libraries(neptune INTERFACE 
    PrivateHeap 
    PrivateStack
    PrivateLoad
)

# RunMoon target - runtime executable
add_executable(RunMoon 
    RunMoon/Main.c
    RunMoon/Support.c
    RunMoon/Bench.c
    RunMoon/Pkey.c
    RunMoon/Protect.c
)
target_compile_definitions(RunMoon PRIVATE ALLOW_EXPERIMENTAL_API)
target_include_directories(RunMoon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(RunMoon PRIVATE neptune PkgConfig::DPDK pcap Threads::Threads ${CMAKE_DL_LIBS})
# patch to link drivers
target_link_options(RunMoon PRIVATE -Wl,--no-as-needed -L/usr/local/lib64 -lrte_net_ixgbe -lrte_mempool_ring)

# TianGou target - runtime interface bridger
add_library(TianGou MODULE 
    TianGou/Glibc.c
    TianGou/Pcap.c
    TianGou/Dpdk.c
    TianGou/Pthread.c
    TianGou/Interface.c
)
target_include_directories(TianGou PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(TianGou PRIVATE Threads::Threads)


# MOON targets
# moons name: Moon_CamelCase

add_library(Moon_Test MODULE
    Moon/Test.c   
)

add_library(Moon_TestPlusPlus MODULE
    Moon/Test++.cxx
)
set_target_properties(Moon_TestPlusPlus PROPERTIES LINKER_LANGUAGE CXX)

add_library(Moon_Libnids MODULE
    # Moon/LibnidsMain.c 
    Moon/LibnidsWtf.c
    # originally statically linked with libnids
    # Vendor/libnids/src/NIDS-link_pic.bc
)
# target_link_libraries(Moon_Libnids PRIVATE gthread-2.0 nsl glib-2.0 net)
target_link_libraries(Moon_Libnids PRIVATE nids)
target_link_libraries(Moon_Libnids PRIVATE Threads::Threads)

# add_library(Moon_Libnids_Slow MODULE
#     # Moon/LibnidsMain.c 
#     Moon/LibnidsSlow.c
#     # originally statically linked with libnids
#     # Vendor/libnids/src/NIDS-link_pic.bc
# )
# # target_link_libraries(Moon_Libnids PRIVATE gthread-2.0 nsl glib-2.0 net)
# target_link_libraries(Moon_Libnids_Slow PRIVATE nids)
# target_link_libraries(Moon_Libnids_Slow PRIVATE Threads::Threads)

# No, Prads is already a shared object
# add_moon(TARGET Moon_Prads SOURCES
#     Vendor/prads/src/prads-link.bc
# )
# target_link_libraries(Moon_Prads PRIVATE pcap pcre resolv)

add_library(Moon_L2Fwd MODULE
    Moon/L2Fwd.c
)
target_compile_definitions(Moon_L2Fwd PRIVATE ALLOW_EXPERIMENTAL_API)
target_compile_options(Moon_L2Fwd PRIVATE -Wno-unused-command-line-argument)
target_link_libraries(Moon_L2Fwd PRIVATE PkgConfig::DPDK pcap Threads::Threads)

add_library(Moon_RxBurstTest MODULE
    Moon/RxBurstTest.c
)

# midstat currently unsupported
# add_moon(TARGET Moon_MidStat SOURCES
#     Vendor/mOS/core/lib/libmtcp.bc
#     Vendor/mOS/samples/midstat/midstat.c
#     Vendor/mOS/samples/common/applib.c
# )
# target_compile_definitions(Moon_MidStat_SourceLibrary PRIVATE NDEBUG NETSTAT INFO DBGERR DBGCERR)
# target_include_directories(Moon_MidStat_SourceLibrary PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/mOS/core/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/mOS/samples/common
#     ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/mOS/util/include
# )
# target_link_libraries(Moon_MidStat PRIVATE ssl crypto numa rt)

# 3rd party targets depend by MOONs
# add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/libnids/src/NIDS-link_pic.bc
#     COMMAND make NIDS-link_pic.bc 2>/dev/null
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/libnids/src
# )
# add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/prads/src/prads-link.bc
#     COMMAND make prads-link.bc 2>/dev/null
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/prads/src
# )
# add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/mOS/core/lib/libmtcp.bc
#     COMMAND make DISABLE_NUMA=1 ../lib/libmtcp.bc 2>/dev/null
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/mOS/core/src
# )

# Other test and bench targets
add_executable(MallocTest MallocTest.c)
target_link_libraries(MallocTest neptune)

add_executable(StackTest StackTest.c)
target_link_libraries(StackTest neptune)

add_executable(StackSwitchBench StackSwitchBench.c)
target_link_libraries(StackSwitchBench neptune)

add_executable(LoaderTest LoaderTest.c)
target_link_libraries(LoaderTest neptune)

add_executable(ExpPthread ExpPthread.c)
target_compile_definitions(ExpPthread PRIVATE ALLOW_EXPERIMENTAL_API)
target_link_libraries(ExpPthread PRIVATE PkgConfig::DPDK Threads::Threads)
target_link_options(ExpPthread PRIVATE -Wl,--no-as-needed -L/usr/local/lib64 -lrte_net_ixgbe -lrte_mempool_ring)