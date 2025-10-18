#pragma once

#include "environment_installer.h"  // For ComponentResult definition
#include "base_component.h"
#include "command_executor.h"
#include <memory>
#include <vector>
#include <tuple>

namespace parallax {
namespace environment {

/**
 * @brief CUDA Toolkit installer component
 */
class CudaToolkitInstaller : public BaseEnvironmentComponent {
 public:
    explicit CudaToolkitInstaller(std::shared_ptr<ExecutionContext> context,
                                  std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsCudaToolkitInstalled();
};

/**
 * @brief Rust Cargo installer component
 */
class CargoInstaller : public BaseEnvironmentComponent {
 public:
    explicit CargoInstaller(std::shared_ptr<ExecutionContext> context,
                            std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsCargoInstalled();
};

/**
 * @brief Ninja build tool installer component
 */
class NinjaInstaller : public BaseEnvironmentComponent {
 public:
    explicit NinjaInstaller(std::shared_ptr<ExecutionContext> context,
                            std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsNinjaInstalled();
};

/**
 * @brief pip upgrade manager component
 */
class PipUpgradeManager : public BaseEnvironmentComponent {
 public:
    explicit PipUpgradeManager(std::shared_ptr<ExecutionContext> context,
                               std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsPipUpToDate();
};

/**
 * @brief Parallax project installer component
 */
class ParallaxProjectInstaller : public BaseEnvironmentComponent {
 public:
    explicit ParallaxProjectInstaller(
        std::shared_ptr<ExecutionContext> context,
        std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override;
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;

    bool IsParallaxProjectInstalled();
    bool HasParallaxProjectGitUpdates();

    // Helper method for executing command sequences
    ComponentResult ExecuteCommandSequence(
        const std::vector<std::tuple<std::string, std::string, int, bool>>&
            commands,
        const std::string& operation_name);
};

}  // namespace environment
}  // namespace parallax
