#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include "utils/utils.h"
#include "utils/process.h"
#include "config/config_manager.h"
#include "tinylog/tinylog.h"
#include <iostream>

namespace parallax {
namespace commands {

// Command execution result
enum class CommandResult {
    Success = 0,
    InvalidArgs = 1,
    EnvironmentError = 2,
    ExecutionError = 3
};

// Command execution context
struct CommandContext {
    std::vector<std::string> args;
    std::string ubuntu_version;
    std::string proxy_url;
    bool is_admin = false;
    bool wsl_available = false;
};

// Base command interface
class ICommand {
 public:
    virtual ~ICommand() = default;
    virtual CommandResult Execute(const std::vector<std::string>& args) = 0;
    virtual void ShowHelp() = 0;
    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
};

// Environment requirements structure
struct EnvironmentRequirements {
    bool need_wsl = false;
    bool need_admin = false;
    bool sync_proxy = false;
};

// Base command template class - using CRTP and template method pattern
template <typename Derived>
class BaseCommand : public ICommand {
 public:
    CommandResult Execute(const std::vector<std::string>& args) override final {
        // Template method: define execution flow
        CommandContext context;
        context.args = args;

        // Prioritize help parameters, return directly
        if (ShouldShowHelp(context.args)) {
            ShowHelp();
            return CommandResult::Success;
        }

        // 1. Argument validation
        auto result = ValidateArgs(context);
        if (result != CommandResult::Success) {
            return result;
        }

        // 2. Environment preparation
        result = PrepareEnvironment(context);
        if (result != CommandResult::Success) {
            return result;
        }

        // 3. Execute specific command (implemented by derived class)
        return static_cast<Derived*>(this)->ExecuteImpl(context);
    }

    void ShowHelp() override final {
        static_cast<Derived*>(this)->ShowHelpImpl();
    }

 protected:
    // Virtual function: determine whether to show help information
    // Derived classes can override this method to customize help parameter
    // checking logic
    virtual bool ShouldShowHelp(const std::vector<std::string>& args) {
        // Default behavior: show help if --help or -h appears anywhere
        for (const auto& arg : args) {
            if (arg == "--help" || arg == "-h") {
                return true;
            }
        }
        return false;
    }

    // Template method: argument validation
    virtual CommandResult ValidateArgs(CommandContext& context) {
        // Call derived class argument validation
        return static_cast<Derived*>(this)->ValidateArgsImpl(context);
    }

    // Template method: environment preparation
    virtual CommandResult PrepareEnvironment(CommandContext& context) {
        // Get basic environment information
        context.ubuntu_version =
            parallax::config::ConfigManager::GetInstance().GetConfigValue(
                parallax::config::KEY_WSL_LINUX_DISTRO);
        context.proxy_url = GetProxyUrl();
        context.is_admin = IsAdmin();

        // Check environment requirements
        auto requirements =
            static_cast<Derived*>(this)->GetEnvironmentRequirements();

        if (requirements.need_admin && !context.is_admin) {
            ShowError("Administrator privileges required for this command.");
            return CommandResult::EnvironmentError;
        }

        if (requirements.need_wsl) {
            context.wsl_available = CheckWSLEnvironment(context);
            if (!context.wsl_available) {
                ShowError(
                    "WSL environment is not available. Please run 'parallax "
                    "install' first.");
                return CommandResult::EnvironmentError;
            }
        }

        return CommandResult::Success;
    }

    std::string GetProxyUrl() { return parallax::utils::GetProxyUrl(); }

    bool IsAdmin() { return parallax::utils::IsAdmin(); }

    bool CheckWSLEnvironment(const CommandContext& context) {
        std::string stdout_output, stderr_output;
        int exit_code = parallax::utils::ExecCommandEx(
            "powershell.exe -Command \"wsl --list --quiet\"", 30, stdout_output,
            stderr_output, false, true);

        if (exit_code != 0) {
            return false;
        }

        // Process PowerShell output encoding
        std::string utf8_stdout =
            parallax::utils::ConvertPowerShellOutputToUtf8(stdout_output);

        return utf8_stdout.find(context.ubuntu_version) != std::string::npos;
    }

    void ShowError(const std::string& message) {
        std::cout << "[ERROR] " << message << std::endl;
    }

    void ShowInfo(const std::string& message) {
        std::cout << "[INFO] " << message << std::endl;
    }

    void ShowWarning(const std::string& message) {
        std::cout << "[WARNING] " << message << std::endl;
    }
};

// Administrator command base class - commands requiring administrator
// privileges
template <typename Derived>
class AdminCommand : public BaseCommand<Derived> {
 public:
    EnvironmentRequirements GetEnvironmentRequirements() {
        EnvironmentRequirements req;
        req.need_admin = true;
        return req;
    }
};

// WSL command base class - commands requiring WSL environment
template <typename Derived>
class WSLCommand : public BaseCommand<Derived> {
 public:
    EnvironmentRequirements GetEnvironmentRequirements() {
        EnvironmentRequirements req;
        req.need_wsl = true;
        return req;
    }

 protected:
    // Build WSL command prefix, including -u root parameter
    std::string GetWSLCommandPrefix(const CommandContext& context) {
        return parallax::utils::GetWSLCommandPrefix(context.ubuntu_version);
    }

    // Build complete WSL bash command
    std::string BuildWSLCommand(const CommandContext& context,
                                const std::string& command) {
        return parallax::utils::BuildWSLCommand(context.ubuntu_version,
                                                command);
    }

    // Build WSL command (without using bash -c, execute command directly)
    std::string BuildWSLDirectCommand(const CommandContext& context,
                                      const std::string& command) {
        return parallax::utils::BuildWSLDirectCommand(context.ubuntu_version,
                                                      command);
    }
};

}  // namespace commands
}  // namespace parallax