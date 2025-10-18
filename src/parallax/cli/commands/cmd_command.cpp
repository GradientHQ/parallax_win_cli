#include "cmd_command.h"
#include "utils/wsl_process.h"
#include "tinylog/tinylog.h"
#include <sstream>

namespace parallax {
namespace commands {

bool CmdCommand::ShouldShowHelp(const std::vector<std::string>& args) {
    // For cmd command, only show cmd command help when the first parameter is
    // --help or -h Otherwise pass the -h parameter to the subcommand to be
    // executed
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        return true;
    }
    return false;
}

CommandResult CmdCommand::ValidateArgsImpl(CommandContext& context) {
    // cmd command requires at least one parameter (command to execute)
    if (context.args.empty()) {
        this->ShowError("No command specified");
        this->ShowError("Usage: parallax cmd [--venv] <command> [args...]");
        this->ShowError("Run 'parallax cmd --help' for usage information.");
        return CommandResult::InvalidArgs;
    }

    // Parse arguments
    auto options = ParseArguments(context.args);

    if (options.command_args.empty()) {
        this->ShowError("No command specified after options");
        this->ShowError("Usage: parallax cmd [--venv] <command> [args...]");
        this->ShowError("Run 'parallax cmd --help' for usage information.");
        return CommandResult::InvalidArgs;
    }

    return CommandResult::Success;
}

CommandResult CmdCommand::ExecuteImpl(const CommandContext& context) {
    // Parse command options
    auto options = ParseArguments(context.args);

    // Build complete command
    std::string full_command = BuildCommand(context, options);

    info_log("Executing command: %s", full_command.c_str());

    // Display execution information
    if (options.use_venv) {
        this->ShowInfo("Executing command in Python virtual environment...");
    } else {
        this->ShowInfo("Executing command in WSL...");
    }

    // Execute command
    if (!ExecuteCommand(context, full_command)) {
        this->ShowError("Command execution failed");
        return CommandResult::ExecutionError;
    }

    return CommandResult::Success;
}

void CmdCommand::ShowHelpImpl() {
    std::cout << "Usage: parallax cmd [options] <command> [args...]\n\n";
    std::cout << "Execute commands in WSL or Python virtual environment.\n\n";
    std::cout << "Options:\n";
    std::cout
        << "  --venv          Execute command in Python virtual environment\n";
    std::cout
        << "                  (activates ~/parallax/venv before execution)\n";
    std::cout << "  --help, -h      Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout
        << "  parallax cmd ls -la                    # List files in WSL\n";
    std::cout
        << "  parallax cmd --venv pip list           # List Python packages\n";
    std::cout
        << "  parallax cmd --venv python --version   # Check Python version\n";
    std::cout << "  parallax cmd --venv python -m parallax.launch  # Run "
                 "Parallax\n\n";
    std::cout << "Notes:\n";
    std::cout << "  - Commands are executed with root privileges in WSL\n";
    std::cout
        << "  - Proxy settings are automatically applied when available\n";
    std::cout << "  - Virtual environment commands require ~/parallax/venv to "
                 "exist\n";
}

CmdCommand::CmdOptions CmdCommand::ParseArguments(
    const std::vector<std::string>& args) {
    CmdOptions options;

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "--venv") {
            options.use_venv = true;
        } else {
            // Remaining are commands and parameters to execute
            for (size_t j = i; j < args.size(); ++j) {
                options.command_args.push_back(args[j]);
            }
            break;
        }
    }

    return options;
}

std::string CmdCommand::BuildCommand(const CommandContext& context,
                                     const CmdOptions& options) {
    // Build command string to execute
    std::ostringstream cmd_stream;
    for (size_t i = 0; i < options.command_args.size(); ++i) {
        if (i > 0) cmd_stream << " ";

        // Wrap parameters containing spaces with quotes
        const std::string& arg = options.command_args[i];
        if (arg.find(' ') != std::string::npos) {
            cmd_stream << "\"" << arg << "\"";
        } else {
            cmd_stream << arg;
        }
    }

    std::string command = cmd_stream.str();
    std::string full_command;

    if (options.use_venv) {
        // Execute in virtual environment
        full_command = "cd ~/parallax && source ./venv/bin/activate";

        // Add proxy support (similar to implementation in model_commands.cpp)
        if (!context.proxy_url.empty()) {
            full_command += " && HTTP_PROXY=\"" + context.proxy_url +
                            "\" HTTPS_PROXY=\"" + context.proxy_url + "\" " +
                            command;
        } else {
            full_command += " && " + command;
        }
    } else {
        // Execute directly in WSL
        if (!context.proxy_url.empty()) {
            full_command = "HTTP_PROXY=\"" + context.proxy_url +
                           "\" HTTPS_PROXY=\"" + context.proxy_url + "\" " +
                           command;
        } else {
            full_command = command;
        }
    }

    // Use BuildWSLCommand to wrap as complete WSL command
    return this->BuildWSLCommand(context, full_command);
}

bool CmdCommand::ExecuteCommand(const CommandContext& context,
                                const std::string& full_command) {
    WSLProcess wsl_process;
    int exit_code = wsl_process.Execute(full_command);

    if (exit_code != 0) {
        error_log("Command execution failed with exit code: %d", exit_code);
        return false;
    }

    return true;
}

}  // namespace commands
}  // namespace parallax
