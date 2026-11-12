---
icon: fas fa-plug
---

# Plugin System

## Overview

The APX GCS uses a flexible plugin architecture that allows extending functionality without modifying core code. This modular approach enables developers to add new features, communication protocols, or UI components while maintaining system stability.

## Plugin Types

### Communication Plugins

Handle different connection types to UAVs:

- Serial communication plugins
- TCP/IP connection plugins  
- UDP network plugins
- Custom protocol implementations

### Data Processing Plugins

Process telemetry and sensor data:

- Data filtering and smoothing
- Protocol conversion
- Data validation
- Statistical analysis

### UI Plugins

Extend user interface capabilities:

- New visualization components
- Custom control panels
- Data display widgets
- Navigation tools

### Mission Plugins  

Handle mission planning and execution:

- Waypoint management
- Mission import/export
- Flight plan optimization
- Safety checks

## Plugin Structure

Each plugin follows a consistent structure:

```
Plugins/
├── MyPlugin/
│   ├── CMakeLists.txt
│   ├── myplugin.cpp
│   ├── myplugin.h
│   ├── qml/
│   │   └── MyPlugin.qml
│   └── resources/
│       └── plugin.json
```

### Plugin Manifest (plugin.json)

```json
{
    "name": "MyPlugin",
    "version": "1.0.0",
    "description": "Description of what this plugin does",
    "author": "Your Name",
    "license": "MIT",
    "type": "communication",
    "dependencies": {
        "apx-gcs": ">=11.0.0"
    }
}
```

## Plugin Registration

Plugins are automatically discovered and registered at application startup:

1. **CMake Integration**: Plugins are added via `add_subdirectory()` in CMakeLists.txt
2. **Dynamic Loading**: Plugins are loaded at runtime using Qt's plugin system  
3. **Dependency Resolution**: Required dependencies are checked automatically
4. **Version Compatibility**: Plugin compatibility is verified

## Development Guidelines

### Creating a New Plugin

1. Create plugin directory structure
2. Implement plugin interface in C++
3. Add QML UI components if needed
4. Register plugin in CMakeLists.txt
5. Test plugin functionality

### Plugin Interface

Plugins must implement the `ApxPlugin` interface:

```cpp
class ApxPlugin : public QObject
{
    Q_OBJECT
public:
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};
```

### Best Practices

- Keep plugins focused on single responsibilities
- Follow naming conventions consistently
- Provide proper error handling
- Include comprehensive documentation
- Test plugin in isolation before integration

## Plugin Management

The system provides runtime management capabilities:

- Enable/disable plugins
- Plugin version checking
- Dependency resolution
- Error reporting and recovery

## Third Party Plugins from GitHub

Plugins can be installed directly from a GitHub repository without rebuilding the
application. Each repository gets a folder in the user plugins folder:

```
<Documents>/UAVOS/Plugins/github/<repo>/<Name>.so       plugin library for this platform
<Documents>/UAVOS/Plugins/github/<repo>/<repo>.json     installed release info (url, tag, asset)
```

The repository keeps the plugin sources (C++ or QML) and its release provides the plugin
library built for every platform as an asset named `<Name>-<os>-<arch>.so|dylib`
(`linux-x86_64`, `macos-universal`, `macos-arm64`). See the
[EMIP](https://github.com/Alexandr-one/emip) repository for the build script and the GitHub
workflow: the plugin folder is copied into `src/Plugins/Tools` of the GCS tree and built with
the regular `apx_plugin()` cmake function, so the same setup works for C++ and QML plugins.

The library links against the GCS libraries, so every plugin release is made for one GCS
version: the release tag is the GCS version plus the plugin build number, `v11.2.32.1` is the
first build for GCS 11.2.32 and `v11.2.32.2` the next one. The GCS loads an installed plugin
only when the version in `<repo>.json` matches its own (the first three numbers); a plugin
built for another version is reported in the console as disabled and is not loaded.

To install a plugin open `Preferences / Application / Plugins / GitHub plugins`, enter the
repository URL (`https://github.com/<owner>/<repo>`) and press `Install`. The GCS reads the
releases, takes the latest one made for its version with a library for its platform and
downloads it. Checks: the repository folder must not exist in `Plugins/github`, a release
for the GCS version with a library for the platform must exist, no plugin with the same name
may be loaded already. The plugin is loaded immediately and behaves like the other user
plugins: it gets a switch in `Preferences / Application / Plugins` and is skipped on the next
start when switched off.

Updates work like the application updates: `Check for updates` (also run automatically a few
seconds after start when `Auto` is on) asks every installed repository for its releases and
opens the `Updates` list when something can be installed. Per plugin the list shows one of:

* `Update available: <tag>` - a newer build for the running GCS version, click to install;
* `Not compatible with version <gcs>, reinstall <tag>` - the installed plugin is built for
  another GCS version (older or newer), click to install the latest build for this version;
* `Not available for version <gcs>` - the repository has no release for this version, the
  plugin stays disabled.

Installed repositories are listed in the same menu with their release tag and status;
`Update` in the entry installs the same release as the list and is disabled when the installed
build is the latest one for this version, `Remove` deletes the folder. A restart is needed to
reload a plugin that is already running; a plugin disabled by the version check is loaded right
after the install.
