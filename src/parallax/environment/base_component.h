#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <memory>

namespace parallax {
namespace environment {

// Forward declarations
struct ComponentResult;
enum class EnvironmentComponent;
enum class InstallationStatus;

// Progress callback function type
using ProgressCallback = std::function<void(
    const std::string& step, const std::string& message, int progress_percent)>;

// Component check callback function type
using ComponentCheckCallback =
    std::function<void(const ComponentResult& result)>;

/**
 * @brief Base interface for all environment components
 *
 * This interface defines the common contract for all environment components
 * that can be checked and installed.
 */
class IEnvironmentComponent {
 public:
    virtual ~IEnvironmentComponent() = default;

    /**
     * @brief Check if the component is properly installed/configured
     * @return ComponentResult indicating the current status
     */
    virtual ComponentResult Check() = 0;

    /**
     * @brief Install or configure the component if needed
     * @return ComponentResult indicating the installation result
     */
    virtual ComponentResult Install() = 0;

    /**
     * @brief Get the component type
     * @return EnvironmentComponent enum value
     */
    virtual EnvironmentComponent GetComponentType() const = 0;

    /**
     * @brief Get human-readable component name
     * @return Component name string
     */
    virtual std::string GetComponentName() const = 0;
};

/**
 * @brief Execution context for component operations
 *
 * This class provides shared context and utilities for all component
 * operations.
 */
class ExecutionContext {
 public:
    ExecutionContext();
    ~ExecutionContext() = default;

    // Progress reporting
    void ReportProgress(const std::string& step, const std::string& message,
                        int progress_percent);
    void SetProgressCallback(ProgressCallback callback);

    // Stop mechanism
    void RequestStop();
    bool IsStopRequested() const;
    void ResetStop();

    // Configuration access
    const std::string& GetTempDirectory() const { return temp_directory_; }
    const std::string& GetUbuntuVersion() const { return ubuntu_version_; }
    const std::string& GetProxyUrl() const { return proxy_url_; }

    // Silent mode
    void SetSilentMode(bool silent) { silent_mode_ = silent; }
    bool IsSilentMode() const { return silent_mode_; }

 private:
    std::string temp_directory_;
    std::string ubuntu_version_;
    std::string proxy_url_;
    bool silent_mode_;
    std::atomic<bool> stop_requested_;
    ProgressCallback progress_callback_;
};

/**
 * @brief Base class for all environment components
 *
 * This class provides common functionality for component implementations.
 */
class BaseEnvironmentComponent : public IEnvironmentComponent {
 public:
    explicit BaseEnvironmentComponent(
        std::shared_ptr<ExecutionContext> context);
    virtual ~BaseEnvironmentComponent() = default;

    // Common utility methods
    ComponentResult CreateSuccessResult(const std::string& message) const;
    ComponentResult CreateFailureResult(const std::string& message,
                                        int error_code = 0) const;
    ComponentResult CreateSkippedResult(const std::string& message) const;
    ComponentResult CreateWarningResult(const std::string& message) const;

 protected:
    std::shared_ptr<ExecutionContext> context_;

    // Utility methods for derived classes
    void LogOperationStart(const std::string& operation) const;
    void LogOperationResult(const std::string& operation,
                            const ComponentResult& result) const;
    bool IsStopRequested() const;
};

}  // namespace environment
}  // namespace parallax
