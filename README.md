# Parallax Windows CLI

Parallax Windows CLI is a development environment configuration tool for Windows platform, providing automatic detection and installation of WSL2, CUDA toolkit, development tools, and more. This project adopts modern C++ architecture and implements one-click deployment through NSIS-built Windows installer, supporting WSL2 + Ubuntu development environment configuration.

## Features

### 1. Environment Management
- **Automatic Check**: Comprehensive checking of Windows environment, WSL2, NVIDIA drivers, and other components
- **One-Click Installation**: Automatic installation and configuration of WSL2, CUDA toolkit, development tools, etc.
- **Smart Detection**: Support for NVIDIA GPU detection and CUDA toolkit checking
- **System Requirements**: Minimum support for RTX 3060 Ti, recommended RTX 4090 or higher configuration

### 2. Automated Environment Configuration
- **Smart Detection**: Automatic detection of system environment and hardware configuration
- **One-Click Installation**: Automatic installation of WSL2, CUDA toolkit, development tools, etc.
- **GPU Support**: Complete NVIDIA GPU detection and driver verification
- **Real-time Feedback**: Support for real-time viewing of installation progress and status

### 3. WSL2 Integration
- **Command Execution**: Complete WSL2 command execution support
- **Real-time Output**: Real-time stdout/stderr output for WSL command execution
- **Interrupt Support**: Support for Ctrl+C interruption of running WSL commands
- **Proxy Synchronization**: Automatic synchronization of system proxy configuration to WSL environment

### 4. Configuration Management
- **Flexible Configuration**: Support for multiple configurations including proxy, WSL distribution, installation sources, etc.
- **Dynamic Updates**: Configuration changes automatically synchronized to related components
- **Reset Function**: One-click reset of all configurations to default state
- **Secure Storage**: Secure storage and access control for configuration files

### 5. Modern Architecture Design
- **Template Method Pattern**: Unified command execution flow and error handling
- **CRTP Technology**: Compile-time polymorphism with zero runtime overhead
- **Command Pattern**: Extensible command registration and execution mechanism
- **Strategy Pattern**: Flexible environment requirements and checking strategies

## Architecture Design

Parallax Windows CLI adopts modern C++ design patterns and mainly contains the following core components:

### 1. Command Line Interface
- **Location**: `src/parallax/cli/`
- **Functions**:
  - Unified command parsing and execution framework
  - Command base class based on template method pattern
  - Support for standardized parameter validation, environment preparation, and execution flow
- **Supported Commands**: check, install, config, run, join, cmd

### 2. Environment Installer
- **Location**: `src/parallax/environment/`
- **Architecture Features**:
  - Modular design with separation of concerns
  - Composite pattern managing multiple specialized components
  - Unified checking and installation interface
- **Core Components**:
  - `base_component`: Basic interface and execution context
  - `command_executor`: Unified command executor
  - `system_checker`: System checker (OS version, NVIDIA GPU, NVIDIA driver, BIOS virtualization)
  - `windows_feature_manager`: Windows feature manager (WSL2, virtual machine platform, WSL package, WSL2 kernel, Ubuntu)
  - `software_installer`: Software installer (CUDA toolkit check, Rust Cargo, Ninja build tool, pip upgrade, Parallax project)
- **State Management**: Support for success, failure, skip, and warning states

### 3. Configuration Manager
- **Location**: `src/parallax/config/`
- **Functions**:
  - Unified configuration file management
  - Support for dynamic configuration loading and updating
  - Configuration validation and default value handling

### 4. Utility Library
- **Location**: `src/parallax/utils/`
- **Functions**:
  - `utils`: WSL command building, string conversion, GPU detection, file operations
  - `process`: Process management and real-time output handling, supporting callbacks and interruption
  - `wsl_process`: WSL-specific process management, supporting real-time output and encoding conversion

### 5. Logging System
- **Location**: `src/parallax/tinylog/`
- **Functions**:
  - High-performance asynchronous logging system
  - Support for log rotation and size limits
  - Multi-level log output and filtering

## System Requirements

### System Requirements
- **Operating System**: Windows 10 Version 2004 (Build 19041) or Windows 11
- **Architecture**: x86_64
- **Permissions**: Administrator privileges (for WSL2 and Docker installation)
- **Memory**: Minimum 16GB RAM, recommended 32GB or higher
- **Storage**: Minimum 50GB available space

### Hardware Requirements
- **GPU**: NVIDIA RTX 3060 Ti or higher configuration
- **VRAM**: Minimum 8GB VRAM, recommended 24GB or higher
- **CPU**: Intel i5-8400 or AMD Ryzen 5 2600 and above (recommended)
- **Network**: Stable internet connection (for downloading images and models)

