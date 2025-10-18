# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial release of Parallax Windows CLI
- Comprehensive environment checking and installation
- WSL2 integration with real-time output
- CUDA toolkit detection and validation
- Modern C++ architecture with CRTP patterns
- Multi-language support (English/Chinese)

### Features
- `parallax check` - Environment requirements checking
- `parallax install` - Automated component installation  
- `parallax config` - Configuration management
- `parallax run` - Direct WSL inference server execution
- `parallax join` - Distributed cluster node joining
- `parallax cmd` - WSL command pass-through

### System Support
- Windows 10 Version 2004+ and Windows 11
- NVIDIA GPU support (RTX 3060 Ti+)
- WSL2 and Ubuntu integration
- CUDA Toolkit 12.8/12.9 compatibility

## [1.0.0] - 2025-10-18

### Added
- Initial public release
- Complete development environment setup automation
- Professional Windows installer with NSIS
- Comprehensive documentation and examples
- Apache 2.0 license for open source distribution

### Architecture
- Modular component-based design
- Template method pattern for commands
- CRTP for compile-time polymorphism
- Separation of concerns across modules

### Documentation
- English and Chinese README files
- Contributing guidelines
- Security policy
- GitHub issue and PR templates

---

## Release Notes Format

Each release should include:
- **Added**: New features
- **Changed**: Changes in existing functionality  
- **Deprecated**: Soon-to-be removed features
- **Removed**: Removed features
- **Fixed**: Bug fixes
- **Security**: Security improvements
