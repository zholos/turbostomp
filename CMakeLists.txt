cmake_minimum_required(VERSION 3.0)

project(turbostomp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL GNU)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
    add_definitions(-Wreturn-type)
    add_definitions(-Wall -Wno-parentheses -Wno-sign-compare)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL Clang)
    add_definitions(-w)
endif()

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/CMakeModules")

find_package(pgamecc REQUIRED)
include_directories(${PGAMECC_INCLUDE_DIRS})
link_libraries(${PGAMECC_LIBRARIES})

find_package(ODE REQUIRED)
include_directories(${ODE_INCLUDE_DIRS})
link_libraries(${ODE_LIBRARIES})

add_subdirectory(core)
add_subdirectory(shaders)
add_subdirectory(meshes)
add_subdirectory(levels)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_subdirectory(test)
endif()