### Software Dependencies
- **WSL2**: Windows Subsystem for Linux 2
- **Ubuntu**: Ubuntu distribution on WSL2 (default Ubuntu-24.04)
- **NVIDIA Driver**: Latest driver supporting CUDA 12.x
- **CUDA Toolkit**: CUDA Toolkit 12.8 or 12.9 version (needs to be pre-installed)
- **Rust Cargo**: Rust package manager and build tool
- **Ninja**: Fast build tool

## Quick Start

### 1. Download and Install
Download the latest installer from the Release page:
```
Gradient_Parallax_PC_Setup_v1.0.0.0.exe
```

### 2. Environment Check
```cmd
parallax check
```

### 3. Environment Installation (if needed)
```cmd
parallax install
```

### 4. Configure Proxy (optional)
```cmd
parallax config proxy_url "http://127.0.0.1:7890"
```

### 5. Verify Installation
```cmd
# Verify all environment components
parallax check

# Start Parallax inference server (optional)
parallax run
```

## Command Reference

### `parallax check`
Check environment requirements and component status
```cmd
parallax check [--help|-h]
```

### `parallax install`
Install required environment components
```cmd
parallax install [--help|-h]
```

### `parallax config`
Configuration management command
```cmd
# View all configurations
parallax config list

# Set configuration item
parallax config <key> <value>

# Delete configuration item
parallax config <key> ""

# Reset all configurations
parallax config reset

# View help
parallax config --help
```

### `parallax run`
Run Parallax inference server directly in WSL
```cmd
parallax run [args...]
```

### `parallax join`
Join distributed inference cluster as a node
```cmd
parallax join [args...]
```

### `parallax cmd`
Execute commands in WSL or Python virtual environment
```cmd
parallax cmd [--venv] <command> [args...]
```

**Command Descriptions**:
- `run`: Start Parallax inference server directly in WSL. You can pass any arguments supported by `parallax run` command. Examples: `parallax run -m Qwen/Qwen3-0.6B`, `parallax run --port 8080`
- `join`: Join distributed inference cluster as a worker node. You can pass any arguments supported by `parallax join` command. Examples: `parallax join -m Qwen/Qwen3-0.6B`, `parallax join -s scheduler-addr`
- `cmd`: Pass-through commands to WSL environment, supports `--venv` option to run in parallax project's Python virtual environment

