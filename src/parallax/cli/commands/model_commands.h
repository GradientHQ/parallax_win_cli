#pragma once

#include "base_command.h"
#include "utils/wsl_process.h"
#include <iostream>

namespace parallax {
namespace commands {

// Run command - directly run Parallax Python script in WSL
class ModelRunCommand : public WSLCommand<ModelRunCommand> {
 public:
    std::string GetName() const override { return "run"; }
    std::string GetDescription() const override {
        return "Run Parallax inference server directly in WSL";
    }

    EnvironmentRequirements GetEnvironmentRequirements() {
        EnvironmentRequirements req;
        req.need_wsl = true;
        req.sync_proxy = false;
        return req;
    }

    CommandResult ValidateArgsImpl(CommandContext& context) {
        // runex command does not accept additional parameters
        for (const auto& arg : context.args) {
            if (arg != "--help" && arg != "-h") {
                this->ShowError("Unknown parameter: " + arg);
                this->ShowError("Usage: parallax run [--help|-h]");
                return CommandResult::InvalidArgs;
            }
        }
        return CommandResult::Success;
    }

    CommandResult ExecuteImpl(const CommandContext& context) {
        // Check if launch.py exists
        // if (!CheckLaunchScriptExists(context)) {
        //     this->ShowError(
        //         "Parallax launch script not found at "
        //         "~/parallax/src/parallax/launch.py");
        //     this->ShowError(
        //         "Please run 'parallax check' to verify your environment "
        //         "setup.");
        //     return CommandResult::ExecutionError;
        // }

        // Check if there are already running processes
        // if (IsParallaxProcessRunning(context)) {
        //     this->ShowError(
        //         "Parallax server is already running. Use 'parallax stop' to "
        //         "stop it first.");
        //     return CommandResult::ExecutionError;
        // }

        // Start Parallax server
        this->ShowInfo("Starting Parallax inference server...");
        this->ShowInfo("Server will be accessible at http://localhost:3000");
        this->ShowInfo("Press Ctrl+C to stop the server\n");

        if (!RunParallaxScript(context)) {
            this->ShowError("Failed to start Parallax server");
            return CommandResult::ExecutionError;
        }

        this->ShowInfo("Parallax server stopped.");
        return CommandResult::Success;
    }

    void ShowHelpImpl() {
        std::cout << "Usage: parallax run [options]\n\n";
        std::cout
            << "Run Parallax distributed inference server directly in WSL.\n\n";
        std::cout << "This command will:\n";
        std::cout << "  1. Check if ~/parallax/src/parallax/launch.py exists\n";
        std::cout << "  2. Start Parallax inference server with default "
                     "configuration\n\n";
        std::cout << "Default Configuration:\n";
        std::cout << "  Model:          Qwen/Qwen3-0.6B\n";
        std::cout << "  Host:           0.0.0.0\n";
        std::cout << "  Port:           3000\n";
        std::cout << "  Max Batch Size: 8\n";
        std::cout << "  Start Layer:    0\n";
        std::cout << "  End Layer:      28\n\n";
        std::cout << "Options:\n";
        std::cout << "  --help, -h      Show this help message\n\n";
        std::cout
            << "Note: The server will be accessible at http://localhost:3000\n";
        std::cout << "      Use 'parallax stop' to stop the running server.\n";
    }

 private:
    bool CheckLaunchScriptExists(const CommandContext& context);
    bool IsParallaxProcessRunning(const CommandContext& context);
    bool RunParallaxScript(const CommandContext& context);
};

// Join command - join distributed inference cluster as a node
class ModelJoinCommand : public WSLCommand<ModelJoinCommand> {
 public:
    std::string GetName() const override { return "join"; }
    std::string GetDescription() const override {
        return "Join distributed inference cluster as a node";
    }

    EnvironmentRequirements GetEnvironmentRequirements() {
        EnvironmentRequirements req;
        req.need_wsl = true;
        req.sync_proxy = true;
        return req;
    }

    CommandResult ValidateArgsImpl(CommandContext& context);
    CommandResult ExecuteImpl(const CommandContext& context);
    void ShowHelpImpl();

 private:
    std::string BuildJoinCommand(const CommandContext& context);
    std::string EscapeForShell(const std::string& arg);
};

}  // namespace commands
}  // namespace parallax