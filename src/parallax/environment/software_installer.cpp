#include "software_installer.h"
#include "environment_installer.h"
#include "config/config_manager.h"
#include "utils/wsl_process.h"
#include "utils/utils.h"
#include "tinylog/tinylog.h"
#include <algorithm>

namespace parallax {
namespace environment {

// CudaToolkitInstaller implementation
CudaToolkitInstaller::CudaToolkitInstaller(
    std::shared_ptr<ExecutionContext> context,
    std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult CudaToolkitInstaller::Check() {
    LogOperationStart("Checking");

    bool cuda_installed = IsCudaToolkitInstalled();

    ComponentResult result =
        cuda_installed
            ? CreateSkippedResult("CUDA Toolkit 12.8 is already installed")
            : CreateFailureResult("CUDA Toolkit 12.8 is not installed", 21);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult CudaToolkitInstaller::Install() {
    LogOperationStart("Installing");

    // Check if CUDA Toolkit is installed
    if (IsCudaToolkitInstalled()) {
        ComponentResult result =
            CreateSkippedResult("CUDA Toolkit 12.8 is already installed");
        LogOperationResult("Installing", result);
        return result;
    }

    info_log("[ENV] Installing CUDA Toolkit 12.8 in WSL...");

    // Install command sequence (step name, command, timeout seconds, use
    // real-time output)
    std::vector<std::tuple<std::string, std::string, int, bool>> commands;

    const std::string& proxy_url = context_->GetProxyUrl();

    // Download CUDA keyring
    std::string wget_cmd =
        "wget "
        "https://developer.download.nvidia.com/compute/cuda/repos/wsl-ubuntu/"
        "x86_64/cuda-keyring_1.1-1_all.deb";
    if (!proxy_url.empty()) {
        wget_cmd = "ALL_PROXY=" + proxy_url + " " + wget_cmd;
    }
    commands.emplace_back("download_cuda_keyring", wget_cmd, 300, false);

    // Install keyring
    commands.emplace_back("install_cuda_keyring",
                          "dpkg -i cuda-keyring_1.1-1_all.deb", 60, false);

    // Update package list
    std::string update_cmd =
        proxy_url.empty()
            ? "apt-get update"
            : "apt-get -o Acquire::http::proxy=\"" + proxy_url +
                  "\" -o Acquire::https::proxy=\"" + proxy_url + "\" update";
    commands.emplace_back("update_package_list", update_cmd, 300, false);

    // Install CUDA Toolkit (use real-time output)
    std::string install_cmd =
        proxy_url.empty() ? "apt-get -y install cuda-toolkit-12-8"
                          : "apt-get -o Acquire::http::proxy=\"" + proxy_url +
                                "\" -o Acquire::https::proxy=\"" + proxy_url +
                                "\" -y install cuda-toolkit-12-8";
    commands.emplace_back("install_cuda_toolkit", install_cmd, 1200, true);

    // Add CUDA to environment variables - use multiple methods to ensure
    // effectiveness
    commands.emplace_back(
        "add_cuda_to_bashrc",
        "echo 'export PATH=/usr/local/cuda-12.8/bin:$PATH' >> ~/.bashrc", 60,
        false);
    commands.emplace_back("add_cuda_lib_to_bashrc",
                          "echo 'export "
                          "LD_LIBRARY_PATH=/usr/local/cuda-12.8/"
                          "lib64:$LD_LIBRARY_PATH' >> ~/.bashrc",
                          60, false);
    commands.emplace_back(
        "add_cuda_to_profile",
        "echo 'export PATH=/usr/local/cuda-12.8/bin:$PATH' >> /etc/profile", 60,
        false);
    commands.emplace_back("add_cuda_lib_to_profile",
                          "echo 'export "
                          "LD_LIBRARY_PATH=/usr/local/cuda-12.8/"
                          "lib64:$LD_LIBRARY_PATH' >> /etc/profile",
                          60, false);
    commands.emplace_back(
        "create_cuda_env_script",
        "echo -e '#!/bin/bash\\nexport "
        "PATH=/usr/local/cuda-12.8/bin:$PATH\\nexport "
        "LD_LIBRARY_PATH=/usr/local/cuda-12.8/lib64:$LD_LIBRARY_PATH' > "
        "/etc/profile.d/cuda.sh && chmod +x /etc/profile.d/cuda.sh",
        60, false);

    for (const auto& [step_name, cmd, timeout, use_realtime] : commands) {
        info_log("[ENV] CUDA Toolkit installation step: %s", step_name.c_str());

        int cmd_exit_code = 0;
        if (use_realtime) {
            // Use WSLProcess to get real-time output
            std::string wsl_cmd = parallax::utils::BuildWSLCommand(
                context_->GetUbuntuVersion(), cmd);
            WSLProcess wsl_process;
            cmd_exit_code = wsl_process.Execute(wsl_cmd);
        } else {
            // Use regular execution method
            auto [exit_code, output] = executor_->ExecuteWSL(cmd, timeout);
            cmd_exit_code = exit_code;
        }

        if (cmd_exit_code != 0) {
            std::string error_msg =
                "Failed at step '" + step_name + "': " + cmd;
            ComponentResult result = CreateFailureResult(error_msg, 21);
            LogOperationResult("Installing", result);
            return result;
        }
    }

    // Verify installation
    ComponentResult result =
        IsCudaToolkitInstalled()
            ? CreateSuccessResult("CUDA Toolkit 12.8 installed successfully")
            : CreateFailureResult(
                  "CUDA Toolkit installation completed but verification failed",
                  21);

    LogOperationResult("Installing", result);
    return result;
}

bool CudaToolkitInstaller::IsCudaToolkitInstalled() {
    // Check if CUDA Toolkit is installed - load environment variables first
    // then detect
    auto [cuda_code, cuda_output] = executor_->ExecuteWSL(
        "source ~/.bashrc && nvcc --version 2>/dev/null || "
        "/usr/local/cuda-12.8/bin/nvcc --version 2>/dev/null || echo 'not "
        "found'");
    if (cuda_code == 0 &&
        (cuda_output.find("release 12.8") != std::string::npos ||
         cuda_output.find("release 12.9") != std::string::npos)) {
        return true;
    }

    // Check if cuda-toolkit-12-8 package exists
    auto [dpkg_code, dpkg_output] =
        executor_->ExecuteWSL("dpkg -l | grep cuda-toolkit-12");
    if (dpkg_code == 0 && !dpkg_output.empty()) {
        return true;
    }

    // Check if CUDA installation directory exists
    auto [dir_code, dir_output] = executor_->ExecuteWSL(
        "ls -la /usr/local/cuda-12.8/bin/nvcc 2>/dev/null || ls -la "
        "/usr/local/cuda/bin/nvcc 2>/dev/null || echo 'not found'");
    return (dir_code == 0 && dir_output.find("not found") == std::string::npos);
}

EnvironmentComponent CudaToolkitInstaller::GetComponentType() const {
    return EnvironmentComponent::kCudaToolkit;
}

std::string CudaToolkitInstaller::GetComponentName() const {
    return "CUDA Toolkit";
}

// CargoInstaller implementation
CargoInstaller::CargoInstaller(std::shared_ptr<ExecutionContext> context,
                               std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult CargoInstaller::Check() {
    LogOperationStart("Checking");

    bool cargo_installed = IsCargoInstalled();

    ComponentResult result =
        cargo_installed
            ? CreateSkippedResult("Rust Cargo is already installed")
            : CreateFailureResult("Rust Cargo is not installed", 22);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult CargoInstaller::Install() {
    LogOperationStart("Installing");

    // Check if Cargo is installed
    if (IsCargoInstalled()) {
        ComponentResult result =
            CreateSkippedResult("Rust Cargo is already installed");
        LogOperationResult("Installing", result);
        return result;
    }

    info_log("[ENV] Installing Rust Cargo in WSL...");

    const std::string& proxy_url = context_->GetProxyUrl();

    // Install Rust and Cargo - execute step by step for proxy settings to take
    // effect
    std::string download_cmd =
        "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs -o "
        "/tmp/rustup.sh";
    std::string install_cmd = "sh /tmp/rustup.sh -y";

    if (!proxy_url.empty()) {
        download_cmd = "ALL_PROXY=" + proxy_url + " " + download_cmd;
        install_cmd = "ALL_PROXY=" + proxy_url + " HTTPS_PROXY=" + proxy_url +
                      " HTTP_PROXY=" + proxy_url + " " + install_cmd;
    }

    // Download rustup script
    auto [download_code, download_output] =
        executor_->ExecuteWSL(download_cmd, 300);
    if (download_code != 0) {
        ComponentResult result = CreateFailureResult(
            "Failed to download rustup script: " + download_output, 22);
        LogOperationResult("Installing", result);
        return result;
    }

    // Perform installation
    auto [install_code, install_output] =
        executor_->ExecuteWSL(install_cmd, 600);
    if (install_code != 0) {
        ComponentResult result = CreateFailureResult(
            "Failed to install Rust: " + install_output, 22);
        LogOperationResult("Installing", result);
        return result;
    }

    // Add Cargo to environment variables - use multiple methods to ensure
    // effectiveness
    auto [bashrc_code, bashrc_output] = executor_->ExecuteWSL(
        "echo 'export PATH=$HOME/.cargo/bin:$PATH' >> ~/.bashrc", 30);
    if (bashrc_code != 0) {
        info_log("[ENV] Warning: Failed to add cargo to bashrc: %s",
                 bashrc_output.c_str());
    }

    auto [profile_code, profile_output] = executor_->ExecuteWSL(
        "echo 'export PATH=$HOME/.cargo/bin:$PATH' >> /etc/profile", 30);
    if (profile_code != 0) {
        info_log("[ENV] Warning: Failed to add cargo to profile: %s",
                 profile_output.c_str());
    }

    // Verify installation
    ComponentResult result =
        IsCargoInstalled()
            ? CreateSuccessResult("Rust Cargo installed successfully")
            : CreateFailureResult(
                  "Cargo installation completed but verification failed", 22);

    LogOperationResult("Installing", result);
    return result;
}

bool CargoInstaller::IsCargoInstalled() {
    // Check if cargo command is available - load environment variables first
    // then detect
    auto [cargo_code, cargo_output] = executor_->ExecuteWSL(
        "source ~/.bashrc && cargo --version 2>/dev/null || ~/.cargo/bin/cargo "
        "--version 2>/dev/null || echo 'not found'");
    return (cargo_code == 0 &&
            cargo_output.find("not found") == std::string::npos &&
            !cargo_output.empty());
}

EnvironmentComponent CargoInstaller::GetComponentType() const {
    return EnvironmentComponent::kCargo;
}

std::string CargoInstaller::GetComponentName() const { return "Rust Cargo"; }

// NinjaInstaller implementation
NinjaInstaller::NinjaInstaller(std::shared_ptr<ExecutionContext> context,
                               std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult NinjaInstaller::Check() {
    LogOperationStart("Checking");

    bool ninja_installed = IsNinjaInstalled();

    ComponentResult result =
        ninja_installed
            ? CreateSkippedResult("Ninja build tool is already installed")
            : CreateFailureResult("Ninja build tool is not installed", 23);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult NinjaInstaller::Install() {
    LogOperationStart("Installing");

    // Check if Ninja is installed
    if (IsNinjaInstalled()) {
        ComponentResult result =
            CreateSkippedResult("Ninja build tool is already installed");
        LogOperationResult("Installing", result);
        return result;
    }

    info_log("[ENV] Installing Ninja build tool in WSL...");

    const std::string& proxy_url = context_->GetProxyUrl();

    // Install Ninja
    std::string install_cmd =
        proxy_url.empty() ? "apt-get install -y ninja-build"
                          : "apt-get -o Acquire::http::proxy=\"" + proxy_url +
                                "\" -o Acquire::https::proxy=\"" + proxy_url +
                                "\" install -y ninja-build";

    auto [install_code, install_output] =
        executor_->ExecuteWSL(install_cmd, 300);

    ComponentResult result =
        (install_code != 0)
            ? CreateFailureResult("Failed to install Ninja: " + install_output,
                                  23)
            : (IsNinjaInstalled()
                   ? CreateSuccessResult(
                         "Ninja build tool installed successfully")
                   : CreateFailureResult(
                         "Ninja installation completed but verification failed",
                         23));

    LogOperationResult("Installing", result);
    return result;
}

bool NinjaInstaller::IsNinjaInstalled() {
    // Check if ninja command is available
    auto [ninja_code, ninja_output] = executor_->ExecuteWSL("ninja --version");
    return (ninja_code == 0 && !ninja_output.empty());
}

EnvironmentComponent NinjaInstaller::GetComponentType() const {
    return EnvironmentComponent::kNinja;
}

std::string NinjaInstaller::GetComponentName() const {
    return "Ninja Build Tool";
}

}  // namespace environment
}  // namespace parallax