**Main Configuration Items**:
- `proxy_url`: Network proxy address (supports http, socks5, socks5h)
- `wsl_linux_distro`: WSL Linux distribution (default Ubuntu-24.04)
- `wsl_installer_url`: WSL installer download URL
- `wsl_kernel_url`: WSL2 kernel update package download URL
- `parallax_git_repo_url`: Parallax project Git repository URL (default: https://github.com/GradientHQ/parallax.git)

## Build Instructions

### Development Environment
- **Visual Studio 2022**: With C++ desktop development workload
- **CMake 3.20+**: For building C++ projects
- **NSIS 3.08+**: For creating Windows installer

### Quick Build
Execute in project root directory:
```cmd
cd src
mkdir build && cd build
cmake ../parallax -A x64
cmake --build . --config Release
```

Generated executable is located at: `src/build/x64/Release/parallax.exe`

### Create Installer
```cmd
# 1. Create installation file directory
mkdir installer\FilesToInstall

# 2. Copy generated executable
copy src\build\x64\Release\parallax.exe installer\FilesToInstall\

# 3. Build installer
call installer\build-nim-nozip.bat
```

## Configuration

Parallax is configured through configuration files, main configuration items include:

```conf
# WSL related configuration
wsl_installer_url=https://github.com/microsoft/WSL/releases/download/2.4.13/wsl.2.4.13.0.x64.msi
wsl_kernel_url=https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_x64.msi
wsl_linux_distro=Ubuntu-24.04

# Parallax project configuration
parallax_git_repo_url=https://github.com/GradientHQ/parallax.git

# Network proxy configuration (optional)
proxy_url=http://127.0.0.1:7890
```

## Troubleshooting

### Common Issues

1. **Environment Check Failed**
   - Ensure running commands with administrator privileges
   - Check if Windows version meets requirements
   - Verify NVIDIA GPU and driver installation

2. **WSL2 Installation Failed**
   - Enable Windows features: Virtual Machine Platform and WSL
   - Check if virtualization is enabled in BIOS
   - Ensure system is updated to the latest version

3. **CUDA Toolkit Check Failed**
   - Check if NVIDIA driver is correctly installed
   - Confirm CUDA Toolkit is correctly installed
   - Verify CUDA version meets requirements (12.8 or 12.9)

4. **Parallax Project Installation Failed**
   - Check Git repository access permissions
   - Verify proxy configuration is correct
   - Ensure sufficient storage space in WSL2

5. **Network Connection Issues**
   - Configure correct proxy settings
   - Check firewall and network policies
   - Verify software package repository access permissions

### Log Viewing

- **Main Program Log**: `parallax.log` (Program running directory: C:\Program Files (x86)\Gradient Parallax\parallax.log)
- **Detailed Debugging**: View detailed output of command execution

### Diagnostic Commands

```cmd
# Check environment status (recommended)
parallax check

# View detailed configuration
parallax config list

# Start inference server test
parallax run

# Execute commands in WSL
parallax cmd "python --version"

# Manual verification of specific components (optional)
wsl --list --verbose    # WSL status
nvidia-smi             # GPU status
```

## Technical Features

### Real-time Output Support
- All long-running commands support real-time output
- Support for Ctrl+C interruption
- Automatic handling of output encoding conversion

### Automatic Proxy Configuration Synchronization
- Automatic detection of proxy configuration changes
- WSL network environment adaptation

### Smart GPU Detection
- Automatic detection of NVIDIA GPU models
- Verification of GPU driver and CUDA compatibility
- Support for mainstream NVIDIA GPU architectures

### Error Handling
- Comprehensive environment checking mechanism
- Detailed error messages and suggestions
- Graceful failure recovery handling

## Development Guide

### Adding New Commands

1. Create new command class in `src/parallax/cli/commands/` directory
2. Inherit appropriate base class (BaseCommand, WSLCommand, AdminCommand)
3. Implement required virtual functions
4. Register new command in `command_parser.cpp`

```cpp
// Example: Adding new check command
class NewCheckCommand : public WSLCommand<NewCheckCommand> {
public:
    std::string GetName() const override { return "newcheck"; }
    std::string GetDescription() const override { 
        return "New check functionality"; 
    }
    
    CommandResult ExecuteImpl(const CommandContext& context) {
        // Implement specific logic
        return CommandResult::Success;
    }
};
```

### Extending Environment Checks

1. Choose appropriate component class based on functionality type (SystemChecker, WindowsFeatureManager, SoftwareInstaller)
2. Inherit `BaseEnvironmentComponent` and implement `IEnvironmentComponent` interface
3. Register new component in main `EnvironmentInstaller`
4. Update related error handling and user prompts

```cpp
// Example: Adding new system check component
class NewSystemChecker : public BaseEnvironmentComponent {
public:
    explicit NewSystemChecker(std::shared_ptr<ExecutionContext> context);
    
    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;
};
```

### Adding Configuration Items

1. Define new configuration keys in `config_manager.h`
2. Add default values and validation logic in `config_manager.cpp`
3. Update help information for configuration commands

## Directory Structure

```
parallax_win/
├── src/
│   └── parallax/
│       ├── cli/                    # Command line interface
│       │   ├── commands/          # Specific command implementations
│       │   │   ├── base_command.h
│       │   │   ├── check_command.cpp/.h
│       │   │   ├── install_command.cpp/.h
│       │   │   ├── config_command.cpp/.h
│       │   │   ├── cmd_command.cpp/.h
│       │   │   └── model_commands.cpp/.h  # run and join commands
│       │   ├── command_parser.h   # Command parser
│       │   └── command_parser.cpp
│       ├── environment/           # Environment installer (modularized after refactoring)
│       │   ├── environment_installer.h/.cpp      # Main controller
│       │   ├── base_component.h/.cpp             # Basic interface and context
│       │   ├── command_executor.h/.cpp           # Command executor
│       │   ├── system_checker.h/.cpp             # System checker
│       │   ├── windows_feature_manager.h/.cpp    # Windows feature manager
│       │   ├── windows_feature_manager2.cpp      # Windows feature manager (continued)
│       │   ├── software_installer.h/.cpp         # Software installer
│       │   └── software_installer2.cpp           # Software installer (continued)
│       ├── config/                # Configuration management
│       │   ├── config_manager.h
│       │   └── config_manager.cpp
│       ├── utils/                 # Utility library
│       │   ├── utils.h/.cpp       # General utility functions
│       │   ├── process.h/.cpp     # Process management
│       │   └── wsl_process.h/.cpp # WSL process management
│       ├── tinylog/               # Logging system
│       │   ├── tinylog.h
│       │   └── tinylog.cpp
│       ├── main.cpp               # Program entry point
│       └── CMakeLists.txt         # Build configuration
├── installer/                     # NSIS installer
│   ├── SetupScripts/
│   └── Output/
├── parallax_config.txt            # Default configuration
└── README.md                      # This document
```

## License

Please refer to the LICENSE file for license information.

## Contributing

1. Fork this project
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Create a Pull Request

## Contact

If you have any questions or suggestions, please contact us through:

- Create an Issue to report problems
- Submit a Pull Request to contribute code
- Contact the development team for support

---

**Note**: Parallax is a high-performance distributed inference framework optimized for NVIDIA GPUs. Please ensure your hardware configuration meets the minimum requirements and that relevant drivers are correctly installed before use.
