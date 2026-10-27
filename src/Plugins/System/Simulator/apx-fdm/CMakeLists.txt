# Flight dynamics model (FDM) for SIL simulation without X-Plane.
#
# Builds the `apx-fdm` executable: a 6-DOF fixed-wing model which talks
# to the `sim` SIL node (sim-darwin / sim-linux) over UDP with the same
# xbus protocol as the X-Plane plugin in ../xplane. One process can run
# several aircraft (one per SIL instance), see README.md.

apx_module(
    TYPE
    STATIC
    SRCS
    "Aircraft.cpp"
    "Aircraft.h"
    "Atmosphere.h"
    "Geo.h"
    "ModelFile.cpp"
    "ModelFile.h"
    "SimLink.cpp"
    "SimLink.h"
    "Vec3.h"
    DEPENDS
    shared.xbus
    shared.xbus.tcp
    shared.mandala
)

add_executable(apx-fdm main.cpp)
target_link_libraries(apx-fdm PRIVATE ${MODULE})
target_compile_definitions(apx-fdm PRIVATE VERSION=\"${APX_GIT_VERSION}\")

# install next to the GCS executable
apx_install(apx-fdm)
