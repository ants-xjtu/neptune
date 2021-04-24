cmake_minimum_required(VERSION 3.16)
project(neptune)

# neptune targets
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
target_link_options(RunMoon PRIVATE -Wl,--no-as-needed -L/usr/local/lib64 -lrte_net_ixgbe -lrte_mempool_ring)

# SwordHolder
find_package(LLVM REQUIRED CONFIG)
add_library(SwordHolderPass MODULE
    SwordHolder/Pass.cxx
)
target_compile_features(SwordHolderPass PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(SwordHolderPass PROPERTIES COMPILE_FLAGS "-fno-rtti")
target_compile_definitions(SwordHolderPass PRIVATE ${LLVM_DEFINITIONS})
target_include_directories(SwordHolderPass PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_directories(SwordHolderPass PRIVATE ${LLVM_LIBRARY_DIRS})

# TianGou
add_library(TianGou SHARED TianGou.c)

# add_moon util and 3rd libraries
function(add_moon)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ADD_MOON "${option}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(MOON_NAME ${ADD_MOON_TARGET})
    set(SOURCES ${ADD_MOON_SOURCES})

    set(BITCODE_FILE ${CMAKE_BINARY_DIR}/${MOON_NAME}-link.bc)
    set(OPT_BITCODE_FILE ${CMAKE_BINARY_DIR}/${MOON_NAME}-opt.bc)
    set(OPT_COMMAND opt-10 
        -load ${GMEM_DIR}/obj/libGmem-pass.so -gmem
        -Gmem-rw=w
        -Gmem-check-method=ifelseall
        -Gmem-verify-external-call-args=false
        -Gmem-whitelist-section=safe_functions
        < ${BITCODE_FILE}
        > ${OPT_BITCODE_FILE}
    )

    message("[AddMoon] " ${MOON_NAME} ": ${SOURCES}")
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE C)
    add_library(${MOON_NAME}_SourceLibrary OBJECT ${SOURCES})
    target_compile_options(${MOON_NAME}_SourceLibrary PRIVATE -emit-llvm)
    set_target_properties(${MOON_NAME}_SourceLibrary PROPERTIES 
        POSITION_INDEPENDENT_CODE True
        LINKER_LANGUAGE C
    )

    add_custom_command(OUTPUT ${BITCODE_FILE}
        DEPENDS neptune-pass $<TARGET_OBJECTS:${MOON_NAME}_SourceLibrary>
        COMMAND llvm-link-10 `echo \"$<TARGET_OBJECTS:${MOON_NAME}_SourceLibrary>\" | sed 's/$<SEMICOLON>/ /g'` ${GMEM_DIR}/obj/Auxiliary.o > ${BITCODE_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_command(OUTPUT ${OPT_BITCODE_FILE}
        DEPENDS ${BITCODE_FILE}
        COMMAND ${OPT_COMMAND}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_source_files_properties(${BITCODE_FILE} ${OPT_BITCODE_FILE} PROPERTIES LANGUAGE C)

    if (MOON_NAME MATCHES "_NoSFI$")
        add_library(${MOON_NAME} SHARED ${BITCODE_FILE})
    else()
        add_library(${MOON_NAME} SHARED ${OPT_BITCODE_FILE})
    endif()
    target_compile_options(${MOON_NAME} PRIVATE -Wno-unused-command-line-argument)
    set_target_properties(${MOON_NAME} PROPERTIES LINKER_LANGUAGE C)
    target_link_libraries(${MOON_NAME} PkgConfig::DPDK pcap)
endfunction(add_moon)

add_custom_target(libnids
    COMMAND make NIDS-link_pic.bc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/moon/libnids/src
)
add_custom_target(prads
    COMMAND make prads-link.bc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/moon/prads/src
)


# MOON targets
# moons name: Moon_CamelCase
macro(create_moons)

add_moon(TARGET Moon_Test${P} SOURCES 
    moon/Test.c
)

add_moon(TARGET Moon_TestPlusPlus${P} SOURCES
    moon/Test++.cxx
)
set_target_properties(Moon_TestPlusPlus${P} PROPERTIES LINKER_LANGUAGE CXX)

add_moon(TARGET Moon_Libnids${P} SOURCES
    moon/LibnidsMain.c 
    moon/libnids/src/NIDS-link_pic.bc
)
add_dependencies(Moon_Libnids${P} libnids)
target_link_libraries(Moon_Libnids${P} gthread-2.0 nsl glib-2.0 net)
target_link_options(Moon_Libnids${P} PRIVATE -pthread)

add_moon(TARGET Moon_Prads${P} SOURCES
    # moon/Prads.c 
    moon/prads/src/prads-link.bc
)
add_dependencies(Moon_Prads${P} prads)
target_link_libraries(Moon_Prads${P} pcap pcre resolv)

endmacro()

set(P "")
create_moons()
set(P "_NoSFI")
create_moons()

# add_moon(Moon_LibnidsNoSFI moon/LibnidsMain.c moon/libnids/src/NIDS-link_pic.bc)
# add_dependencies(Moon_LibnidsNoSFI libnids)
# target_link_libraries(Moon_LibnidsNoSFI gthread-2.0 nsl glib-2.0 net)
# target_link_options(Moon_LibnidsNoSFI PRIVATE -pthread)

# add_moon(Moon_PradsNoSFI moon/Prads.c moon/prads/src/prads-link.bc)
# add_dependencies(Moon_PradsNoSFI prads) # just mean the sequence of building, not implying actual depenencies
# target_link_libraries(Moon_PradsNoSFI pcap pcre resolv)

# add_moon(Moon_TestPlusPlusNoSFI moon/Test++.cxx)
# set_target_properties(Moon_TestPlusPlusNoSFI PROPERTIES LINKER_LANGUAGE CXX)
