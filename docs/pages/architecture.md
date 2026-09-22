---
icon: fas fa-sitemap
---

# System Architecture

## Overview

The APX Ground Control Software (GCS) is a modular, cross-platform application designed for unmanned aerial vehicle (UAV) command and control. Built on Qt6 with C++ and QML, it provides a flexible framework for UAV operations with extensible plugin architecture.

## High-Level Components

### Core Modules

- **ApxCore**: Provides fundamental data structures and communication protocols
- **ApxData**: Handles data management and storage operations  
- **ApxFw**: Firmware-related functionality and updates
- **ApxGcs**: GCS-specific application logic and UI components

### Application Structure

```
src/
├── main/              # Main application entry point
├── lib/               # Core libraries
│   ├── ApxCore/       # Core data model and communication
│   ├── ApxData/       # Data handling and storage
│   ├── ApxFw/         # Firmware management
│   └── ApxGcs/        # GCS application logic
└── Plugins/           # Extensible plugin system
```

## Communication Architecture

### Data Flow

1. **Hardware Interface**: Serial/USB communication with UAVs
2. **Protocol Layer**: Abstraction layer for different communication protocols  
3. **Data Model**: Fact-based data structure (using Mandala pattern)
4. **UI Layer**: QML-based user interface components
5. **Plugin System**: Extensible functionality through plugins

### Protocol Abstraction

The system supports multiple communication protocols through a plugin architecture:

- Serial communication
- TCP/IP connections  
- NMEA protocol support
- Custom UAV protocols

## Plugin System

The GCS uses a flexible plugin architecture that allows extending functionality without modifying core code:

### Plugin Types

- **Communication Plugins**: Handle different connection types
- **Data Processing Plugins**: Process telemetry data  
- **UI Plugins**: Extend user interface capabilities
- **Mission Plugins**: Handle mission planning and execution

### Plugin Integration

Plugins are dynamically loaded at runtime, providing:

- Easy extension without recompilation
- Version compatibility handling
- Runtime plugin management

## Data Model Architecture

The system uses a Fact-based data model pattern (Mandala) for consistent data handling:

### Key Concepts

- **Facts**: Atomic data elements with type information
- **Fact Trees**: Hierarchical organization of facts  
- **Fact Properties**: Metadata and validation rules
- **Fact Events**: Change notifications and callbacks

This architecture enables:

- Consistent data representation across components
- Easy data binding in UI layer
- Extensible data types
- Real-time data synchronization
