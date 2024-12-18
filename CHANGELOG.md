# [APX Ground Control v11.2.0](https://github.com/uavos/apx-gcs/releases/tag/release-11.2.0) (12/18/24)

> Branch: `main`\
> Date: `12/18/24 14:40:45`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/4ff1445905132122d572e51ff75009807ac1be73...fe34f8bb01884b0f6c205c96f1e11adee9a3d582)

## New Features
* Antenna Tracking System (ATS) control plugin

## Bug Fixes
* segfault on remote conf change
* subdirectory
* parameters saved wav announcement
* QML active focus op (closes [`86`](https://github.com/uavos/apx-gcs/issues/86))
* event overflows for SDL
* init SDL for macOS
* readme file

## Performance Enhancements
* fact lookup by name optimizations
* fact tree model queued response
* maps storage in files

# [APX Ground Control v11.1.17](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.17) (05/30/24)

> Branch: `main`\
> Date: `05/30/24 17:03:27`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/145a2911defe7aa9dc23bb0fdfcf838eeda109d7...ab9f993f32a6139903d8342c29ccc2aef8448a99)

## Bug Fixes
* xplane plugin universal binary
* XPDR protocol fix
* Vehicle display in XPDR downlink mode
* app loading ports enabling behavior

# [APX Ground Control v11.1.16](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.16) (04/24/24)

> Branch: `main`\
> Date: `04/24/24 16:05:30`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/0fdf2020589d4955289786c402cd444bd06d37b3...9397ad4321ab5d5a28249db6dec17c7db8112b3c)

