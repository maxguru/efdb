cmake_minimum_required (VERSION 2.8)

project (efdb)
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

set(Debug Debug)
set(RelWithDebInfo RelWithDebInfo)
set(Profile Profile)
set(Release Release)
set(x86 x86)
set(Native Native)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_BUILD_TYPE ${Debug} CACHE STRING "Set build type: ${Debug} | ${RelWithDebInfo} | ${Profile} | ${Release}")
set(PLATFORM_TARGET ${Native} CACHE STRING "Set target platform: ${x86} | ${Native}")

include_directories("${PROJECT_BINARY_DIR}")

message(STATUS "Building efdb ${VERSION_MAJOR}.${VERSION_MINOR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform target: ${PLATFORM_TARGET}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")

# cmake module files dir
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(cmake/includes/Libraries.cmake)

# compile all source files
file(GLOB_RECURSE EFDB_CPP_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/test/*.cpp")
file(GLOB_RECURSE EFDB_HEADERS "${PROJECT_SOURCE_DIR}/src/*.h" "${PROJECT_SOURCE_DIR}/test/*.h")

set (EFDB_INCLUDE_DIRS "")
foreach (_headerFile ${EFDB_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND EFDB_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES EFDB_INCLUDE_DIRS)

set(EFDB_SOURCES ${EFDB_CPP_SOURCES})
set(EFDB_INCLUDE_DIRS ${EFDB_INCLUDE_DIRS} ${CMAKE_BINARY_DIR})

include(cmake/includes/CompilerOptions.cmake)

include_directories(${EFDB_INCLUDE_DIRS})

# config.h
configure_file (
  "${PROJECT_SOURCE_DIR}/src/config.h.in"
  "${PROJECT_SOURCE_DIR}/src/config.h"
  )

add_executable(efdb ${EFDB_SOURCES})

target_link_libraries(efdb ${LIBS})
