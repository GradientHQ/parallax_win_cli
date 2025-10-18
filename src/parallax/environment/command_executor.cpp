#include "command_executor.h"
#include "base_component.h"
#include "utils/process.h"
#include "utils/utils.h"
#include "tinylog/tinylog.h"
#include <windows.h>

namespace parallax {
namespace environment {

CommandExecutor::CommandExecutor(std::shared_ptr<ExecutionContext> context)
    : context_(context) {}

std::pair<int, std::string> CommandExecutor::ExecutePowerShell(
    const std::string& command, int timeout_seconds) {
    // Check if stop has been requested
    if (context_->IsStopRequested()) {
        return {-1, "Operation interrupted by stop request"};
    }

    std::string stdout_output, stderr_output;
    int exit_code = parallax::utils::ExecCommandEx(
        "powershell.exe -Command \"" + command + "\"", timeout_seconds,
        stdout_output, stderr_output, false, true);

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

    // Add error logging - record detailed information when PowerShell command
    // execution fails
    if (exit_code != 0) {
        error_log(
            "[ENV] PowerShell command failed - Command: %s, Exit code: %d, "
            "Output: %s",
            command.c_str(), exit_code, combined_output.c_str());
    }

    // Check if stop has been requested after execution
    if (context_->IsStopRequested()) {
        return {
            -1,
            "Operation interrupted by stop request after command execution"};
    }

    return {exit_code, combined_output};
}

std::pair<int, std::string> CommandExecutor::ExecuteWSL(
    const std::string& command, int timeout_seconds) {
    // Check if stop has been requested
    if (context_->IsStopRequested()) {
        return {-1, "Operation interrupted by stop request"};
    }

    // Use specified Ubuntu version to execute command
    std::string wsl_command =
        parallax::utils::BuildWSLCommand(context_->GetUbuntuVersion(), command);

    debug_log("[ENV] WSL command: %s", wsl_command.c_str());

    std::string stdout_output, stderr_output;
    int exit_code = parallax::utils::ExecCommandEx(wsl_command, timeout_seconds,
                                                   stdout_output, stderr_output,
                                                   false, true);

    // Handle WSL output encoding
    std::string utf8_stdout =
        parallax::utils::ConvertWslOutputToUtf8(stdout_output, false);
    std::string utf8_stderr =
        parallax::utils::ConvertWslOutputToUtf8(stderr_output, true);

    if (utf8_stdout.empty() && !stdout_output.empty()) {
        utf8_stdout = stdout_output;
    }
    if (utf8_stderr.empty() && !stderr_output.empty()) {
        utf8_stderr = stderr_output;
    }

    // Merge output and return
    std::string combined_output = utf8_stdout;
    if (!utf8_stderr.empty()) {
        if (!combined_output.empty()) {
            combined_output += "\n";
        }
        combined_output += utf8_stderr;
    }

    // Add error logging - record detailed information when WSL command
    // execution fails
    if (exit_code != 0) {
        error_log(
            "[ENV] WSL command failed - Command: %s, Exit code: %d, Output: %s",
            command.c_str(), exit_code, combined_output.c_str());
    }

    // Check if stop has been requested after execution
    if (context_->IsStopRequested()) {
        return {
            -1,
            "Operation interrupted by stop request after command execution"};
    }

    return {exit_code, combined_output};
}

bool CommandExecutor::IsWindowsFeatureEnabled(const std::string& feature_name) {
    std::string cmd = "Get-WindowsOptionalFeature -Online -FeatureName " +
                      feature_name + " | Select-Object -ExpandProperty State";
    auto [exit_code, output] = ExecutePowerShell(cmd);

    return (exit_code == 0 && output.find("Enabled") != std::string::npos);
}

bool CommandExecutor::EnableWindowsFeature(const std::string& feature_name) {
    std::string cmd = "Enable-WindowsOptionalFeature -Online -FeatureName " +
                      feature_name + " -NoRestart";
    auto [exit_code, output] = ExecutePowerShell(cmd);

    return (exit_code == 0);
}

bool CommandExecutor::DownloadFile(const std::string& url,
                                   const std::string& local_path) {
    // Check if stop has been requested
    if (context_->IsStopRequested()) {
        error_log("[ENV] Download operation interrupted by stop request");
        return false;
    }

    // Use download functionality from utils
    bool result = parallax::utils::DownloadFile(url, local_path);

    // Check again if stop has been requested after download
    if (context_->IsStopRequested()) {
        error_log(
            "[ENV] Download operation interrupted by stop request after "
            "download");
        // Delete downloaded file
        if (GetFileAttributesA(local_path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            DeleteFileA(local_path.c_str());
        }
        return false;
    }

    if (!result) {
        error_log("[ENV] Failed to download file from URL: %s", url.c_str());
        // Delete possible incomplete file
        if (GetFileAttributesA(local_path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            DeleteFileA(local_path.c_str());
        }
        return false;
    }

    return true;
}

}  // namespace environment
}  // namespace parallax
