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

Pure QML plugins can be installed directly from a GitHub repository without rebuilding the
application. The sources are kept in the user plugins folder:

```
<Documents>/UAVOS/Plugins/github/<repo>/        repository sources (latest release)
<Documents>/UAVOS/Plugins/github/<repo>.json    installed release info (url, tag, plugins)
```

Every `<Name>Plugin.qml` file found in the repository (up to two folder levels deep) is loaded
as the plugin `<Name>`, the same way as the other user plugins: it gets its switch in
`Preferences / Application / Plugins` and is skipped on the next start when switched off.

To install a plugin open `Preferences / Application / Plugins / GitHub plugins`, enter the
repository URL (`https://github.com/<owner>/<repo>`) and press `Install`. The latest release
archive is downloaded (the default branch when the repository has no releases) and checked
before it is installed:

- the repository folder must not exist in `Plugins/github`;
- the repository must contain at least one `<Name>Plugin.qml`;
- no plugin with the same name may already be loaded.

The new plugins are loaded immediately. Installed repositories are listed in the same menu
with their release tag; `Update` downloads a newer release and `Remove` deletes the folder
(a restart is needed to unload plugins that are already running).

Example: [EMIP](https://github.com/Alexandr-one/emip), the engine monitoring and indication panel.
