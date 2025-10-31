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
        // Check if it's a help request
        if (context.args.size() == 1 &&
            (context.args[0] == "--help" || context.args[0] == "-h")) {
            this->ShowHelpImpl();
            return CommandResult::Success;
        }

        // run command can be executed with any user-provided parameters
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
        std::cout << "Usage: parallax run [args...]\n\n";
        std::cout
            << "Run Parallax distributed inference server directly in WSL.\n\n";
        std::cout << "This command will:\n";
        std::cout << "  1. Change to ~/parallax directory\n";
        std::cout << "  2. Activate the Python virtual environment\n";
        std::cout << "  3. Set proxy environment variables (if configured)\n";
        std::cout << "  4. Execute 'parallax run' with your arguments\n\n";
        std::cout << "Arguments:\n";
        std::cout << "  args...       Arguments to pass to parallax run "
                     "(optional)\n\n";
        std::cout << "Options:\n";
        std::cout << "  --help, -h    Show this help message\n\n";
        std::cout << "Examples:\n";
        std::cout
            << "  parallax run                             # Execute: parallax "
               "run\n";
        std::cout << "  parallax run -m Qwen/Qwen3-0.6B         # Execute: parallax "
                     "run -m Qwen/Qwen3-0.6B\n";
        std::cout
            << "  parallax run --port 8080                 # Execute: parallax "
               "run --port 8080\n\n";
        std::cout << "Note: All arguments will be passed to the built-in "
                     "parallax run script\n";
        std::cout << "      in the Parallax Python virtual environment.\n";
    }

 private:
    bool CheckLaunchScriptExists(const CommandContext& context);
    bool IsParallaxProcessRunning(const CommandContext& context);
    bool RunParallaxScript(const CommandContext& context);
    std::string BuildRunCommand(const CommandContext& context);
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
};

// Chat command - access chat interface from non-scheduler computer
class ModelChatCommand : public WSLCommand<ModelChatCommand> {
 public:
    std::string GetName() const override { return "chat"; }
    std::string GetDescription() const override {
        return "Access chat interface from non-scheduler computer";
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
    std::string BuildChatCommand(const CommandContext& context);
};

}  // namespace commands
}  // namespace parallax