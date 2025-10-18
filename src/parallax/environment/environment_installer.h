#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <map>
#include "utils/wsl_process.h"

// Environment-related log prefix identifier
#define ENV_LOG_PREFIX "[ENV] "

namespace parallax {
namespace environment {

// Forward declarations for modular architecture
class ExecutionContext;
class CommandExecutor;
class IEnvironmentComponent;

// Environment component type enumeration - defines Windows development
// environment components to check and install
enum class EnvironmentComponent {
    kOSVersion,     // Operating system version check
    kNvidiaGPU,     // NVIDIA GPU hardware detection (minimum RTX 3060 Ti)
    kNvidiaDriver,  // NVIDIA driver detection
    kWSL2,          // WSL2 subsystem
    kVirtualMachinePlatform,  // Virtual machine platform
    kWSLInstall,              // WSL basic installation (wsl --install)
    kWSL2Kernel,              // WSL2 kernel
    kWSL2DefaultVersion,      // WSL2 default version setting
    kUbuntu,                  // Ubuntu distribution
    kBIOSVirtualization,      // BIOS virtualization detection
    // Extended mode components
    kCudaToolkit,     // CUDA Toolkit 12.8
    kCargo,           // Rust Cargo
    kNinja,           // Ninja build tool
    kPipUpgrade,      // pip upgrade
    kParallaxProject  // Parallax project installation
};

// Installation status enumeration - represents the current state of each
// component
enum class InstallationStatus {
    kSuccess = 0,     // Component is properly installed or configured
    kFailed = 1,      // Component installation or configuration failed
    kSkipped = 2,     // Component already exists, skip installation
    kInProgress = 3,  // Component is currently being installed
    kWarning =
        4  // Component is installed but has warnings (e.g., updates available)
};

// Single component check/installation result
struct ComponentResult {
    EnvironmentComponent component;
    InstallationStatus status;
    std::string message;
    int error_code;

    ComponentResult(EnvironmentComponent comp, InstallationStatus stat,
                    const std::string& msg, int code = 0)
        : component(comp), status(stat), message(msg), error_code(code) {}
};

// Progress callback function type - used to report installation progress
using ProgressCallback = std::function<void(
    const std::string& step, const std::string& message, int progress_percent)>;

// Component check callback function type - used to report check results for
// each component in real-time
using ComponentCheckCallback =
    std::function<void(const ComponentResult& result)>;

// Overall environment check and installation result
struct EnvironmentResult {
    std::vector<ComponentResult> component_results;
    bool reboot_required;
    std::string overall_message;
};

/**
 * @brief Environment Installer
 *
 * This class manages the detection and installation of development
 * environment components using a modular architecture.
 */
class EnvironmentInstaller {
 public:
    EnvironmentInstaller();
    ~EnvironmentInstaller();

    // Main public interface
    EnvironmentResult CheckEnvironment(
        ComponentCheckCallback component_callback = nullptr);

    EnvironmentResult InstallEnvironment(
        ProgressCallback progress_callback = nullptr);

    // Configuration methods
    void SetSilentMode(bool silent);
    void Stop();
    bool IsStopped() const;
    void ResetStop();

 private:
    // Core components
    std::shared_ptr<ExecutionContext> context_;
    std::shared_ptr<CommandExecutor> executor_;

    // Component registry
    std::map<EnvironmentComponent, std::shared_ptr<IEnvironmentComponent>>
        components_;

    // Helper methods
    void InitializeComponents();
    bool CheckAdminPrivileges();
    ComponentResult ExecuteComponentOperation(
        std::shared_ptr<IEnvironmentComponent> component,
        bool perform_installation, ComponentCheckCallback callback = nullptr);

    // Progress tracking
    int GetProgressPercentForComponent(EnvironmentComponent component);
    void ReportComponentProgress(EnvironmentComponent component,
                                 const std::string& operation);

    // Result processing
    bool CheckIfRebootRequired(const ComponentResult& result);
    void ProcessVirtualizationResults(std::vector<ComponentResult>& results);
    void ProcessRebootRequirements(EnvironmentResult& result);

    // Component progress mapping
    static const std::map<EnvironmentComponent, int> component_progress_;
};

/**
 * @brief Factory class for creating environment components
 */
class ComponentFactory {
 public:
    static std::shared_ptr<IEnvironmentComponent> CreateComponent(
        EnvironmentComponent type, std::shared_ptr<ExecutionContext> context,
        std::shared_ptr<CommandExecutor> executor);

    static std::vector<EnvironmentComponent> GetAllComponents();
    static std::vector<EnvironmentComponent> GetSystemComponents();
    static std::vector<EnvironmentComponent> GetWindowsFeatureComponents();
    static std::vector<EnvironmentComponent> GetSoftwareComponents();
};

// Component name conversion functions
std::string ComponentToString(EnvironmentComponent component);
std::string StatusToString(InstallationStatus status);

}  // namespace environment
}  // namespace parallax