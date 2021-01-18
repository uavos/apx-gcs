cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

set(package-contact "sa@uavos.com")

set(APX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(APX_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# paths used by apx_module
get_filename_component(APX_SHARED_DIR "${APX_SOURCE_DIR}/../lib" ABSOLUTE)
get_filename_component(APX_MODULES_DIR "${APX_SOURCE_DIR}/src" ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH ${APX_SHARED_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${APX_SOURCE_DIR}/cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${APX_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${APX_BINARY_DIR}/lib/shared)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${APX_BINARY_DIR})

set(APX_APP_DATA_DIR "Contents/Resources")
set(APX_APP_LIBS_DIR "Contents/Frameworks")
set(APX_APP_PLUGINS_DIR "Contents/PlugIns")

# LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

option(CMAKE_VERBOSE_MAKEFILE "" OFF)

set(CMAKE_PREFIX_PATH
    "$ENV{HOME}/QtOS/5.15.2/clang_64"
    CACHE PATH "Qt installation path"
)

set(CMAKE_MACOSX_BUNDLE ON)
# set(CMAKE_MACOSX_RPATH ON)

include(apx)
include(git)

set(APX_COPYRIGHT "(C) ${APX_GIT_YEAR} Aliaksei Stratsilatau (sa@uavos.com)")

project(
    apx-gcs
    VERSION ${APX_GIT_VERSION}
    DESCRIPTION "APX Ground Control"
    HOMEPAGE_URL "http://docs.uavos.com"
    LANGUAGES CXX C ASM
)

include(ccache)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(
    Qt5
    COMPONENTS Core
    REQUIRED
)

include(apx_gcs_lib)
include(apx_gcs_bundle)

apx_use_module("lib.ApxCore")
