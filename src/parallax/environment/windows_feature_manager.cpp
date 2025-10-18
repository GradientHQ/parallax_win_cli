#include "windows_feature_manager.h"
#include "environment_installer.h"
#include "config/config_manager.h"
#include "utils/process.h"
#include "utils/utils.h"
#include "tinylog/tinylog.h"
#include <windows.h>
#include <winsvc.h>
#include <sstream>

namespace parallax {
namespace environment {

// WSLFeatureManager implementation
WSLFeatureManager::WSLFeatureManager(std::shared_ptr<ExecutionContext> context,
                                     std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult WSLFeatureManager::Check() {
    LogOperationStart("Checking");

    // Check if WSL Windows feature is enabled
    bool wsl_feature_enabled =
        executor_->IsWindowsFeatureEnabled("Microsoft-Windows-Subsystem-Linux");

    info_log("[ENV] WSL Windows feature status: %s",
             wsl_feature_enabled ? "enabled" : "disabled");

    ComponentResult result =
        wsl_feature_enabled
            ? CreateSkippedResult("WSL Windows feature is already enabled")
            : CreateFailureResult("WSL Windows feature is not enabled", 2);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult WSLFeatureManager::Install() {
    LogOperationStart("Installing");

    // Check if WSL Windows feature is enabled
    bool wsl_feature_enabled =
        executor_->IsWindowsFeatureEnabled("Microsoft-Windows-Subsystem-Linux");

    // Record initial state to determine if reboot is needed
    bool initial_wsl_feature_enabled = wsl_feature_enabled;

    info_log("[ENV] WSL Windows feature status: %s",
             wsl_feature_enabled ? "enabled" : "disabled");

    // If WSL Windows feature is enabled, return success directly
    if (wsl_feature_enabled) {
        ComponentResult result =
            CreateSkippedResult("WSL Windows feature is already enabled");
        LogOperationResult("Installing", result);
        return result;
    }

    // Enable WSL Windows feature
    info_log("[ENV] Enabling WSL Windows feature...");

    std::string dism_cmd =
        "dism.exe /online /enable-feature "
        "/featurename:Microsoft-Windows-Subsystem-Linux /all /norestart";
    auto [dism_exit_code, dism_output] = executor_->ExecutePowerShell(dism_cmd);

    // DISM return value meanings: 0=success no reboot needed, 1=success reboot
    // needed, 2=feature already enabled
    if (dism_exit_code != 0 && dism_exit_code != 1 && dism_exit_code != 2) {
        // Fallback method: use PowerShell Enable-WindowsOptionalFeature command
        if (!executor_->EnableWindowsFeature(
                "Microsoft-Windows-Subsystem-Linux")) {
            ComponentResult result = CreateFailureResult(
                "Failed to enable WSL Windows feature: " + dism_output, 2);
            LogOperationResult("Installing", result);
            return result;
        }
    }

    info_log("[ENV] WSL Windows feature enabled successfully");

    // Verify if feature is enabled
    wsl_feature_enabled =
        executor_->IsWindowsFeatureEnabled("Microsoft-Windows-Subsystem-Linux");
    if (!wsl_feature_enabled) {
        ComponentResult result =
            CreateFailureResult("Failed to enable WSL Windows feature", 2);
        LogOperationResult("Installing", result);
        return result;
    }

    // Check if reboot is needed (if WSL feature was not enabled before and is
    // now enabled, reboot is required)
    ComponentResult result =
        !initial_wsl_feature_enabled
            ? CreateSuccessResult(
                  "WSL Windows feature enabled successfully. System reboot "
                  "required to complete installation.")
            : CreateSuccessResult("WSL Windows feature enabled successfully");

    LogOperationResult("Installing", result);
    return result;
}

EnvironmentComponent WSLFeatureManager::GetComponentType() const {
    return EnvironmentComponent::kWSL2;
}

std::string WSLFeatureManager::GetComponentName() const {
    return "WSL2 Feature";
}

// VirtualMachinePlatformManager implementation
VirtualMachinePlatformManager::VirtualMachinePlatformManager(
    std::shared_ptr<ExecutionContext> context,
    std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult VirtualMachinePlatformManager::Check() {
    LogOperationStart("Checking");

    // Check if Windows feature is enabled
    bool feature_enabled =
        executor_->IsWindowsFeatureEnabled("VirtualMachinePlatform");

    ComponentResult result =
        feature_enabled
            ? CreateSkippedResult("Virtual Machine Platform is already enabled")
            : CreateFailureResult("Virtual Machine Platform is not enabled", 3);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult VirtualMachinePlatformManager::Install() {
    LogOperationStart("Installing");

    // Check if Windows feature is enabled
    bool feature_enabled =
        executor_->IsWindowsFeatureEnabled("VirtualMachinePlatform");

    // Record initial state to determine if reboot is needed
    bool initial_feature_enabled = feature_enabled;

    // If Windows feature is enabled, consider feature properly configured
    if (feature_enabled) {
        ComponentResult result =
            CreateSkippedResult("Virtual Machine Platform is already enabled");
        LogOperationResult("Installing", result);
        return result;
    }

    // Try to enable Windows feature
    // Use DISM command to enable Virtual Machine Platform feature - required
    // prerequisite for WSL2
    std::string dism_cmd =
        "dism.exe /online /enable-feature "
        "/featurename:VirtualMachinePlatform "
        "/all /norestart";
    auto [dism_exit_code, dism_output] = executor_->ExecutePowerShell(dism_cmd);

    // DISM return value meanings: 0=success no reboot needed, 1=success reboot
    // needed, 2=feature already enabled
    if (dism_exit_code == 0 || dism_exit_code == 1 || dism_exit_code == 2) {
        // DISM command successful, no need for fallback method
        info_log(
            "[ENV] DISM command successful for VirtualMachinePlatform, exit "
            "code: %d",
            dism_exit_code);
    } else {
        // Fallback method: use PowerShell Enable-WindowsOptionalFeature command
        if (!executor_->EnableWindowsFeature("VirtualMachinePlatform")) {
            ComponentResult result = CreateFailureResult(
                "Failed to enable Virtual Machine Platform: " + dism_output, 3);
            LogOperationResult("Installing", result);
            return result;
        }
    }

    // Re-check if Windows feature is enabled
    feature_enabled =
        executor_->IsWindowsFeatureEnabled("VirtualMachinePlatform");

    if (!feature_enabled) {
        ComponentResult result =
            CreateFailureResult("Failed to enable Virtual Machine Platform", 3);
        LogOperationResult("Installing", result);
        return result;
    }

    // Check if reboot is needed (if Virtual Machine Platform feature was not
    // enabled before and is now enabled, reboot is required)
    ComponentResult result =
        !initial_feature_enabled
            ? CreateSuccessResult(
                  "Virtual Machine Platform enabled successfully. System "
                  "reboot required to complete installation.")
            : CreateSuccessResult(
                  "Virtual Machine Platform enabled successfully");

    LogOperationResult("Installing", result);
    return result;
}

EnvironmentComponent VirtualMachinePlatformManager::GetComponentType() const {
    return EnvironmentComponent::kVirtualMachinePlatform;
}

std::string VirtualMachinePlatformManager::GetComponentName() const {
    return "Virtual Machine Platform";
}

// WSLPackageInstaller implementation
WSLPackageInstaller::WSLPackageInstaller(
    std::shared_ptr<ExecutionContext> context,
    std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult WSLPackageInstaller::Check() {
    LogOperationStart("Checking");

    // Check if WSL is fully installed (using method that doesn't depend on wsl
    // command)
    bool wsl_installed = IsWSLPackageInstalled();

    info_log("[ENV] WSL package installation status: %s",
             wsl_installed ? "installed" : "not installed");

    ComponentResult result =
        wsl_installed ? CreateSkippedResult("WSL package is already installed")
                      : CreateFailureResult("WSL package is not installed", 2);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult WSLPackageInstaller::Install() {
    LogOperationStart("Installing");

    // Check if WSL is fully installed (using method that doesn't depend on wsl
    // command)
    bool wsl_installed = IsWSLPackageInstalled();

    // Record initial state to determine if reboot is needed
    bool initial_wsl_installed = wsl_installed;

    info_log("[ENV] WSL package installation status: %s",
             wsl_installed ? "installed" : "not installed");

    // If WSL is installed, return success directly
    if (wsl_installed) {
        ComponentResult result =
            CreateSkippedResult("WSL package is already installed");
        LogOperationResult("Installing", result);
        return result;
    }

    // Install WSL core
    info_log("[ENV] Installing WSL package...");

    // WSL installer package URL (obtained from config file)
    std::string wsl_installer_url =
        parallax::config::ConfigManager::GetInstance().GetConfigValue(
            parallax::config::KEY_WSL_INSTALLER_URL);

    // Generate local file path (in temporary directory)
    std::string local_wsl_path = context_->GetTempDirectory() + "wsl.x64.msi";

    // Download WSL installer package
    if (!executor_->DownloadFile(wsl_installer_url, local_wsl_path)) {
        ComponentResult result = CreateFailureResult(
            "Failed to download WSL installer from: " + wsl_installer_url, 2);
        LogOperationResult("Installing", result);
        return result;
    }

    info_log("[ENV] WSL installer downloaded successfully to: %s",
             local_wsl_path.c_str());

    // Use msiexec to silently install WSL
    std::string install_cmd =
        "msiexec /i \"" + local_wsl_path + "\" /quiet /norestart";

    auto [install_exit_code, install_output] =
        executor_->ExecutePowerShell(install_cmd, 300);

    // Clean downloaded files
    DeleteFileA(local_wsl_path.c_str());

    if (install_exit_code != 0) {
        ComponentResult result = CreateFailureResult(
            "Failed to install WSL package: " + install_output, 2);
        LogOperationResult("Installing", result);
        return result;
    }

    info_log("[ENV] WSL package installed successfully");

    // Wait for WSL service to start (up to 60 seconds)
    WaitForWSLServiceStart(60);

    // Verify installation result
    wsl_installed = IsWSLPackageInstalled();
    if (!wsl_installed) {
        ComponentResult result = CreateFailureResult(
            "WSL package installation completed but verification failed", 2);
        LogOperationResult("Installing", result);
        return result;
    }

    // Check if reboot is needed (WSL package installation usually requires
    // reboot)
    ComponentResult result =
        !initial_wsl_installed
            ? CreateSuccessResult(
                  "WSL package installed successfully. System reboot required "
                  "to complete installation.")
            : CreateSuccessResult("WSL package installed successfully");

    LogOperationResult("Installing", result);
    return result;
}

bool WSLPackageInstaller::IsWSLPackageInstalled() {
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

    // Method 3: Check if WSL service exists
    bool wsl_service_exists = false;
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager) {
        SC_HANDLE hService =
            OpenService(hSCManager, L"wslservice", SERVICE_QUERY_STATUS);
        if (hService) {
            wsl_service_exists = true;
            CloseServiceHandle(hService);
        }
        CloseServiceHandle(hSCManager);
    }

    // Consider WSL installed only if 3 conditions are met
    int detection_count = 0;
    if (wsl_exe_exists) detection_count++;
    if (wsl_registry_exists) detection_count++;
    if (wsl_service_exists) detection_count++;

    info_log(
        "[ENV] WSL detection: exe=%s, registry=%s, service=%s (score: %d/3)",
        wsl_exe_exists ? "yes" : "no", wsl_registry_exists ? "yes" : "no",
        wsl_service_exists ? "yes" : "no", detection_count);

    return detection_count >= 3;
}

bool WSLPackageInstaller::WaitForWSLServiceStart(int timeout_seconds) {
    info_log("[ENV] Waiting for WSL service to start (timeout: %d seconds)...",
             timeout_seconds);

    int elapsed_seconds = 0;
    while (elapsed_seconds < timeout_seconds) {
        // Check wslservice service status
        SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (hSCManager) {
            SC_HANDLE hService =
                OpenService(hSCManager, L"wslservice", SERVICE_QUERY_STATUS);
            if (hService) {
                SERVICE_STATUS_PROCESS ssp;
                DWORD dwBytesNeeded;
                if (QueryServiceStatusEx(
                        hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
                        sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
                    if (ssp.dwCurrentState == SERVICE_RUNNING) {
                        info_log(
                            "[ENV] WSL service started successfully after %d "
                            "seconds",
                            elapsed_seconds);
                        CloseServiceHandle(hService);
                        CloseServiceHandle(hSCManager);
                        return true;
                    }
                }
                CloseServiceHandle(hService);
            }
            CloseServiceHandle(hSCManager);
        }

        // Wait 1 second then retry
        Sleep(1000);
        elapsed_seconds++;

        // Check if stop is requested
        if (IsStopRequested()) {
            info_log("[ENV] WSL service wait interrupted by stop request");
            return false;
        }
    }

    info_log(
        "[ENV] WSL service wait timeout after %d seconds (this is not an "
        "error)",
        timeout_seconds);
    return false;
}

EnvironmentComponent WSLPackageInstaller::GetComponentType() const {
    return EnvironmentComponent::kWSLInstall;
}

std::string WSLPackageInstaller::GetComponentName() const {
    return "WSL Package";
}

}  // namespace environment
}  // namespace parallax
