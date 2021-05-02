cmake_minimum_required(VERSION 3.16)
project(neptune)

# neptune targets
# neptune target - support library
add_library(dlmalloc dlmalloc.c)
target_compile_definitions(dlmalloc PRIVATE 
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
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED IMPORTED_TARGET libdpdk)

add_executable(RunMoon 
    RunMoon/Main.c
    RunMoon/Support.c
)
target_compile_definitions(RunMoon PRIVATE ALLOW_EXPERIMENTAL_API)
target_include_directories(RunMoon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(RunMoon neptune PkgConfig::DPDK pcap)
# patch to link drivers
target_link_options(RunMoon PRIVATE -Wl,--no-as-needed -L/usr/local/lib64 -lrte_net_ixgbe -lrte_mempool_ring)

# SwordHolder targets - SFI pass
find_package(LLVM REQUIRED CONFIG)
add_library(SwordHolderPass MODULE
    SwordHolder/Pass.cxx
)
target_compile_features(SwordHolderPass PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(SwordHolderPass PROPERTIES COMPILE_FLAGS "-fno-rtti")
target_compile_definitions(SwordHolderPass PRIVATE ${LLVM_DEFINITIONS})
target_include_directories(SwordHolderPass PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_directories(SwordHolderPass PRIVATE ${LLVM_LIBRARY_DIRS})

# TianGou target - runtime interface bridger
add_library(TianGou MODULE TianGou.c)

# add_moon util
function(add_moon)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ADD_MOON "${option}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(MOON_NAME ${ADD_MOON_TARGET})
    set(SOURCES ${ADD_MOON_SOURCES})
    # message("[AddMoon] " ${MOON_NAME} ": ${SOURCES}")
    set(BITCODE_FILE ${CMAKE_BINARY_DIR}/${MOON_NAME}-link.bc)
    set(OPT_BITCODE_FILE ${CMAKE_BINARY_DIR}/${MOON_NAME}-opt.bc)

    # step 1: *.c/*.bc + RuntimeLibrary.c -> *.bc
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE C)
    add_library(${MOON_NAME}_SourceLibrary OBJECT ${SOURCES} SwordHolder/RuntimeLibrary.c)
    target_compile_options(${MOON_NAME}_SourceLibrary PRIVATE -emit-llvm -Xclang -disable-O0-optnone -Wno-unused-command-line-argument)
    set_target_properties(${MOON_NAME}_SourceLibrary PROPERTIES 
        POSITION_INDEPENDENT_CODE True
        LINKER_LANGUAGE C
    )

    # step 2: *.bc -> link.bc
    add_custom_command(OUTPUT ${BITCODE_FILE}
        DEPENDS ${MOON_NAME}_SourceLibrary $<TARGET_OBJECTS:${MOON_NAME}_SourceLibrary> 
        COMMAND llvm-link-10 `echo \"$<TARGET_OBJECTS:${MOON_NAME}_SourceLibrary>\" | sed 's/$<SEMICOLON>/ /g'` > ${BITCODE_FILE}
        # COMMAND llvm-dis ${BITCODE_FILE}
    )

    # step 3: link.bc -> opt.bc
    # 1: BITCODE_FILE, 2: OPT_BITCODE_FILE
    set(OPT12_COMMAND opt-10 -O3 -o ${OPT_BITCODE_FILE} ${BITCODE_FILE})
    set(OPT22_COMMAND opt-10 -O3 -o ${OPT_BITCODE_FILE} ${OPT_BITCODE_FILE})
    set(OPT11_COMMAND opt-10 -O3 -o ${BITCODE_FILE} ${BITCODE_FILE})
    set(SFI12_COMMAND opt-10
        -load $<TARGET_FILE:SwordHolderPass> -SwordHolder
        -o ${OPT_BITCODE_FILE} ${BITCODE_FILE}
    )
    if (MOON_NAME MATCHES "_NoSFI$")
        add_custom_command(OUTPUT ${OPT_BITCODE_FILE}
            DEPENDS ${BITCODE_FILE}
            COMMAND ${OPT12_COMMAND}
            # COMMAND llvm-dis ${OPT_BITCODE_FILE}
        )
    else()
        add_custom_command(OUTPUT ${OPT_BITCODE_FILE}
            DEPENDS ${BITCODE_FILE} SwordHolderPass
            COMMAND ${OPT11_COMMAND}
            COMMAND ${SFI12_COMMAND}
            COMMAND ${OPT22_COMMAND}
            # COMMAND llvm-dis ${OPT_BITCODE_FILE}
        )
    endif()

    # step 4: opt.bc -> MOON.so
    # additional options could be applied after target is created
    set_source_files_properties(${OPT_BITCODE_FILE} PROPERTIES LANGUAGE C)
    add_library(${MOON_NAME} MODULE ${OPT_BITCODE_FILE})
    target_compile_options(${MOON_NAME} PRIVATE -Wno-unused-command-line-argument)
    set_target_properties(${MOON_NAME} PROPERTIES LINKER_LANGUAGE C)
    target_link_libraries(${MOON_NAME} PkgConfig::DPDK pcap)
endfunction(add_moon)

# MOON targets
# moons name: Moon_CamelCase
macro(create_moons)

add_moon(TARGET Moon_Test${P} SOURCES 
    Moon/Test.c
)

add_moon(TARGET Moon_TestPlusPlus${P} SOURCES
    Moon/Test++.cxx
)
set_target_properties(Moon_TestPlusPlus${P} PROPERTIES LINKER_LANGUAGE CXX)

add_moon(TARGET Moon_Libnids${P} SOURCES
    # Moon/LibnidsMain.c 
    Moon/LibnidsWtf.c
    Moon/libnids/src/NIDS-link_pic.bc
)
target_link_libraries(Moon_Libnids${P} gthread-2.0 nsl glib-2.0 net)
target_link_options(Moon_Libnids${P} PRIVATE -pthread)

add_moon(TARGET Moon_Prads${P} SOURCES
    Moon/prads/src/prads-link.bc
)
target_link_libraries(Moon_Prads${P} pcap pcre resolv)

add_moon(TARGET Moon_L2Fwd${P} SOURCES
    Moon/L2Fwd.c
)
target_compile_definitions(Moon_L2Fwd${P}_SourceLibrary PRIVATE ALLOW_EXPERIMENTAL_API)

add_moon(TARGET Moon_RxBurstTest${P} SOURCES
    Moon/RxBurstTest.c
)

endmacro()

set(P "")
create_moons()
set(P "_NoSFI")
create_moons()

# 3rd party targets depend by MOONs
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/Moon/libnids/src/NIDS-link_pic.bc
    COMMAND make NIDS-link_pic.bc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Moon/libnids/src
)
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/Moon/prads/src/prads-link.bc
    COMMAND make prads-link.bc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Moon/prads/src
)

# Other test and bench targets
add_executable(MallocTest MallocTest.c)
target_link_libraries(MallocTest neptune)

add_executable(StackTest StackTest.c)
target_link_libraries(StackTest neptune)

add_executable(StackSwitchBench StackSwitchBench.c)
target_link_libraries(StackSwitchBench neptune)

add_executable(LoaderTest LoaderTest.c)
target_link_libraries(LoaderTest neptune)

