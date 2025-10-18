# Parallax Windows CLI

中文版 | [English](README.md)

Parallax Windows CLI 是一个用于 Windows 平台的开发环境配置工具，提供 WSL2、CUDA 工具包、开发工具等的自动检测和安装。该项目采用现代 C++ 架构，通过 NSIS 制作的 Windows 安装包实现一键部署，支持 WSL2 + Ubuntu 的开发环境配置。

## 功能特性

### 1. 环境管理
- **自动检查**: 全面检查 Windows 环境、WSL2、NVIDIA 驱动等组件
- **一键安装**: 自动安装和配置 WSL2、CUDA工具包、开发工具等
- **智能检测**: 支持 NVIDIA GPU 检测和 CUDA 工具包检查
- **系统要求**: 最低支持 RTX 3060 Ti，推荐 RTX 4090 或更高配置

### 2. 环境自动配置
- **智能检测**: 自动检测系统环境和硬件配置
- **一键安装**: 自动安装WSL2、CUDA工具包、开发工具等
- **GPU 支持**: 完整的 NVIDIA GPU 检测和驱动验证
- **实时反馈**: 支持实时查看安装进度和状态

### 3. WSL2 集成
- **命令执行**: 完整的 WSL2 命令执行支持
- **实时输出**: WSL 命令执行的实时 stdout/stderr 输出
- **中断支持**: 支持 Ctrl+C 中断正在运行的 WSL 命令
- **代理同步**: 自动同步系统代理配置到 WSL 环境

### 4. 配置管理
- **灵活配置**: 支持代理、WSL 发行版、安装源等多种配置
- **动态更新**: 配置变更自动同步到相关组件
- **重置功能**: 一键重置所有配置到默认状态
- **安全存储**: 配置文件安全存储和访问控制

### 5. 现代架构设计
- **模板方法模式**: 统一的命令执行流程和错误处理
- **CRTP 技术**: 编译时多态，零运行时开销
- **命令模式**: 可扩展的命令注册和执行机制
- **策略模式**: 灵活的环境要求和检查策略

## 架构设计

Parallax Windows CLI 采用现代 C++ 设计模式，主要包含以下核心组件：

### 1. 命令行接口
- **位置**: `src/parallax/cli/`
- **功能**:
  - 统一的命令解析和执行框架
  - 基于模板方法模式的命令基类
  - 支持参数验证、环境准备、执行流程的标准化
- **支持命令**: check、install、config、run、join、cmd

### 2. 环境安装器
- **位置**: `src/parallax/environment/`
- **架构特点**:
  - 采用模块化设计，职责分离
  - 组合模式管理多个专门组件
  - 提供统一的检查和安装接口
- **核心组件**:
  - `base_component`: 基础接口和执行上下文
  - `command_executor`: 统一的命令执行器
  - `system_checker`: 系统检查器（OS版本、NVIDIA GPU、NVIDIA驱动、BIOS虚拟化）
  - `windows_feature_manager`: Windows功能管理器（WSL2、虚拟机平台、WSL包、WSL2内核、Ubuntu）
  - `software_installer`: 软件安装器（CUDA工具包检查、Rust Cargo、Ninja构建工具、pip升级、Parallax项目）
- **状态管理**: 支持成功、失败、跳过、警告四种状态

### 3. 配置管理器
- **位置**: `src/parallax/config/`
- **功能**:
  - 统一的配置文件管理
  - 支持动态配置加载和更新
  - 配置验证和默认值处理

### 4. 工具库
- **位置**: `src/parallax/utils/`
- **功能**:
  - `utils`: WSL命令构建、字符串转换、GPU检测、文件操作
  - `process`: 进程管理和实时输出处理，支持回调和中断
  - `wsl_process`: WSL专用进程管理，支持实时输出和编码转换

### 5. 日志系统
- **位置**: `src/parallax/tinylog/`
- **功能**:
  - 高性能的异步日志系统
  - 支持日志轮转和大小限制
  - 多级别日志输出和过滤

## 环境要求

### 系统要求
- **操作系统**: Windows 10 Version 2004 (Build 19041) 或 Windows 11
- **架构**: x86_64
- **权限**: 管理员权限（用于 WSL2 和 Docker 安装）
- **内存**: 最低 16GB RAM，推荐 32GB 或更高
- **存储**: 最低 50GB 可用空间

### 硬件要求
- **GPU**: NVIDIA RTX 3060 Ti 或更高配置
- **显存**: 最低 8GB VRAM，推荐 24GB 或更高
- **CPU**: Intel i5-8400 或 AMD Ryzen 5 2600 及以上（建议）
- **网络**: 稳定的互联网连接（用于下载镜像和模型）

