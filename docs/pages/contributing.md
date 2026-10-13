---
icon: fas fa-hands-helping
---

# Contributing to APX GCS

Welcome to the APX Ground Control Software project! We appreciate your interest in contributing to this open-source UAV control system.

## How to Contribute

### Reporting Issues

- Use GitHub Issues to report bugs or request features
- Include detailed information about your environment and steps to reproduce
- Provide logs, screenshots, or code examples when relevant
- Check existing issues before creating new ones

### Submitting Pull Requests

1. Fork the repository
2. Create a feature branch from `main`
3. Make your changes with clear commit messages
4. Ensure all tests pass
5. Submit a pull request with detailed description

### Code Style Guidelines

- Follow existing code patterns and conventions
- Use descriptive variable and function names
- Include comments for complex logic
- Maintain consistent formatting
- Write unit tests for new functionality

## Development Setup

### Prerequisites

- Qt6 development tools (minimum 6.5)
- CMake 3.19 or higher
- Ninja build system
- Python3 for build scripts
- Git for version control

### Building the Project

```bash
git clone --recurse-submodules https://github.com/uavos/apx-gcs.git
cd apx-gcs
cmake -H. -Bbuild -G Ninja
cmake --build build
```

## Code Structure

### Core Modules

- `src/lib/ApxCore/` - Core data model and communication protocols  
- `src/lib/ApxData/` - Data handling and storage components
- `src/lib/ApxFw/` - Firmware management functionality
- `src/lib/ApxGcs/` - GCS-specific application logic

### Plugin Architecture

The system uses a plugin-based architecture that allows extending functionality without modifying core code.

## Testing

We encourage contributors to write tests for new features:

- Unit tests for core components
- Integration tests for plugin systems  
- UI tests where applicable
- Regression tests to prevent breaking changes

## Documentation

All contributions should include updated documentation:

- Code comments for new functions/methods
- API documentation for public interfaces
- User guides for new features
- Architecture documentation for significant changes

## Community Guidelines

### Code of Conduct

This project follows the standard open-source community guidelines:

- Be respectful and considerate
- Focus on constructive feedback
- Help others learn and grow
- Follow project maintainers' decisions

### Communication

- Use GitHub Issues for discussions
- Engage with the community through pull requests
- Ask questions in appropriate channels
- Share your experiences and improvements

## License

By contributing to this project, you agree that your contributions will be licensed under the MIT license included in the repository.

## Questions?

If you have any questions about contributing or development, please:

1. Check existing issues and documentation
2. Open a new issue for clarification
3. Reach out through GitHub discussions
