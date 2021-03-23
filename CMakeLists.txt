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

# TODO
# add_library(PrivateStack PrivateStack.c)
# add_library(PrivateLoad PrivateLoad.c)

add_library(neptune INTERFACE)
target_link_libraries(neptune INTERFACE 
    PrivateHeap 
    # PrivateStack 
    # PrivateLoad
)

add_custom_target(neptune-pass
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gmem_pass
)
add_dependencies(neptune neptune-pass)

add_executable(MallocTest MallocTest.c)
target_link_libraries(MallocTest neptune)

# moons name: Moon_CamelCase
add_executable(Moon_Test moon/test.c)