## Bug Fixes
* mission title buffer overflow crash (closes [`71`](https://github.com/uavos/apx-gcs/issues/71))
* nodes plugin title

## Performance Enhancements
* move xplane models and files to another [repo](uavos/apx-sim-xpl)

# [APX Ground Control v11.1.15](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.15) (03/10/24)

> Branch: `main`\
> Date: `03/10/24 13:27:50`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/b69c2fa4d3991cbee52dd8db13f2ffe0013666ee...c3ea5472029ada288896893dd175557198b705de)

## Bug Fixes
* [`27`](https://github.com/uavos/apx-gcs/issues/27) multi gcs commands unpack error
* multi GCS bundle unpack locally

# [APX Ground Control v11.1.14](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.14) (02/23/24)

> Branch: `main`\
> Date: `02/23/24 08:58:47`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/f69f9f789c1fca25dee2edd7948f6bd251196080...9b75ea65ba46fc209aeddf6930c831aa78963a25)

## Bug Fixes
* Autoupdate crush bug
* [`11`](https://github.com/uavos/apx-gcs/issues/11) Access modifiers order fix
* [`44`](https://github.com/uavos/apx-gcs/issues/44) exchange creating symlink on .xpl plugin
* [`45`](https://github.com/uavos/apx-gcs/issues/45) add exception check
* [`45`](https://github.com/uavos/apx-gcs/issues/45) plugins blacklisted on segfault

## Comments

**fix:[`44`](https://github.com/uavos/apx-gcs/issues/44) exchange creating symlink on .xpl plugin**

with copying of it

**fix:[`45`](https://github.com/uavos/apx-gcs/issues/45) add exception check**

to give other plugins chance
to go out of blacklist

**fix:[`45`](https://github.com/uavos/apx-gcs/issues/45) plugins blacklisted on segfault**

replace try-catch with removing plugin
from blacklist while dependency is loading

# [APX Ground Control v11.1.13](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.13) (12/17/23)

> Branch: `main`\
> Date: `12/17/23 17:00:39`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/b4c109c4940b041735ffc0ba3f814f40c538f8a8...31601115bbb32ed7028b73c8671fa2daa19f2af1)

## New Features
* send position fix command for `AHRS`
* system LLVM support for script compiler

## Bug Fixes
* default units precision

# [APX Ground Control v11.1.12](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.12) (11/05/23)

> Branch: `main`\
> Date: `11/05/23 19:00:38`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/fe39810c4d78805213a6e01c6baa931da7f8a5fc...890bfbe7d8a9a88c1597c7c1953090525b965b56)

## Bug Fixes
* datalink server TCP transport between apps

# [APX Ground Control v11.1.11](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.11) (09/15/23)

> Branch: `main`\
> Date: `09/15/23 11:09:32`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/e2f755a458616e70a96277cb47e1963c7cf6aaeb...b0cd14a0758ca5427e7af77262d02abad06856ee)

## New Features
* GNSS Jamming/Spoofing Simulation
* Estimated position error display on map view

## Bug Fixes
* CpuID for Linux
* sim plugin AP link timeouts and UDP reconnect

# [APX Ground Control v11.1.10](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.10) (07/10/23)

> Branch: `main`\
> Date: `07/10/23 20:33:16`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/5e4a5e450873a691878a6187e946fdaddfc2a941...5f68765c2b6a39a4bb8551e08ecc63562fe2f8f2)

## New Features
* TCP datalink uses COBS codec
* UDP datalink server
* UDP for simulator link

## Bug Fixes
* server discovery via UDP

# [APX Ground Control v11.1.9](https://github.com/uavos/apx-gcs/releases/tag/release-11.1.9) (06/21/23)

> Branch: `main`\
> Date: `06/21/23 12:24:54`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/96357916c51c4147ba3fd842eb5605d0a8413d56...5e4a5e450873a691878a6187e946fdaddfc2a941)

## New Features
* WASM console `dump` function
* WASM scripting helpers
* Mandala status display
* telemetry auto sync dataset option
* mandala silent send value method
* multiline node messages
* VCP to SerialPort forwarding plugin

## Bug Fixes
* apxfw versioning and updates
* RSS ground and onboard display
* vscode search paths
* WASM math lib warnings
* TTS config
* mandala report menu URL
* empty telemetry stream support
* telemetry initial values recording
* engine start switch on multi GCS bug
* node download with no files
* mandala send value units conversion
* COBS encoder
* firmware versions handling
* run docker image
* local build by dockerfile
* CI MacOS build
* export vehicle config from `REPLAY`
* mission export title
* SerialPort forwarding plugin
* Qt5.15.10 compatibility (remove itemsoverlay on map)

## Performance Enhancements
* remove group mandala facts from data streams
* telemetry datasets selection

# [APX Ground Control v11.0.221](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.221) (05/20/21)

> Branch: `main`\
> Date: `05/20/21 17:15:21`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/a14bcb5131f7d42477dbceedec712aca1f4b70c4...e9e9e456ac12af6ef61c60baa0d3212021858137)

## New Features
* home point update function
* run guard to detect multiple GCS instances
* multi GCS node parameters updates
* time units editors and delegates
* file sharing available to JS context
* vscode script editor option
* default sim config update
* qml script edit with title field

## Bug Fixes
* runway map item circle coordinate
* mission runway option pack
* nodes config save when updated from remote GCS
* Fact buttons tooltips by press and hold
* POI timeout in minutes
* terminal list views auto scrolling
* sites editor

# [APX Ground Control v11.0.199](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.199) (05/08/21)

> Branch: `main`\
> Date: `05/08/21 16:18:26`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/4254a3cf919d1d4df35bd748ab26be72359ee393...2f85b8fb890eec22324ef47bd7c813904ed22e95)

## New Features
* simulator slip angle simulation
* crosstrack error map display
* taxiway select buttons

## Bug Fixes
* runway shows original circle on landing

# [APX Ground Control v11.0.190](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.190) (05/05/21)

> Branch: `main`\
> Date: `05/05/21 10:47:11`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/810d295fafd4f5180bf7acea4f162f2bdf78ea47...ef884ee32af3cc1d02046064cd72b67dec4f226c)

## New Features
* flyTo JS helper and coordinate type conversion
* docs update