### 软件依赖
- **WSL2**: Windows Subsystem for Linux 2
- **Ubuntu**: WSL2上的Ubuntu发行版（默认Ubuntu-24.04）
- **NVIDIA 驱动**: 支持 CUDA 12.x 的最新驱动
- **CUDA 工具包**: CUDA Toolkit 12.8或12.9版本（需要预先安装）
- **Rust Cargo**: Rust包管理器和构建工具
- **Ninja**: 快速构建工具

## 快速开始

### 1. 下载安装
从 Release 页面下载最新的安装包：
```
Gradient_Parallax_PC_Setup_v1.0.0.0.exe
```

### 2. 环境检查
```cmd
parallax check
```

### 3. 环境安装（如需要）
```cmd
parallax install
```

### 4. 配置代理（可选）
```cmd
parallax config proxy_url "http://127.0.0.1:7890"
```

### 5. 验证安装
```cmd
# 验证所有环境组件
parallax check

# 启动Parallax推理服务器（可选）
parallax run
```

## 命令参考

### `parallax check`
检查环境要求和组件状态
```cmd
parallax check [--help|-h]
```

### `parallax install`
安装必需的环境组件
```cmd
parallax install [--help|-h]
```

### `parallax config`
配置管理命令
```cmd
# 查看所有配置
parallax config list

# 设置配置项
parallax config <key> <value>

# 删除配置项
parallax config <key> ""

# 重置所有配置
parallax config reset

# 查看帮助
parallax config --help
```

### `parallax run`
直接在WSL中运行Parallax推理服务器
```cmd
parallax run [--help|-h]
```

### `parallax join`
加入分布式推理集群作为节点
```cmd
parallax join <coordinator_url> [options]
```

### `parallax cmd`
在WSL或Python虚拟环境中执行命令
```cmd
parallax cmd [--venv] <command> [args...]
```

**命令说明**:
- `run`: 直接在WSL中启动Parallax推理服务器，默认配置为Qwen/Qwen3-0.6B模型，监听localhost:3001
- `join`: 加入分布式推理集群作为工作节点(管理员权限新开一个终端输入parallax join)
- `cmd`: 透传命令到WSL环境执行，支持`--venv`选项在parallax project的Python虚拟环境中运行

**主要配置项**:
- `proxy_url`: 网络代理地址（支持 http、socks5、socks5h）
- `wsl_linux_distro`: WSL Linux 发行版（默认 Ubuntu-24.04）
- `wsl_installer_url`: WSL安装包下载地址
- `wsl_kernel_url`: WSL2内核更新包下载地址
- `parallax_git_repo_url`: Parallax项目Git仓库地址（默认：https://github.com/GradientHQ/parallax.git）


## 构建说明

### 开发环境
- **Visual Studio 2022**: 带 C++ 桌面开发工作负载
- **CMake 3.20+**: 用于构建 C++ 项目
- **NSIS 3.08+**: 用于创建 Windows 安装包

### 快速构建
在项目根目录执行：
```cmd
cd src
mkdir build && cd build
cmake ../parallax -A x64
cmake --build . --config Release
```

生成的可执行文件位于：`src/build/x64/Release/parallax.exe`

### 创建安装包
```cmd
# 1. 创建安装文件目录
mkdir installer\FilesToInstall

# 2. 复制生成的可执行文件
copy src\build\x64\Release\parallax.exe installer\FilesToInstall\

# 3. 构建安装包
call installer\build-nim-nozip.bat
```

## 配置说明

Parallax 通过配置文件进行配置，主要配置项包括：

```conf
# WSL 相关配置
wsl_installer_url=https://github.com/microsoft/WSL/releases/download/2.4.13/wsl.2.4.13.0.x64.msi
wsl_kernel_url=https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_x64.msi
wsl_linux_distro=Ubuntu-24.04

# Parallax 项目配置
parallax_git_repo_url=https://github.com/GradientHQ/parallax.git

# 网络代理配置（可选）
proxy_url=http://127.0.0.1:7890
```

## 故障排除

### 常见问题

1. **环境检查失败**
   - 确保以管理员权限运行命令
   - 检查 Windows 版本是否满足要求
   - 验证 NVIDIA GPU 和驱动安装

2. **WSL2 安装失败**
   - 启用 Windows 功能：虚拟机平台和 WSL
   - 检查 BIOS 中是否启用虚拟化
   - 确保系统更新到最新版本

3. **CUDA 工具包检查失败**
   - 检查 NVIDIA 驱动是否正确安装
   - 确认 CUDA Toolkit 是否已正确安装
   - 验证 CUDA 版本是否符合要求（12.8或12.9）

4. **Parallax 项目安装失败**
   - 检查 Git 仓库访问权限
   - 验证代理配置是否正确
   - 确保 WSL2 中有足够的存储空间

5. **网络连接问题**
   - 配置正确的代理设置
   - 检查防火墙和网络策略
   - 验证软件包仓库访问权限

### 日志查看

- **主程序日志**: `parallax.log`（程序运行目录: C:\Program Files (x86)\Gradient Parallax\parallax.log）
- **详细调试**: 查看命令执行的详细输出

