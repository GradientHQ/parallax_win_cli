#include "config_command.h"
#include "config/config_manager.h"
#include "utils/utils.h"
#include "utils/process.h"
#include "tinylog/tinylog.h"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace parallax {
namespace cli {

ConfigCommand::ConfigCommand() {}

parallax::commands::CommandResult ConfigCommand::ValidateArgsImpl(
    parallax::commands::CommandContext& context) {
    if (context.args.empty()) {
        this->ShowError("config command requires at least 1 argument");
        ShowUsage();
        return parallax::commands::CommandResult::InvalidArgs;
    }

    // Handle help parameters
    const std::string& command = context.args[0];
    if (command == "--help" || command == "-h") {
        // Help parameters will be handled by base class, return success
        // directly here
        return parallax::commands::CommandResult::Success;
    }

    return parallax::commands::CommandResult::Success;
}

parallax::commands::CommandResult ConfigCommand::ExecuteImpl(
    const parallax::commands::CommandContext& context) {
    const std::string& command = context.args[0];

    // Handle subcommands
    if (command == "set") {
        if (context.args.size() != 3) {
            this->ShowError(
                "'set' command requires exactly 2 arguments: key value");
            ShowUsage();
            return parallax::commands::CommandResult::InvalidArgs;
        }
        int result = SetConfigValue(context.args[1], context.args[2]);
        return (result == 0)
                   ? parallax::commands::CommandResult::Success
                   : parallax::commands::CommandResult::ExecutionError;
    } else if (command == "get") {
        if (context.args.size() != 2) {
            this->ShowError("'get' command requires exactly 1 argument: key");
            ShowUsage();
            return parallax::commands::CommandResult::InvalidArgs;
        }

        std::string value =
            parallax::config::ConfigManager::GetInstance().GetConfigValue(
                context.args[1]);
        if (value.empty()) {
            this->ShowError("Configuration key '" + context.args[1] +
                            "' not found or is empty");
            return parallax::commands::CommandResult::ExecutionError;
        }

        std::cout << context.args[1] << "=" << value << std::endl;
        return parallax::commands::CommandResult::Success;
    } else if (command == "list") {
        int result = ListConfig();
        return (result == 0)
                   ? parallax::commands::CommandResult::Success
                   : parallax::commands::CommandResult::ExecutionError;
    } else if (command == "reset") {
        int result = ResetConfig();
        return (result == 0)
                   ? parallax::commands::CommandResult::Success
                   : parallax::commands::CommandResult::ExecutionError;
    } else {
        this->ShowError("Unknown config command: " + command);
        ShowUsage();
        return parallax::commands::CommandResult::InvalidArgs;
    }
}

void ConfigCommand::ShowHelpImpl() {
    std::cout << "Usage: parallax config <command> [arguments]\n\n";
    std::cout << "Configure Parallax settings.\n\n";
    std::cout << "Commands:\n";
    std::cout << "  set <key> <value>    Set configuration value\n";
    std::cout << "  get <key>            Get configuration value\n";
    std::cout << "  list                 List all configuration values\n";
    std::cout
        << "  reset                Reset all configuration to defaults\n\n";
    std::cout << "Available configuration keys:\n";
    std::cout << "  proxy_url           HTTP/SOCKS proxy URL (e.g., "
                 "http://127.0.0.1:7890)\n";
    std::cout << "  wsl_distro          WSL distribution name (default: "
                 "Ubuntu-24.04)\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -h          Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  parallax config set proxy_url http://127.0.0.1:7890\n";
    std::cout << "  parallax config get proxy_url\n";
    std::cout << "  parallax config list\n";
    std::cout << "  parallax config reset\n";
}

void ConfigCommand::ShowUsage() const {
    std::cout << "Usage: parallax config <command|key> [value]" << std::endl;
    std::cout << "Use 'parallax config --help' for more information."
              << std::endl;
}

int ConfigCommand::SetConfigValue(const std::string& key,
                                  const std::string& value) {
    auto& config_manager = parallax::config::ConfigManager::GetInstance();

    // Validate configuration key
    if (!config_manager.IsValidConfigKey(key)) {
        std::cout << "Error: Invalid configuration key: " << key << std::endl;
        std::cout << "Valid keys are:" << std::endl;
        std::cout << "  proxy_url" << std::endl;
        std::cout << "  nvidia_repo_base_url" << std::endl;
        std::cout << "  wsl_linux_distro" << std::endl;
        std::cout << "  wsl_installer_url" << std::endl;
        std::cout << "  wsl_kernel_url" << std::endl;
        return 1;
    }

    // Check if value is empty (for non-proxy_url keys, empty values are not
    // allowed)
    if (key != parallax::config::KEY_PROXY_URL && IsEmptyValue(value)) {
        std::cout << "Error: Configuration value cannot be empty for key '"
                  << key << "'" << std::endl;
        std::cout
            << "Note: Only 'proxy_url' can be set to empty to disable proxy"
            << std::endl;
        return 1;
    }

    try {
        // Set new value
        config_manager.SetConfigValue(key, value);

        // Save configuration
        if (!config_manager.SaveConfig()) {
            std::cout << "Error: Failed to save configuration" << std::endl;
            return 1;
        }

        std::cout << "Configuration updated successfully:" << std::endl;
        std::cout << "  " << key << " = " << value << std::endl;

        info_log("Configuration updated: %s = %s", key.c_str(), value.c_str());

        return 0;
    } catch (const std::exception& e) {
        std::cout << "Error: Failed to update configuration: " << e.what()
                  << std::endl;
        error_log("Failed to update configuration: %s", e.what());
        return 1;
    }
}

int ConfigCommand::ListConfig() {
    auto& config_manager = parallax::config::ConfigManager::GetInstance();

    try {
        // Get all configuration items
        auto all_configs = config_manager.GetAllConfigValues();

        if (all_configs.empty()) {
            std::cout << "No configuration values set." << std::endl;
            return 0;
        }

        std::cout << "Current configuration values:" << std::endl;
        std::cout << std::endl;

        // Display sorted by key name
        for (const auto& kv : all_configs) {
            std::cout << "  " << kv.first << " = ";
            if (kv.second.empty()) {
                std::cout << "(empty)" << std::endl;
            } else {
                std::cout << kv.second << std::endl;
            }
        }

        return 0;
    } catch (const std::exception& e) {
        std::cout << "Error: Failed to list configuration: " << e.what()
                  << std::endl;
        return 1;
    }
}

int ConfigCommand::ResetConfig() {
    auto& config_manager = parallax::config::ConfigManager::GetInstance();

    try {
        // Reset to default values
        config_manager.ResetToDefaults();

        // Save configuration
        if (!config_manager.SaveConfig()) {
            std::cout << "Error: Failed to save reset configuration"
                      << std::endl;
            return 1;
        }

        std::cout << "Configuration reset to default values successfully."
                  << std::endl;

        info_log("Configuration reset to defaults by user");

        return 0;
    } catch (const std::exception& e) {
        std::cout << "Error: Failed to reset configuration: " << e.what()
                  << std::endl;
        return 1;
    }
}

bool ConfigCommand::IsEmptyValue(const std::string& value) const {
    // Check if empty or contains only whitespace characters
    return value.empty() || std::all_of(value.begin(), value.end(), [](char c) {
               return std::isspace(static_cast<unsigned char>(c));
           });
}

}  // namespace cli
}  // namespace parallax