## Bug Fixes
* flyHere command fix
* xplane crash on exit with APX plugin loaded (closes [`13`](https://github.com/uavos/apx-gcs/issues/13))
* docs link in HTTP service
* GCS crashes when connecting joystick
* telemetry byte values unpack handling
* vehicle label size grow bug
* multi-platform font sizes

## Performance Enhancements
* UI fonts and scale factors refactoring
* JS calculations optimizations

# [APX Ground Control v11.0.186](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.186) (05/04/21)

> Branch: `main`\
> Date: `05/04/21 17:41:59`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/810d295fafd4f5180bf7acea4f162f2bdf78ea47...abc3f9e8ff6d3fe1a27bb3ac8909f2605e9b131d)

## New Features
* flyTo JS helper and coordinate type conversion
* docs update

## Bug Fixes
* flyHere command fix
* xplane crash on exit with APX plugin loaded (closes [`13`](https://github.com/uavos/apx-gcs/issues/13))
* docs link in HTTP service
* GCS crashes when connecting joystick
* telemetry byte values unpack handling

## Performance Enhancements
* UI fonts and scale factors refactoring
* JS calculations optimizations

# [APX Ground Control v11.0.156](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.156) (04/20/21)

> Branch: `main`\
> Date: `04/20/21 17:42:53`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/b8006770cebb6185cf73e9c895ef55a5cadbc2cb...1dea988aed478483b83b23c9fdd60279d669d0ed)

## New Features
* WASM examples
* Datalink ESC uart codec
* AHRS validity flags on EFIS
* GYRO and ACC validity flags on EFIS
* airbrakes buttons on landing instrument

## Bug Fixes
* fresh install errors fix

## Performance Enhancements
* central settings file source path
* move all js functions to `gsc.js` file
* database for node parameters meta data

# [APX Ground Control v11.0.154](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.154) (04/20/21)

> Branch: `main`\
> Date: `04/20/21 17:00:50`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/b8006770cebb6185cf73e9c895ef55a5cadbc2cb...3d9a84a53047e9c81dec890fceb4f8e4b14e0c61)

## New Features
* WASM examples
* Datalink ESC uart codec
* AHRS validity flags on EFIS
* GYRO and ACC validity flags on EFIS
* airbrakes buttons on landing instrument

## Bug Fixes
* fresh install errors fix

## Performance Enhancements
* central settings file source path
* move all js functions to `gsc.js` file
* database for node parameters meta data

# [APX Ground Control v11.0.141](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.141) (04/17/21)

> Branch: `main`\
> Date: `04/17/21 16:15:32`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/6400f69dddc848ba57d054b139a188b1259f6c76...29870d2ec82ee4301f7ca367f397182292c17c82)

## New Features
* Node modules tool reset function
* EFIS cursor on controlled areas
* node `alive` property
* Vehicle identity conflicts resolution

## Performance Enhancements
* disable telemetry on upgrading
* node modules addressing through indexes

# [APX Ground Control v11.0.134](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.134) (04/15/21)

> Branch: `main`\
> Date: `04/15/21 16:02:57`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/645a51fc17097c91c21a8d2795657e946a4165f6...d4898dc000ad37a40b180c0b031c4549151ca31e)

## New Features
* OTA firmware upgrade
* vehicles deletion function

## Bug Fixes
* variant data formats parsers

## Performance Enhancements
* plugins load crash protection blacklist

# [APX Ground Control v11.0.128](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.128) (04/13/21)

> Branch: `main`\
> Date: `04/13/21 21:32:54`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/c3f6d7a0f344b37c066d7e8f94c174ea7484ec40...11ea326501522f894ff58a4f5a03b5537a1997de)

## New Features
* long text display elide options
* Datalink Inspector plugin
* telemetry files share
* node modules tool
* default templates import: telemetry and configs
* AP configuration import substitutions
* takeoff condition detector for auto recording

