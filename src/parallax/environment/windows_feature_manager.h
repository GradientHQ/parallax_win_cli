#pragma once

#include "environment_installer.h"  // For ComponentResult definition
#include "base_component.h"
#include "command_executor.h"
#include <memory>

namespace parallax {
namespace environment {

/**
 * @brief WSL Windows Feature manager component
 */
class WSLFeatureManager : public BaseEnvironmentComponent {
 public:
    explicit WSLFeatureManager(std::shared_ptr<ExecutionContext> context,
                               std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;
};

/**
 * @brief Virtual Machine Platform feature manager component
 */
class VirtualMachinePlatformManager : public BaseEnvironmentComponent {
 public:
    explicit VirtualMachinePlatformManager(
        std::shared_ptr<ExecutionContext> context,
        std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;
};

/**
 * @brief WSL Package installer component
 */
class WSLPackageInstaller : public BaseEnvironmentComponent {
 public:
    explicit WSLPackageInstaller(std::shared_ptr<ExecutionContext> context,
                                 std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsWSLPackageInstalled();
    bool WaitForWSLServiceStart(int timeout_seconds);
};

/**
 * @brief WSL2 Kernel installer component
 */
class WSL2KernelInstaller : public BaseEnvironmentComponent {
 public:
    explicit WSL2KernelInstaller(std::shared_ptr<ExecutionContext> context,
                                 std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsWSL2KernelInstalled();
};

/**
 * @brief WSL2 Default Version manager component
 */
class WSL2DefaultVersionManager : public BaseEnvironmentComponent {
 public:
    explicit WSL2DefaultVersionManager(
        std::shared_ptr<ExecutionContext> context,
        std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsWSL2DefaultVersion();

 public:  // Make this public so UbuntuInstaller can access it
    bool IsWSLPackageInstalled();
};

/**
 * @brief Ubuntu distribution installer component
 */
class UbuntuInstaller : public BaseEnvironmentComponent {
 public:
    explicit UbuntuInstaller(std::shared_ptr<ExecutionContext> context,
                             std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsUbuntuInstalled();
    bool CheckUbuntuInstalled();
};

}  // namespace environment
}  // namespace parallax
