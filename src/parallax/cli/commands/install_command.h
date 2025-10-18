#pragma once

#include "base_command.h"

namespace parallax {
namespace environment {
struct EnvironmentResult;
}

namespace commands {

// parallax install command implementation - using new architecture
class InstallCommand : public AdminCommand<InstallCommand> {
 public:
    InstallCommand();
    ~InstallCommand();

    std::string GetName() const override { return "install"; }
    std::string GetDescription() const override {
        return "Install and configure Parallax environment";
    }

    CommandResult ValidateArgsImpl(CommandContext& context);
    CommandResult ExecuteImpl(const CommandContext& context);
    void ShowHelpImpl();

 private:
    // Install all environment components
    int InstallAllComponents();

    // Display installation results
    void DisplayResults(const parallax::environment::EnvironmentResult& result);

    // Progress callback function
    static void ProgressCallback(const std::string& step,
                                 const std::string& message,
                                 int progress_percent);
};

}  // namespace commands
}  // namespace parallax
