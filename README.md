# APX Autopilot by UAVOS - Ground Control Software

GCS - APX Ground Control Software

## Build instructions

### Clone the repository

```
mkdir apx && cd apx
git clone --recurse-submodules git@github.com:uavos/apx-gcs.git gcs
git clone --recurse-submodules git@github.com:uavos/apx-lib.git lib
```

### Build project

```
cd gcs
cmake -H. -Bbuild -G Ninja
cmake --build build
```

To create app bundle use `bundle` target.

See additional targets in `Makefile`.

### Output directories

 - `build/out` contains runtime binaries;
 - `build/deploy` contains app bundle;
 