#pragma once

#include "base_command.h"

namespace parallax {
namespace cli {

// parallax config command implementation - using new architecture
class ConfigCommand : public parallax::commands::BaseCommand<ConfigCommand> {
 public:
    ConfigCommand();

    std::string GetName() const override { return "config"; }
    std::string GetDescription() const override {
        return "Configure Parallax settings";
    }

    parallax::commands::EnvironmentRequirements GetEnvironmentRequirements() {
        // config command does not require special environment requirements
        parallax::commands::EnvironmentRequirements req;
        return req;
    }

    parallax::commands::CommandResult ValidateArgsImpl(
        parallax::commands::CommandContext& context);
    parallax::commands::CommandResult ExecuteImpl(
        const parallax::commands::CommandContext& context);
    void ShowHelpImpl();

 private:
    void ShowUsage() const;
    int SetConfigValue(const std::string& key, const std::string& value);
    int ListConfig();
    int ResetConfig();
    bool IsEmptyValue(const std::string& value) const;
};

}  // namespace cli
}  // namespace parallax