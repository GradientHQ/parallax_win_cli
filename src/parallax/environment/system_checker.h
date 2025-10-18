#pragma once

#include "environment_installer.h"  // For ComponentResult definition
#include "base_component.h"
#include "command_executor.h"
#include <memory>

namespace parallax {
namespace environment {

/**
 * @brief OS Version checker component
 */
class OSVersionChecker : public BaseEnvironmentComponent {
 public:
    explicit OSVersionChecker(std::shared_ptr<ExecutionContext> context);

    ComponentResult Check() override;
    ComponentResult Install() override {
        return Check();
    }  // OS version cannot be "installed"
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;
};

/**
 * @brief NVIDIA GPU hardware checker component
 */
class NvidiaGPUChecker : public BaseEnvironmentComponent {
 public:
    explicit NvidiaGPUChecker(std::shared_ptr<ExecutionContext> context);

    ComponentResult Check() override;
    ComponentResult Install() override {
        return Check();
    }  // GPU hardware cannot be "installed"
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    bool IsGPUMeetsMinimumRequirement(const std::string& gpu_name);
};

/**
 * @brief NVIDIA Driver checker component
 */
class NvidiaDriverChecker : public BaseEnvironmentComponent {
 public:
    explicit NvidiaDriverChecker(std::shared_ptr<ExecutionContext> context);

    ComponentResult Check() override;
    ComponentResult Install() override {
        return Check();
    }  // Driver installation is external
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;
};

/**
 * @brief BIOS Virtualization checker component
 */
class BIOSVirtualizationChecker : public BaseEnvironmentComponent {
 public:
    explicit BIOSVirtualizationChecker(
        std::shared_ptr<ExecutionContext> context,
        std::shared_ptr<CommandExecutor> executor);

    ComponentResult Check() override;
    ComponentResult Install() override {
        return Check();
    }  // BIOS settings cannot be automated
    EnvironmentComponent GetComponentType() const override;
    std::string GetComponentName() const override;

 private:
    std::shared_ptr<CommandExecutor> executor_;
};

}  // namespace environment
}  // namespace parallax
