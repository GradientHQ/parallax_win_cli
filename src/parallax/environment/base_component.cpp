#include "base_component.h"
#include "environment_installer.h"  // For StatusToString function
#include "config/config_manager.h"
#include "utils/utils.h"
#include "tinylog/tinylog.h"
#include <windows.h>

namespace parallax {
namespace environment {

// ExecutionContext implementation
ExecutionContext::ExecutionContext()
    : silent_mode_(false), stop_requested_(false) {
    // Set temporary directory to system temporary directory
    char temp_path[MAX_PATH];
    DWORD result = GetTempPathA(MAX_PATH, temp_path);
    if (result > 0 && result <= MAX_PATH) {
        temp_directory_ = temp_path;
    } else {
        temp_directory_ = "C:\\Temp\\";
    }

    ubuntu_version_ =
        parallax::config::ConfigManager::GetInstance().GetConfigValue(
            parallax::config::KEY_WSL_LINUX_DISTRO);

    // Get proxy URL from utils
    proxy_url_ = parallax::utils::GetProxyUrl();

    info_log(
        "[ENV] ExecutionContext initialized. Temp directory: %s, Ubuntu "
        "version: %s, Proxy URL: %s",
        temp_directory_.c_str(), ubuntu_version_.c_str(),
        proxy_url_.empty() ? "none" : proxy_url_.c_str());
}

void ExecutionContext::ReportProgress(const std::string& step,
                                      const std::string& message,
                                      int progress_percent) {
    if (progress_callback_) {
        progress_callback_(step, message, progress_percent);
    }
}

void ExecutionContext::SetProgressCallback(ProgressCallback callback) {
    progress_callback_ = callback;
}

void ExecutionContext::RequestStop() { stop_requested_ = true; }

bool ExecutionContext::IsStopRequested() const {
    return stop_requested_.load();
}

void ExecutionContext::ResetStop() { stop_requested_ = false; }

// BaseEnvironmentComponent implementation
BaseEnvironmentComponent::BaseEnvironmentComponent(
    std::shared_ptr<ExecutionContext> context)
    : context_(context) {}

ComponentResult BaseEnvironmentComponent::CreateSuccessResult(
    const std::string& message) const {
    return ComponentResult(GetComponentType(), InstallationStatus::kSuccess,
                           message);
}

ComponentResult BaseEnvironmentComponent::CreateFailureResult(
    const std::string& message, int error_code) const {
    return ComponentResult(GetComponentType(), InstallationStatus::kFailed,
                           message, error_code);
}

ComponentResult BaseEnvironmentComponent::CreateSkippedResult(
    const std::string& message) const {
    return ComponentResult(GetComponentType(), InstallationStatus::kSkipped,
                           message);
}

ComponentResult BaseEnvironmentComponent::CreateWarningResult(
    const std::string& message) const {
    return ComponentResult(GetComponentType(), InstallationStatus::kWarning,
                           message);
}

void BaseEnvironmentComponent::LogOperationStart(
    const std::string& operation) const {
    info_log("[ENV] %s %s", operation.c_str(), GetComponentName().c_str());
}

void BaseEnvironmentComponent::LogOperationResult(
    const std::string& operation, const ComponentResult& result) const {
    info_log("[ENV] %s %s result: status=%s, message=%s", operation.c_str(),
             GetComponentName().c_str(), StatusToString(result.status).c_str(),
             result.message.c_str());
}

bool BaseEnvironmentComponent::IsStopRequested() const {
    return context_->IsStopRequested();
}

}  // namespace environment
}  // namespace parallax
