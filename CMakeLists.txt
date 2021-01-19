cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

set(package-contact "sa@uavos.com")

# paths used by apx_module
get_filename_component(APX_SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib" ABSOLUTE)
get_filename_component(APX_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH ${APX_SHARED_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib/shared)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

get_filename_component(APX_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources" ABSOLUTE)

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

set(prefix "${PROJECT_NAME}.app/Contents")
set(INSTALL_RUNTIME_DIR "${prefix}/MacOS")
set(INSTALL_RESOURCES_DIR "${prefix}/Resources")

# set(APX_APP_DATA_DIR "Contents/Resources")
# set(APX_APP_LIBS_DIR "Contents/Frameworks")
# set(APX_APP_PLUGINS_DIR "Contents/PlugIns")

include(ccache)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(
    Qt5
    COMPONENTS Widgets
    REQUIRED
)

include(apx_gcs_qt)
include(apx_gcs_qrc)
include(apx_gcs_bundle)
include(apx_gcs_lib)
include(apx_gcs_app)

apx_use_module("main")

# app dist package

set(CPACK_PACKAGE_NAME ${PROJECT_DESCRIPTION})
string(TOLOWER ${CMAKE_SYSTEM_NAME} CPACK_SYSTEM_NAME)
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_DESCRIPTION}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME})
string(REPLACE " " "_" CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_FILE_NAME})

set(CPACK_GENERATOR "DragNDrop")
include(CPack)
