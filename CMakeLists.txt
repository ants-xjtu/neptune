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

# add_library(PrivateStack PrivateStack.c)

add_custom_command(
    OUTPUT glibc-build/libc.a
    COMMAND bash -c "make -j $(grep -c ^processor /proc/cpuinfo)"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glibc-build
)
add_library(PrivateLoad PrivateLoad.c)

add_library(neptune INTERFACE)
target_link_libraries(neptune INTERFACE 
    PrivateHeap 
    # PrivateStack 
    PrivateLoad
)

add_executable(MallocTest MallocTest.c)
target_link_libraries(MallocTest neptune)

add_executable(LoaderTest LoaderTest.c)
target_link_libraries(LoaderTest neptune ${CMAKE_CURRENT_SOURCE_DIR}/glibc-build/libc.a)