cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

# paths used by apx_module
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../lib" APX_SHARED_DIR)
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src" APX_MODULES_DIR)

list(APPEND CMAKE_MODULE_PATH ${APX_SHARED_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/static)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(APX_PLUGINS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/plugins)

file(REAL_PATH "${CMAKE_SOURCE_DIR}/resources" APX_RESOURCES_DIR)

# LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

option(CMAKE_VERBOSE_MAKEFILE "" OFF)

set(CMAKE_PREFIX_PATH
    "$ENV{HOME}/QtOS/5.15.2/clang_64"
    CACHE PATH "Qt installation path"
)

# set(DESTDIR
#     ""
#     CACHE PATH ""
# )
set(CMAKE_INSTALL_PREFIX
    "${CMAKE_CURRENT_BINARY_DIR}/install"
    CACHE PATH ""
)

set(CMAKE_OSX_DEPLOYMENT_TARGET
    "10.13"
    CACHE STRING "Minimum OS X deployment version"
)

# set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# set(CMAKE_MACOSX_BUNDLE ON)
# set(CMAKE_MACOSX_RPATH ON)

include(apx)
include(git)

set(APX_COPYRIGHT "(C) ${APX_GIT_YEAR} Aliaksei Stratsilatau (sa@uavos.com)")

project(
    gcs
    VERSION ${APX_GIT_VERSION}
    DESCRIPTION "APX Ground Control"
    HOMEPAGE_URL "http://docs.uavos.com"
    LANGUAGES CXX C
)

include(ccache)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(
    Qt5
    COMPONENTS Core QuickCompiler
    REQUIRED
)
file(REAL_PATH "${Qt5Core_DIR}/../../.." QT_DIR)

include(apx_gcs_qt)
include(apx_gcs_qrc)
include(apx_gcs_lib)
include(apx_gcs_plugin)
include(apx_gcs_framework)
include(apx_gcs_install)
include(apx_gcs_bundle)
include(apx_gcs_meta)

apx_use_module("main")

add_subdirectory("src/Plugins")

apx_metadata_generate()

apx_install_bundle_libs()
