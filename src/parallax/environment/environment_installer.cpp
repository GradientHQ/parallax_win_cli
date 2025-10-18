#include "environment_installer.h"
#include "base_component.h"
#include "command_executor.h"
#include "system_checker.h"
#include "windows_feature_manager.h"
#include "software_installer.h"
#include "utils/utils.h"
#include "tinylog/tinylog.h"

namespace parallax {
namespace environment {

// Component name conversion function implementation
std::string ComponentToString(EnvironmentComponent component) {
    switch (component) {
        case EnvironmentComponent::kOSVersion:
            return "OS Version";
        case EnvironmentComponent::kNvidiaGPU:
            return "NVIDIA GPU Hardware";
        case EnvironmentComponent::kNvidiaDriver:
            return "NVIDIA Driver";
        case EnvironmentComponent::kWSL2:
            return "WSL2 Feature";
        case EnvironmentComponent::kWSLInstall:
            return "WSL Package";
        case EnvironmentComponent::kVirtualMachinePlatform:
            return "Virtual Machine Platform";
        case EnvironmentComponent::kWSL2Kernel:
            return "WSL2 Kernel";
        case EnvironmentComponent::kWSL2DefaultVersion:
            return "WSL2 Default Version";
        case EnvironmentComponent::kUbuntu:
            return "Ubuntu";
        case EnvironmentComponent::kBIOSVirtualization:
            return "BIOS Virtualization";
        case EnvironmentComponent::kCudaToolkit:
            return "CUDA Toolkit";
        case EnvironmentComponent::kCargo:
            return "Rust Cargo";
        case EnvironmentComponent::kNinja:
            return "Ninja Build Tool";
        case EnvironmentComponent::kPipUpgrade:
            return "pip Upgrade";
        case EnvironmentComponent::kParallaxProject:
            return "Parallax Project";
        default:
            return "Unknown";
    }
}

std::string StatusToString(InstallationStatus status) {
    switch (status) {
        case InstallationStatus::kSuccess:
            return "success";
        case InstallationStatus::kFailed:
            return "failed";
        case InstallationStatus::kSkipped:
            return "skipped";
        case InstallationStatus::kInProgress:
            return "in_progress";
        case InstallationStatus::kWarning:
            return "warning";
        default:
            return "unknown";
    }
}

// Component progress mapping
const std::map<EnvironmentComponent, int>
    EnvironmentInstaller::component_progress_ = {
        {EnvironmentComponent::kOSVersion, 5},
        {EnvironmentComponent::kNvidiaGPU, 8},
        {EnvironmentComponent::kNvidiaDriver, 10},
        {EnvironmentComponent::kWSL2, 15},
        {EnvironmentComponent::kVirtualMachinePlatform, 20},
        {EnvironmentComponent::kWSLInstall, 25},
        {EnvironmentComponent::kBIOSVirtualization, 30},
        {EnvironmentComponent::kWSL2Kernel, 35},
        {EnvironmentComponent::kWSL2DefaultVersion, 40},
        {EnvironmentComponent::kUbuntu, 45},
        {EnvironmentComponent::kCudaToolkit, 60},
        {EnvironmentComponent::kCargo, 70},
        {EnvironmentComponent::kNinja, 80},
        {EnvironmentComponent::kPipUpgrade, 90},
        {EnvironmentComponent::kParallaxProject, 95}};

EnvironmentInstaller::EnvironmentInstaller() {
    // Initialize core components
    context_ = std::make_shared<ExecutionContext>();
    executor_ = std::make_shared<CommandExecutor>(context_);

    // Initialize all environment components
    InitializeComponents();

    info_log(ENV_LOG_PREFIX
             "EnvironmentInstaller initialized with %zu components",
             components_.size());
}

EnvironmentInstaller::~EnvironmentInstaller() {
    // Cleanup is handled by smart pointers
}

void EnvironmentInstaller::InitializeComponents() {
    // Get all component types and create instances
    auto all_components = ComponentFactory::GetAllComponents();

    for (auto component_type : all_components) {
        auto component = ComponentFactory::CreateComponent(component_type,
                                                           context_, executor_);
        if (component) {
            components_[component_type] = component;
        }
    }
}

EnvironmentResult EnvironmentInstaller::CheckEnvironment(
    ComponentCheckCallback component_callback) {
    EnvironmentResult result;
    result.reboot_required = false;

    context_->ReportProgress("check_environment",
                             "Starting environment check...", 0);
    info_log(ENV_LOG_PREFIX "Starting environment check process");

    // Step 1: Check administrator privileges
    info_log(ENV_LOG_PREFIX "Checking administrator privileges");
    if (!CheckAdminPrivileges()) {
        info_log(ENV_LOG_PREFIX "Administrator privileges check failed");
        ComponentResult admin_result(EnvironmentComponent::kOSVersion,
                                     InstallationStatus::kFailed,
                                     "Administrator privileges required", 1);
        result.component_results.push_back(admin_result);
        if (component_callback) component_callback(admin_result);
        result.overall_message = "Administrator privileges required";
        return result;
    }
    info_log(ENV_LOG_PREFIX "Administrator privileges check passed");

    // Step 2: Check all components in order
    auto check_order = ComponentFactory::GetAllComponents();

    for (auto component_type : check_order) {
        auto component_it = components_.find(component_type);
        if (component_it == components_.end()) {
            continue;  // Skip if component not found
        }

        auto component = component_it->second;
        ComponentResult comp_result =
            ExecuteComponentOperation(component, false, component_callback);

        result.component_results.push_back(comp_result);

        // For critical system components, stop on failure
        if (comp_result.status == InstallationStatus::kFailed) {
            bool is_critical =
                (component_type == EnvironmentComponent::kOSVersion ||
                 component_type == EnvironmentComponent::kNvidiaGPU ||
                 component_type == EnvironmentComponent::kNvidiaDriver);

            if (is_critical) {
                info_log(ENV_LOG_PREFIX "Critical component check failed: %s",
                         comp_result.message.c_str());
                result.overall_message =
                    "Critical component failure: " + comp_result.message;
                return result;
            }
        }
    }

    // Process special cases (virtualization results)
    ProcessVirtualizationResults(result.component_results);

    context_->ReportProgress("check_complete", "Environment check completed",
                             100);

    // Set overall message
    bool all_success = true;
    for (const auto& comp_result : result.component_results) {
        if (comp_result.status == InstallationStatus::kFailed) {
            all_success = false;
            break;
        }
    }

    result.overall_message =
        all_success ? "All required components are properly configured"
                    : "Some components need attention";

    info_log(ENV_LOG_PREFIX "Environment check completed: %s",
             result.overall_message.c_str());
    return result;
}

EnvironmentResult EnvironmentInstaller::InstallEnvironment(
    ProgressCallback progress_callback) {
    context_->SetProgressCallback(progress_callback);
    context_->ResetStop();

    EnvironmentResult result;
    result.reboot_required = false;

    context_->ReportProgress("install_start",
                             "Starting environment installation...", 0);
    info_log(ENV_LOG_PREFIX "Starting environment installation process");

    // Check administrator privileges
    info_log(ENV_LOG_PREFIX "Checking administrator privileges");
    if (!CheckAdminPrivileges()) {
        info_log(ENV_LOG_PREFIX "Administrator privileges check failed");
        ComponentResult admin_result(EnvironmentComponent::kOSVersion,
                                     InstallationStatus::kFailed,
                                     "Administrator privileges required", 1);
        result.component_results.push_back(admin_result);
        result.overall_message = "Administrator privileges required";
        return result;
    }
    info_log(ENV_LOG_PREFIX "Administrator privileges check passed");

    // Install components in phases

    // Phase 1: System checks (cannot be installed)
    auto system_components = ComponentFactory::GetSystemComponents();
    for (auto component_type : system_components) {
        auto component_it = components_.find(component_type);
        if (component_it == components_.end()) continue;

        ComponentResult comp_result = ExecuteComponentOperation(
            component_it->second, false);  // Check only
        result.component_results.push_back(comp_result);

        if (comp_result.status == InstallationStatus::kFailed) {
            info_log(ENV_LOG_PREFIX "System component check failed: %s",
                     comp_result.message.c_str());
            result.overall_message =
                "System requirement not met: " + comp_result.message;
            return result;
        }

        if (context_->IsStopRequested()) {
            result.overall_message = "Installation interrupted by stop request";
            return result;
        }
    }

    // Phase 2: Windows Features
    context_->ReportProgress("phase2_start",
                             "Phase 2: Installing Windows features...", 15);
    info_log(ENV_LOG_PREFIX "Phase 2: Installing Windows features");

    auto windows_components = ComponentFactory::GetWindowsFeatureComponents();
    for (auto component_type : windows_components) {
        auto component_it = components_.find(component_type);
        if (component_it == components_.end()) continue;

        ComponentResult comp_result =
            ExecuteComponentOperation(component_it->second, true);  // Install
        result.component_results.push_back(comp_result);

        if (comp_result.status == InstallationStatus::kFailed) {
            info_log(ENV_LOG_PREFIX "Windows feature installation failed: %s",
                     comp_result.message.c_str());
            result.overall_message =
                "Windows feature installation failed: " + comp_result.message;
            return result;
        }

        if (context_->IsStopRequested()) {
            result.overall_message = "Installation interrupted by stop request";
            return result;
        }
    }

    // Check for reboot requirements after Windows features
    ProcessRebootRequirements(result);
    if (result.reboot_required) {
        return result;
    }

    // Phase 3: Software Components
    context_->ReportProgress("phase3_start",
                             "Phase 3: Installing software components...", 60);
    info_log(ENV_LOG_PREFIX "Phase 3: Installing software components");

    auto software_components = ComponentFactory::GetSoftwareComponents();
    for (auto component_type : software_components) {
        auto component_it = components_.find(component_type);
        if (component_it == components_.end()) continue;

        ComponentResult comp_result =
            ExecuteComponentOperation(component_it->second, true);  // Install
        result.component_results.push_back(comp_result);

        if (comp_result.status == InstallationStatus::kFailed) {
            info_log(ENV_LOG_PREFIX "Software installation failed: %s",
                     comp_result.message.c_str());
            result.overall_message =
                "Software installation failed: " + comp_result.message;
            return result;
        }

        if (context_->IsStopRequested()) {
            result.overall_message = "Installation interrupted by stop request";
            return result;
        }
    }

    context_->ReportProgress("install_complete", "Installation completed", 100);

    // Set overall message
    bool all_success = true;
    for (const auto& comp_result : result.component_results) {
        if (comp_result.status == InstallationStatus::kFailed) {
            all_success = false;
            break;
        }
    }

    result.overall_message = all_success
                                 ? "All components installed successfully"
                                 : "Some components failed to install";

    info_log(ENV_LOG_PREFIX "Environment installation completed: %s",
             result.overall_message.c_str());
    return result;
}

void EnvironmentInstaller::SetSilentMode(bool silent) {
    context_->SetSilentMode(silent);
}

void EnvironmentInstaller::Stop() { context_->RequestStop(); }

bool EnvironmentInstaller::IsStopped() const {
    return context_->IsStopRequested();
}

void EnvironmentInstaller::ResetStop() { context_->ResetStop(); }

bool EnvironmentInstaller::CheckAdminPrivileges() {
    return parallax::utils::IsAdmin();
}

ComponentResult EnvironmentInstaller::ExecuteComponentOperation(
    std::shared_ptr<IEnvironmentComponent> component, bool perform_installation,
    ComponentCheckCallback callback) {
    ReportComponentProgress(component->GetComponentType(),
                            perform_installation ? "Installing" : "Checking");

    ComponentResult result =
        perform_installation ? component->Install() : component->Check();

    if (callback) {
        callback(result);
    }

    return result;
}

int EnvironmentInstaller::GetProgressPercentForComponent(
    EnvironmentComponent component) {
    auto it = component_progress_.find(component);
    return (it != component_progress_.end()) ? it->second : 50;
}

void EnvironmentInstaller::ReportComponentProgress(
    EnvironmentComponent component, const std::string& operation) {
    std::string progress_id =
        "op_" + operation + "_" + std::to_string(static_cast<int>(component));
    std::string message =
        operation + " " + ComponentToString(component) + "...";
    context_->ReportProgress(progress_id, message,
                             GetProgressPercentForComponent(component));
}

bool EnvironmentInstaller::CheckIfRebootRequired(
    const ComponentResult& result) {
    return (result.status == InstallationStatus::kSuccess &&
            result.message.find("reboot") != std::string::npos);
}

void EnvironmentInstaller::ProcessVirtualizationResults(
    std::vector<ComponentResult>& results) {
    // Find VirtualMachinePlatform and BIOSVirtualization results and merge them
    ComponentResult* vm_result = nullptr;
    ComponentResult* bios_result = nullptr;

    for (auto& result : results) {
        if (result.component == EnvironmentComponent::kVirtualMachinePlatform) {
            vm_result = &result;
        } else if (result.component ==
                   EnvironmentComponent::kBIOSVirtualization) {
            bios_result = &result;
        }
    }

    if (vm_result && bios_result) {
        if (vm_result->status == InstallationStatus::kSkipped &&
            bios_result->status == InstallationStatus::kSuccess) {
            vm_result->message =
                "Virtual Machine Platform is enabled and BIOS virtualization "
                "is working properly";
        } else if (vm_result->status == InstallationStatus::kSkipped &&
                   bios_result->status == InstallationStatus::kFailed) {
            vm_result->status = InstallationStatus::kFailed;
            vm_result->message =
                "Virtual Machine Platform is enabled in Windows, but " +
                bios_result->message;
            vm_result->error_code = bios_result->error_code;
        }
    }
}

void EnvironmentInstaller::ProcessRebootRequirements(
    EnvironmentResult& result) {
    std::vector<std::string> reboot_reasons;

    for (const auto& comp_result : result.component_results) {
        if (CheckIfRebootRequired(comp_result)) {
            result.reboot_required = true;
            reboot_reasons.push_back(ComponentToString(comp_result.component));
        }
    }

    if (result.reboot_required) {
        std::string reboot_message = "Installation completed (";
        for (size_t i = 0; i < reboot_reasons.size(); ++i) {
            if (i > 0) reboot_message += ", ";
            reboot_message += reboot_reasons[i];
        }
        reboot_message += "). System reboot required before continuing.";
        result.overall_message = reboot_message;
    }
}

// ComponentFactory implementation
std::shared_ptr<IEnvironmentComponent> ComponentFactory::CreateComponent(
    EnvironmentComponent type, std::shared_ptr<ExecutionContext> context,
    std::shared_ptr<CommandExecutor> executor) {
    switch (type) {
        case EnvironmentComponent::kOSVersion:
            return std::make_shared<OSVersionChecker>(context);
        case EnvironmentComponent::kNvidiaGPU:
            return std::make_shared<NvidiaGPUChecker>(context);
        case EnvironmentComponent::kNvidiaDriver:
            return std::make_shared<NvidiaDriverChecker>(context);
        case EnvironmentComponent::kWSL2:
            return std::make_shared<WSLFeatureManager>(context, executor);
        case EnvironmentComponent::kVirtualMachinePlatform:
            return std::make_shared<VirtualMachinePlatformManager>(context,
                                                                   executor);
        case EnvironmentComponent::kWSLInstall:
            return std::make_shared<WSLPackageInstaller>(context, executor);
        case EnvironmentComponent::kWSL2Kernel:
            return std::make_shared<WSL2KernelInstaller>(context, executor);
        case EnvironmentComponent::kWSL2DefaultVersion:
            return std::make_shared<WSL2DefaultVersionManager>(context,
                                                               executor);
        case EnvironmentComponent::kUbuntu:
            return std::make_shared<UbuntuInstaller>(context, executor);
        case EnvironmentComponent::kBIOSVirtualization:
            return std::make_shared<BIOSVirtualizationChecker>(context,
                                                               executor);
        case EnvironmentComponent::kCudaToolkit:
            return std::make_shared<CudaToolkitInstaller>(context, executor);
        case EnvironmentComponent::kCargo:
            return std::make_shared<CargoInstaller>(context, executor);
        case EnvironmentComponent::kNinja:
            return std::make_shared<NinjaInstaller>(context, executor);
        case EnvironmentComponent::kPipUpgrade:
            return std::make_shared<PipUpgradeManager>(context, executor);
        case EnvironmentComponent::kParallaxProject:
            return std::make_shared<ParallaxProjectInstaller>(context,
                                                              executor);
        default:
            return nullptr;
    }
}

std::vector<EnvironmentComponent> ComponentFactory::GetAllComponents() {
    return {EnvironmentComponent::kOSVersion,
            EnvironmentComponent::kNvidiaGPU,
            EnvironmentComponent::kNvidiaDriver,
            EnvironmentComponent::kWSL2,
            EnvironmentComponent::kVirtualMachinePlatform,
            EnvironmentComponent::kWSLInstall,
            EnvironmentComponent::kBIOSVirtualization,
            EnvironmentComponent::kWSL2Kernel,
            EnvironmentComponent::kWSL2DefaultVersion,
            EnvironmentComponent::kUbuntu,
            EnvironmentComponent::kCudaToolkit,
            EnvironmentComponent::kCargo,
            EnvironmentComponent::kNinja,
            EnvironmentComponent::kPipUpgrade,
            EnvironmentComponent::kParallaxProject};
}

std::vector<EnvironmentComponent> ComponentFactory::GetSystemComponents() {
    return {EnvironmentComponent::kOSVersion, EnvironmentComponent::kNvidiaGPU,
            EnvironmentComponent::kNvidiaDriver};
}

std::vector<EnvironmentComponent>
ComponentFactory::GetWindowsFeatureComponents() {
    return {EnvironmentComponent::kWSL2,
            EnvironmentComponent::kVirtualMachinePlatform,
            EnvironmentComponent::kWSLInstall,
            EnvironmentComponent::kWSL2Kernel,
            EnvironmentComponent::kWSL2DefaultVersion,
            EnvironmentComponent::kBIOSVirtualization,
            EnvironmentComponent::kUbuntu};
}

std::vector<EnvironmentComponent> ComponentFactory::GetSoftwareComponents() {
    return {EnvironmentComponent::kCudaToolkit, EnvironmentComponent::kCargo,
            EnvironmentComponent::kNinja, EnvironmentComponent::kPipUpgrade,
            EnvironmentComponent::kParallaxProject};
}

}  // namespace environment
}  // namespace parallax