## Bug Fixes
* apxfw package parsing
* error string description
* reserved fact names (closes [`9`](https://github.com/uavos/apx-gcs/issues/9))
* telemetry recorder reset by time shift
* telemetry chart remember shown curves
* js scripting commands and shortcuts
* shortcuts plugin failure

## Performance Enhancements
* http service as plugin
* mandala units conversion for radians

# [APX Ground Control v11.0.126](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.126) (04/13/21)

> Branch: `main`\
> Date: `04/13/21 17:34:19`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/c3f6d7a0f344b37c066d7e8f94c174ea7484ec40...9a69088d88eca14e0d0cc47cebdb6e8df1c8969d)

## New Features
* long text display elide options
* Datalink Inspector plugin
* telemetry files share
* node modules tool
* default templates import: telemetry and configs
* AP configuration import substitutions
* takeoff condition detector for auto recording

## Bug Fixes
* apxfw package parsing
* error string description
* reserved fact names (closes [`9`](https://github.com/uavos/apx-gcs/issues/9))
* telemetry recorder reset by time shift
* telemetry chart remember shown curves
* js scripting commands and shortcuts

## Performance Enhancements
* http service as plugin
* mandala units conversion for radians

# [APX Ground Control v11.0.49](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.49) (03/13/21)

> Branch: `main`\
> Date: `03/13/21 12:18:18`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/356dc7f314153a8402e70f4fe349a072172a1440...cf7cce604973d45e8f9c4e77e02ec934b3f54779)

## New Features
* TECS weighting factor instrument

## Bug Fixes
* linux fullscreen (closes [`1`](https://github.com/uavos/apx-gcs/issues/1))
* fact button anchors and text elide (closes [`4`](https://github.com/uavos/apx-gcs/issues/4))
* fact name restrictions (closes [`5`](https://github.com/uavos/apx-gcs/issues/5))
* terminal autocomplete (closes [`10`](https://github.com/uavos/apx-gcs/issues/10))
* js objects write protection (closes [`12`](https://github.com/uavos/apx-gcs/issues/12))
* apxfw package parsing for firmware updates

# [APX Ground Control v11.0.47](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.47) (03/12/21)

> Branch: `main`\
> Date: `03/12/21 21:38:40`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/356dc7f314153a8402e70f4fe349a072172a1440...4bb8f87b1be031b1ccadbac6f37fb41248016866)

## New Features
* TECS weighting factor instrument

## Bug Fixes
* linux fullscreen (closes [`1`](https://github.com/uavos/apx-gcs/issues/1))
* fact button anchors and text elide (closes [`4`](https://github.com/uavos/apx-gcs/issues/4))
* fact name restrictions (closes [`5`](https://github.com/uavos/apx-gcs/issues/5))
* terminal autocomplete (closes [`10`](https://github.com/uavos/apx-gcs/issues/10))
* js objects write protection (closes [`12`](https://github.com/uavos/apx-gcs/issues/12))
* apxfw package parsing for firmware updates

# [APX Ground Control v11.0.39](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.39) (03/10/21)

> Branch: `main`\
> Date: `03/10/21 21:48:44`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/626cd9a9a72290a6bcf916e002749acc9f3ad20a...db5daba6f55d64ac76e15954e3160df85fb47ada)

## New Features
* mission share formats

## Bug Fixes
* release notes
* linux auto updates
* build cmake issues (closes [`3`](https://github.com/uavos/apx-gcs/issues/3))
* mission storage and export

# [APX Ground Control v11.0.37](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.37) (03/10/21)

> Branch: `main`\
> Date: `03/10/21 20:36:03`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/626cd9a9a72290a6bcf916e002749acc9f3ad20a...6cbf0803b783e6db0cdd1897194de8583323ac42)

## New Features
* mission share formats

## Bug Fixes
* release notes
* linux auto updates
* build cmake issues (closes [`3`](https://github.com/uavos/apx-gcs/issues/3))
* mission storage and export

# [APX Ground Control v11.0.35](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.35) (03/10/21)

> Branch: `main`\
> Date: `03/10/21 15:00:42`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/626cd9a9a72290a6bcf916e002749acc9f3ad20a...30e7e7b2c7577d6ae18c9a47d2603a3e656851f1)

## New Features
* mission share formats

## Bug Fixes
* release notes
* linux auto updates
* build cmake issues (closes [`3`](https://github.com/uavos/apx-gcs/issues/3))
* mission storage and export

# [APX Ground Control v11.0.32](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.32) (03/09/21)

> Branch: `main`\
> Date: `03/09/21 21:58:59`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/626cd9a9a72290a6bcf916e002749acc9f3ad20a...2644b1653be41a79a3e130ac77a255f9bb409da7)

## New Features
* mission share formats

## Bug Fixes
* release notes
* linux auto updates
* build cmake issues (closes [`3`](https://github.com/uavos/apx-gcs/issues/3))
* mission storage and export

# [APX Ground Control v11.0.22](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.22) (02/21/21)

> Branch: `dev`\
> Date: `02/21/21 15:16:41`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/79a97d9018c7e2812766ea1d88fe5b1b260c8027...52295e73f4dd9c6c8b559fe2b94daf263051f77b)

## Bug Fixes
* about dialog info
* xpl plugin crash
* linux SIL locations

## Performance Enhancements
* value text property with no units

# [APX Ground Control v11.0.17](https://github.com/uavos/apx-gcs/releases/tag/release-11.0.17) (02/19/21)

> Branch: `main`\
> Date: `02/19/21 23:18:02`\
> Diff: [uavos/apx-gcs](https://github.com/uavos/apx-gcs/compare/b42c313a266a90c253209b9cf3423ca89f81840c...7a07f00720c443bfd00a00712afd26761fca1b6d)

## New Features
* CI releases
* build instructions
* link to firmware repository

## Bug Fixes
* VideoStreaming GL context on minimal platform
* macos build name
* xplane simulation from APXFW
* pre-release
* SITL simulation
* sparkle nodmg delta files

