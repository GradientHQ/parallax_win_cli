#pragma once

#include <string>
#include <utility>
#include <memory>

namespace parallax {
namespace environment {

class ExecutionContext;

/**
 * @brief Command execution utility class
 *
 * This class provides unified command execution capabilities for both
 * PowerShell and WSL commands with proper error handling and encoding.
 */
class CommandExecutor {
 public:
    explicit CommandExecutor(std::shared_ptr<ExecutionContext> context);
    ~CommandExecutor() = default;

    /**
     * @brief Execute a PowerShell command
     * @param command The PowerShell command to execute
     * @param timeout_seconds Timeout in seconds (default: 300)
     * @return Pair of (exit_code, combined_output)
     */
    std::pair<int, std::string> ExecutePowerShell(const std::string& command,
                                                  int timeout_seconds = 300);

    /**
     * @brief Execute a WSL command
     * @param command The command to execute in WSL
     * @param timeout_seconds Timeout in seconds (default: 300)
     * @return Pair of (exit_code, combined_output)
     */
    std::pair<int, std::string> ExecuteWSL(const std::string& command,
                                           int timeout_seconds = 300);

    /**
     * @brief Check if a Windows feature is enabled
     * @param feature_name The name of the Windows feature
     * @return true if enabled, false otherwise
     */
    bool IsWindowsFeatureEnabled(const std::string& feature_name);

    /**
     * @brief Enable a Windows feature
     * @param feature_name The name of the Windows feature to enable
     * @return true if successful, false otherwise
     */
    bool EnableWindowsFeature(const std::string& feature_name);

    /**
     * @brief Download a file from URL to local path
     * @param url The URL to download from
     * @param local_path The local path to save the file
     * @return true if successful, false otherwise
     */
    bool DownloadFile(const std::string& url, const std::string& local_path);

 private:
    std::shared_ptr<ExecutionContext> context_;
};

}  // namespace environment
}  // namespace parallax
