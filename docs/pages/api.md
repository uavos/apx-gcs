---
icon: fas fa-code
---

# API Documentation

## Overview

This section documents the public APIs available in the APX GCS system. The documentation is organized by module to help developers understand how to extend and integrate with the system.

## Core Modules

### ApxCore

The core module provides fundamental data structures and communication protocols:

- **Fact System**: Hierarchical data model for UAV telemetry and configuration
- **Communication Protocols**: Base interfaces for different connection types  
- **Event System**: Asynchronous notifications and callbacks

### ApxData

Handles data management and storage operations:

- **Data Storage**: Persistent storage of mission plans, logs, and settings
- **Data Conversion**: Format conversion utilities
- **Data Validation**: Input validation and error handling

### ApxFw

Firmware-related functionality:

- **Update Management**: Firmware update processes
- **Version Control**: Firmware version handling
- **Compatibility Checks**: Version compatibility verification

### ApxGcs

GCS-specific application logic:

- **Application State**: Main application lifecycle management
- **UI Components**: QML component interfaces
- **Plugin Interface**: Plugin system integration

## Plugin API

The plugin system provides extension points for custom functionality:

- **Plugin Registration**: How to register and load plugins
- **Communication Interfaces**: Protocol implementation guidelines  
- **Data Processing**: Data transformation and analysis APIs
- **UI Extension**: QML component integration

## Development Resources

### Build System Integration

How to integrate with CMake build system:

- Plugin inclusion in CMakeLists.txt
- Dependency management
- Cross-platform compilation

### Testing Framework

Guidelines for testing plugins and core components:

- Unit test structure
- Integration test patterns
- Continuous integration setup

## Future Documentation

This API documentation will be expanded to include:

- Detailed function signatures and parameters
- Code examples and usage patterns
- Complete class hierarchies
- Diagrams showing system interactions
