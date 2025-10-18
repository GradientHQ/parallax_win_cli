#pragma once

#include "base_command.h"

namespace parallax {
namespace environment {
struct EnvironmentResult;
}

namespace commands {

// parallax check command implementation - using new architecture
class CheckCommand : public AdminCommand<CheckCommand> {
 public:
    CheckCommand();
    ~CheckCommand();

    std::string GetName() const override { return "check"; }
    std::string GetDescription() const override {
        return "Check Parallax environment requirements";
    }

    CommandResult ValidateArgsImpl(CommandContext& context);
    CommandResult ExecuteImpl(const CommandContext& context);
    void ShowHelpImpl();

 private:
    // Check all environment components
    int CheckAllComponents();

    // Display check results
    void DisplayResults(const parallax::environment::EnvironmentResult& result);
};

}  // namespace commands
}  // namespace parallax
