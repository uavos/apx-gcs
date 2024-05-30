cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

# paths used by apx_module
get_filename_component(APX_SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shared" ABSOLUTE)
get_filename_component(APX_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH ${APX_SHARED_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out/bin)

set(APX_PLUGINS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out/plugins)

set(APX_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/resources")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING ""
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON) # LTO
endif()

option(CMAKE_VERBOSE_MAKEFILE "" OFF)
option(CCACHE "Use ccache if available" ON)

set(CMAKE_INSTALL_PREFIX
    "${CMAKE_CURRENT_BINARY_DIR}/install"
    CACHE PATH ""
)

# Universal binary build for MacOS Release
if(APPLE AND CMAKE_BUILD_TYPE STREQUAL Release)
    set(CMAKE_OSX_DEPLOYMENT_TARGET
        "12"
        CACHE STRING "Minimum OS X deployment version"
    )
    set(CMAKE_OSX_ARCHITECTURES
        "arm64;x86_64"
        CACHE STRING "MacOS universal binary" FORCE
    )
endif()

# Project creation and versioning
include(git)

set(APX_COPYRIGHT "(C) ${APX_GIT_YEAR} Aliaksei Stratsilatau (sa@uavos.com)")

project(
    gcs
    VERSION ${APX_GIT_VERSION}
    DESCRIPTION "APX Ground Control"
    HOMEPAGE_URL "http://docs.uavos.com"
    LANGUAGES CXX C
)

include(apx)

include(ccache)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt should be found in PATH
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)
get_filename_component(QT_DIR "${Qt6Core_DIR}/../../.." ABSOLUTE)
message(STATUS "Qt6 found in ${QT_DIR}")

include(apx_gcs_qt)
include(apx_gcs_qrc)
include(apx_gcs_lib)
include(apx_gcs_plugin)
include(apx_gcs_framework)
include(apx_gcs_meta)
include(apx_gcs_install)
include(apx_gcs_deploy)

apx_use_module("main")

add_subdirectory("src/Plugins")

apx_metadata_generate()

add_subdirectory("docs")
