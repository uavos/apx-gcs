apx_plugin(DEPENDS lib.ApxGcs)

# aircraft models of the built-in flight dynamics
apx_install_res(${APX_RESOURCES_DIR} fdm/*.fdm)

add_subdirectory(xplane)
add_subdirectory(apx-fdm)
