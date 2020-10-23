# APX Autopilot by UAVOS - Ground Control Software

GCS - APX Ground Control Software

## Build instructions

```
mkdir apx && cd apx
git clone --recurse-submodules git@github.com:uavos/apx-gcs.git gcs
git clone --recurse-submodules git@github.com:uavos/apx-lib.git lib
```

Use `gcs/gcs.qbs` to open and build project in QtCreator.

>The included `gcs/Makefile` is used by release builds only.