### 诊断命令

```cmd
# 检查环境状态（推荐）
parallax check

# 查看详细配置
parallax config list

# 启动推理服务器测试
parallax run

# 在WSL中执行命令
parallax cmd "python --version"

# 如需手动验证特定组件（可选）
wsl --list --verbose    # WSL状态
nvidia-smi             # GPU状态
```


## 技术特性

### 实时输出支持
- 所有长时间运行的命令都支持实时输出
- 支持 Ctrl+C 中断操作
- 自动处理输出编码转换

### 代理配置自动同步
- 自动检测代理配置变更
- WSL 网络环境适配

### GPU 智能检测
- 自动检测 NVIDIA GPU 型号
- 验证 GPU 驱动和 CUDA 兼容性
- 支持主流 NVIDIA GPU 架构

### 错误处理
- 完善的环境检查机制
- 详细的错误信息和建议
- 优雅的失败恢复处理

## 开发指南

### 添加新命令

1. 在 `src/parallax/cli/commands/` 目录下创建新的命令类
2. 继承适当的基类（BaseCommand、WSLCommand、AdminCommand）
3. 实现必需的虚函数
4. 在 `command_parser.cpp` 中注册新命令

```cpp
// 示例：添加新的检查命令
class NewCheckCommand : public WSLCommand<NewCheckCommand> {
public:
    std::string GetName() const override { return "newcheck"; }
    std::string GetDescription() const override { 
        return "New check functionality"; 
    }
    
    CommandResult ExecuteImpl(const CommandContext& context) {
        // 实现具体逻辑
        return CommandResult::Success;
    }
};
```

### 扩展环境检查

1. 根据功能类型选择合适的组件类（SystemChecker、WindowsFeatureManager、SoftwareInstaller）
2. 继承 `BaseEnvironmentComponent` 并实现 `IEnvironmentComponent` 接口
3. 在主 `EnvironmentInstaller` 中注册新组件
4. 更新相关的错误处理和用户提示

```cpp
// 示例：添加新的系统检查组件
class NewSystemChecker : public BaseEnvironmentComponent {
public:
    explicit NewSystemChecker(std::shared_ptr<ExecutionContext> context);
    
    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;
};
```

### 添加配置项

1. 在 `config_manager.h` 中定义新的配置键
2. 在 `config_manager.cpp` 中添加默认值和验证逻辑
3. 更新配置命令的帮助信息

## 目录结构

```
parallax_win/
├── src/
│   └── parallax/
│       ├── cli/                    # 命令行接口
│       │   ├── commands/          # 具体命令实现
│       │   │   ├── base_command.h
│       │   │   ├── check_command.cpp/.h
│       │   │   ├── install_command.cpp/.h
│       │   │   ├── config_command.cpp/.h
│       │   │   ├── cmd_command.cpp/.h
│       │   │   └── model_commands.cpp/.h  # run和join命令
│       │   ├── command_parser.h   # 命令解析器
│       │   └── command_parser.cpp
│       ├── environment/           # 环境安装器（重构后模块化）
│       │   ├── environment_installer.h/.cpp      # 主控制器
│       │   ├── base_component.h/.cpp             # 基础接口和上下文
│       │   ├── command_executor.h/.cpp           # 命令执行器
│       │   ├── system_checker.h/.cpp             # 系统检查器
│       │   ├── windows_feature_manager.h/.cpp    # Windows功能管理器
│       │   ├── windows_feature_manager2.cpp      # Windows功能管理器（续）
│       │   ├── software_installer.h/.cpp         # 软件安装器
│       │   └── software_installer2.cpp           # 软件安装器（续）
│       ├── config/                # 配置管理
│       │   ├── config_manager.h
│       │   └── config_manager.cpp
│       ├── utils/                 # 工具库
│       │   ├── utils.h/.cpp       # 通用工具函数
│       │   ├── process.h/.cpp     # 进程管理
│       │   └── wsl_process.h/.cpp # WSL进程管理
│       ├── tinylog/               # 日志系统
│       │   ├── tinylog.h
│       │   └── tinylog.cpp
│       ├── main.cpp               # 程序入口
│       └── CMakeLists.txt         # 构建配置
├── installer/                     # NSIS 安装包
│   ├── SetupScripts/
│   └── Output/
├── parallax_config.txt            # 默认配置
└── README.md                      # 本文档
```


## 许可证

请参考 LICENSE 文件了解许可证信息。

## 贡献指南

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交代码变更 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 联系方式

如有问题或建议，请通过以下方式联系：

- 创建 Issue 报告问题
- 提交 Pull Request 贡献代码
- 联系开发团队获取支持

---

**注意**: Parallax 是一个高性能的分布式推理框架，专为 NVIDIA GPU 优化。使用前请确保您的硬件配置满足最低要求，并正确安装了相关驱动程序。 