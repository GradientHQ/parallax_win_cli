#pragma once

#include "base_command.h"
#include <vector>
#include <string>

namespace parallax {
namespace commands {

// Cmd command - pass through commands to WSL or virtual environment for
// execution
class CmdCommand : public WSLCommand<CmdCommand> {
 public:
    std::string GetName() const override { return "cmd"; }
    std::string GetDescription() const override {
        return "Execute commands in WSL or Python virtual environment";
    }

    EnvironmentRequirements GetEnvironmentRequirements() {
        EnvironmentRequirements req;
        req.need_wsl = true;
        req.sync_proxy = false;
        return req;
    }

    CommandResult ValidateArgsImpl(CommandContext& context);
    CommandResult ExecuteImpl(const CommandContext& context);
    void ShowHelpImpl();

 protected:
    // Override help parameter checking logic: only show cmd command help when
    // the first parameter is a help parameter
    bool ShouldShowHelp(const std::vector<std::string>& args) override;

 private:
    struct CmdOptions {
        bool use_venv = false;
        std::vector<std::string> command_args;
    };

    CmdOptions ParseArguments(const std::vector<std::string>& args);
    std::string BuildCommand(const CommandContext& context,
                             const CmdOptions& options);
    bool ExecuteCommand(const CommandContext& context,
                        const std::string& full_command);
};

}  // namespace commands
}  // namespace parallax
