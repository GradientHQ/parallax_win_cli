#include "windows_feature_manager.h"
#include "environment_installer.h"
#include "config/config_manager.h"
#include "utils/process.h"
#include "utils/utils.h"
#include "tinylog/tinylog.h"
#include <windows.h>
#include <sstream>

namespace parallax {
namespace environment {

// WSL2KernelInstaller implementation
WSL2KernelInstaller::WSL2KernelInstaller(
    std::shared_ptr<ExecutionContext> context,
    std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult WSL2KernelInstaller::Check() {
    LogOperationStart("Checking");

    // Check if WSL2 kernel is installed
    bool kernel_installed = IsWSL2KernelInstalled();

    ComponentResult result =
        kernel_installed
            ? CreateSkippedResult("WSL2 kernel is already installed")
            : CreateFailureResult("WSL2 kernel is not installed", 12);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult WSL2KernelInstaller::Install() {
    LogOperationStart("Installing");

    // Check if WSL2 kernel is installed
    if (IsWSL2KernelInstalled()) {
        ComponentResult result =
            CreateSkippedResult("WSL2 kernel is already installed");
        LogOperationResult("Installing", result);
        return result;
    }

    // WSL2 kernel download URL (obtained from config file)
    std::string kernel_url =
        parallax::config::ConfigManager::GetInstance().GetConfigValue(
            parallax::config::KEY_WSL_KERNEL_URL);

    info_log("[ENV] Downloading WSL2 kernel from: %s", kernel_url.c_str());

    // Generate local file path (in temporary directory)
    std::string local_kernel_path =
        context_->GetTempDirectory() + "wsl_update_x64.msi";

    // Download WSL2 kernel file
    if (!executor_->DownloadFile(kernel_url, local_kernel_path)) {
        ComponentResult result = CreateFailureResult(
            "Failed to download WSL2 kernel from: " + kernel_url, 12);
        LogOperationResult("Installing", result);
        return result;
    }

    info_log("[ENV] WSL2 kernel downloaded successfully to: %s",
             local_kernel_path.c_str());

    // Use msiexec to silently install WSL2 kernel
    std::string install_cmd =
        "msiexec /i \"" + local_kernel_path + "\" /quiet /norestart";

    auto [install_exit_code, install_output] =
        executor_->ExecutePowerShell(install_cmd, 300);

    // Clean downloaded files
    DeleteFileA(local_kernel_path.c_str());

    ComponentResult result =
        (install_exit_code == 0)
            ? (info_log("[ENV] WSL2 kernel installed successfully"),
               CreateSuccessResult("WSL2 kernel installed successfully"))
            : (error_log("[ENV] Failed to install WSL2 kernel: %s",
                         install_output.c_str()),
               CreateFailureResult(
                   "Failed to install WSL2 kernel: " + install_output, 12));

    LogOperationResult("Installing", result);
    return result;
}

bool WSL2KernelInstaller::IsWSL2KernelInstalled() {
    // Check multiple possible WSL2 kernel file paths
    std::vector<std::string> kernel_paths = {
        "C:\\Windows\\System32\\lxss\\tools\\kernel",  // Traditional path
        "C:\\Program Files\\WSL\\tools\\kernel",       // Newly discovered path
        "C:\\Windows\\System32\\wsl\\kernel",          // Backup path
        "C:\\Windows\\System32\\lxss\\kernel",         // Another backup path
        "C:\\Program Files\\WSL\\kernel"               // Program files path
    };

    for (const auto& path : kernel_paths) {
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return true;
        }
    }

    // Check KernelVersion in registry
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WSL", 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[256];
        DWORD buffer_size = sizeof(buffer);
        if (RegQueryValueExA(hKey, "KernelVersion", NULL, NULL, (LPBYTE)buffer,
                             &buffer_size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        RegCloseKey(hKey);
    }

    return false;
}

EnvironmentComponent WSL2KernelInstaller::GetComponentType() const {
    return EnvironmentComponent::kWSL2Kernel;
}

std::string WSL2KernelInstaller::GetComponentName() const {
    return "WSL2 Kernel";
}

// WSL2DefaultVersionManager implementation
WSL2DefaultVersionManager::WSL2DefaultVersionManager(
    std::shared_ptr<ExecutionContext> context,
    std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult WSL2DefaultVersionManager::Check() {
    LogOperationStart("Checking");

    // First check if WSL package is installed
    if (!IsWSLPackageInstalled()) {
        ComponentResult result = CreateFailureResult(
            "Cannot check WSL2 default version: WSL package is not installed",
            4);
        LogOperationResult("Checking", result);
        return result;
    }

    // Check if WSL2 is the default version
    bool is_default = IsWSL2DefaultVersion();

    ComponentResult result =
        is_default ? CreateSkippedResult("WSL2 is already the default version")
                   : CreateFailureResult("WSL2 is not the default version", 4);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult WSL2DefaultVersionManager::Install() {
    LogOperationStart("Setting");

    // First check if WSL package is installed
    if (!IsWSLPackageInstalled()) {
        ComponentResult result = CreateFailureResult(
            "Cannot set WSL2 default version: WSL package is not installed", 4);
        LogOperationResult("Setting", result);
        return result;
    }

    // Check if WSL2 is the default version
    if (IsWSL2DefaultVersion()) {
        ComponentResult result =
            CreateSkippedResult("WSL2 is already the default version");
        LogOperationResult("Setting", result);
        return result;
    }

    // Set WSL default version to 2
    auto [exit_code, output] =
        executor_->ExecutePowerShell("wsl --set-default-version 2");

    ComponentResult result =
        (exit_code == 0)
            ? CreateSuccessResult("WSL default version set to 2")
            : CreateFailureResult(
                  "Failed to set WSL default version: " + output, 4);

    LogOperationResult("Setting", result);
    return result;
}

bool WSL2DefaultVersionManager::IsWSL2DefaultVersion() {
    // First check if WSL package is installed
    if (!IsWSLPackageInstalled()) {
        return false;
    }

    // Check if WSL default version is 2
    auto [exit_code, output] = executor_->ExecutePowerShell("wsl --status");
    if (exit_code == 0) {
        // Check if output contains "Default Version: 2"
        // Chinese version is default version: 2
        if (output.find(": 2") != std::string::npos) {
            return true;
        }
    }

    // Backup check method: parse wsl --list --verbose output
    auto [version_code, version_output] =
        executor_->ExecutePowerShell("wsl --list --verbose");
    if (version_code == 0) {
        // Parse output format: NAME            STATE           VERSION
        // Default version should have a * mark
        std::istringstream iss(version_output);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("*") != std::string::npos &&
                line.find("2") != std::string::npos) {
                return true;
            }
        }
    }

    return false;
}

bool WSL2DefaultVersionManager::IsWSLPackageInstalled() {
    // Method 1: Check if WSL executable file exists
    std::vector<std::string> wsl_exe_paths = {"C:\\Windows\\System32\\wsl.exe",
                                              "C:\\Windows\\SysWOW64\\wsl.exe"};

    bool wsl_exe_exists = false;
    for (const auto& path : wsl_exe_paths) {
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            wsl_exe_exists = true;
            break;
        }
    }

    // Method 2: Check WSL related registry entries
    bool wsl_registry_exists = false;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Lxss", 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {
        wsl_registry_exists = true;
        RegCloseKey(hKey);
    }

    // Consider WSL installed if both conditions are met
    return wsl_exe_exists && wsl_registry_exists;
}

EnvironmentComponent WSL2DefaultVersionManager::GetComponentType() const {
    return EnvironmentComponent::kWSL2DefaultVersion;
}

std::string WSL2DefaultVersionManager::GetComponentName() const {
    return "WSL2 Default Version";
}

// UbuntuInstaller implementation
UbuntuInstaller::UbuntuInstaller(std::shared_ptr<ExecutionContext> context,
                                 std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult UbuntuInstaller::Check() {
    LogOperationStart("Checking");

    bool ubuntu_installed = IsUbuntuInstalled();

    ComponentResult result =
        ubuntu_installed
            ? CreateSkippedResult(context_->GetUbuntuVersion() +
                                  " is already installed")
            : CreateFailureResult(
                  context_->GetUbuntuVersion() + " is not installed", -1);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult UbuntuInstaller::Install() {
    LogOperationStart("Installing");

    if (IsUbuntuInstalled()) {
        info_log("[ENV] Ubuntu %s is already installed",
                 context_->GetUbuntuVersion().c_str());
        ComponentResult result = CreateSkippedResult(
            context_->GetUbuntuVersion() + " is already installed");
        LogOperationResult("Installing", result);
        return result;
    }

    info_log("[ENV] Installing Ubuntu %s...",
             context_->GetUbuntuVersion().c_str());

    // Set longer timeout because Ubuntu installation may take longer
    int ubuntu_install_timeout = 1200;

    std::string stdout_output, stderr_output;
    // Since WSL infrastructure is already installed, directly install the
    // specified Ubuntu distribution
    std::string install_cmd =
        "wsl --install -d " + context_->GetUbuntuVersion();

    // Use ExecCommandEx2 to execute command and pass callback function
    auto self = this;  // Capture this pointer
    int install_code = parallax::utils::ExecCommandEx2(
        "powershell.exe -Command \"" + install_cmd + "\"",
        ubuntu_install_timeout, stdout_output, stderr_output,
        [self]() -> bool {
            // Check if Ubuntu is installed, if installed then terminate command
            // execution
            return self->CheckUbuntuInstalled();
        },
        false, false);

    // If terminated because Ubuntu is already installed, treat as success
    if (install_code == -3 && CheckUbuntuInstalled()) {
        info_log("[ENV] Ubuntu %s installation completed successfully",
                 context_->GetUbuntuVersion().c_str());

        // Execute wsl --shutdown command to restart virtual machine, let
        // systemd take effect
        info_log(
            "[ENV] Shutting down WSL to restart the virtual machine and enable "
            "systemd");
        auto [shutdown_code, shutdown_output] =
            executor_->ExecutePowerShell("wsl --shutdown", 30);
        if (shutdown_code != 0) {
            error_log("[ENV] Failed to shutdown WSL: %s",
                      shutdown_output.c_str());
        } else {
            info_log(
                "[ENV] WSL shutdown successful, systemd should be effective "
                "after restart");
        }

        ComponentResult result = CreateSuccessResult(
            context_->GetUbuntuVersion() + " installed successfully");
        LogOperationResult("Installing", result);
        return result;
    }

    if (install_code != 0) {
        // Handle PowerShell output encoding (usually UTF-16)
        std::string utf8_stdout =
            parallax::utils::ConvertPowerShellOutputToUtf8(stdout_output);
        std::string utf8_stderr =
            parallax::utils::ConvertPowerShellOutputToUtf8(stderr_output);

        // Merge output and return
        std::string combined_output = utf8_stdout;
        if (!utf8_stderr.empty()) {
            if (!combined_output.empty()) {
                combined_output += "\n";
            }
            combined_output += utf8_stderr;
        }

        error_log("[ENV] Failed to install Ubuntu %s: %s",
                  context_->GetUbuntuVersion().c_str(),
                  combined_output.c_str());
        ComponentResult result = CreateFailureResult(
            "Failed to install Ubuntu: " + combined_output, install_code);
        LogOperationResult("Installing", result);
        return result;
    }

    info_log("[ENV] Ubuntu %s installation completed successfully",
             context_->GetUbuntuVersion().c_str());
    ComponentResult result = CreateSuccessResult(context_->GetUbuntuVersion() +
                                                 " installed successfully");
    LogOperationResult("Installing", result);
    return result;
}

bool UbuntuInstaller::IsUbuntuInstalled() {
    // First check if WSL package is installed
    WSL2DefaultVersionManager wsl_manager(context_, executor_);
    if (!wsl_manager.IsWSLPackageInstalled()) {
        return false;
    }

    // Use wsl --list --quiet to check if Ubuntu is in the distribution list
    auto [list_exit_code, list_output] =
        executor_->ExecutePowerShell("wsl --list --quiet");
    bool ubuntu_installed =
        (list_exit_code == 0 &&
         list_output.find(context_->GetUbuntuVersion()) != std::string::npos);

    info_log("[ENV] Ubuntu %s detection: result=%s",
             context_->GetUbuntuVersion().c_str(),
             ubuntu_installed ? "installed" : "not installed");

    return ubuntu_installed;
}

bool UbuntuInstaller::CheckUbuntuInstalled() {
    // Check if Ubuntu is installed (for callback function)
    // Don't use class member functions to avoid thread safety issues

    // Use wsl --list --quiet to check if Ubuntu is in the distribution list
    auto [list_exit_code, list_output] =
        executor_->ExecutePowerShell("wsl --list --quiet");
    return (list_exit_code == 0 &&
            list_output.find(context_->GetUbuntuVersion()) !=
                std::string::npos);
}

EnvironmentComponent UbuntuInstaller::GetComponentType() const {
    return EnvironmentComponent::kUbuntu;
}

std::string UbuntuInstaller::GetComponentName() const {
    return context_->GetUbuntuVersion();
}

}  // namespace environment
}  // namespace parallax
