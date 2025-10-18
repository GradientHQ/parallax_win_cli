#include "command_parser.h"
#include "commands/check_command.h"
#include "commands/install_command.h"
#include "commands/config_command.h"
#include "commands/model_commands.h"
#include "commands/cmd_command.h"
#include "tinylog/tinylog.h"
#include <iostream>
#include <algorithm>

namespace parallax {
namespace cli {

CommandParser::CommandParser() {
    info_log("parallax cmd enter");
    InitializeBuiltinCommands();
}

CommandParser::~CommandParser() { info_log("parallax cmd exit"); }

int CommandParser::Parse(int argc, char* argv[]) {
    if (argc < 1) {
        error_log("Invalid argument count");
        return 1;
    }

    program_name_ = argv[0];

    // If no arguments, show help
    if (argc == 1) {
        ShowHelp();
        return 0;
    }

    std::string command_name = argv[1];

    // Handle built-in options
    if (command_name == "--help" || command_name == "-h") {
        ShowHelp();
        return 0;
    }

    if (command_name == "--version" || command_name == "-v") {
        ShowVersion();
        return 0;
    }

    // Find and execute command
    Command* command = FindCommand(command_name);
    if (!command) {
        std::cerr << "Unknown command: " << command_name << std::endl;
        std::cerr << "Run 'parallax --help' for usage information."
                  << std::endl;
        return 1;
    }

    // Prepare command arguments (skip program name and command name)
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    info_log("Executing command: %s with %d arguments", command_name.c_str(),
             static_cast<int>(args.size()));

    // Execute command
    try {
        return command->handler(args);
    } catch (const std::exception& e) {
        error_log("Command execution failed: %s", e.what());
        std::cerr << "Error executing command '" << command_name
                  << "': " << e.what() << std::endl;
        return 1;
    }
}

void CommandParser::RegisterCommand(const std::string& name,
                                    const std::string& description,
                                    CommandHandler handler) {
    commands_.push_back(std::make_unique<Command>(name, description, handler));
}

void CommandParser::ShowHelp() {
    std::cout << "Parallax - Distributed Inference Framework\n\n";
    std::cout << "Usage: parallax <command> [options]\n\n";
    std::cout << "Available commands:\n";

    for (const auto& command : commands_) {
        std::cout << "  " << command->name;
        // Align description text
        int padding = (15 - static_cast<int>(command->name.length()));
        if (padding < 1) padding = 1;
        for (int i = 0; i < padding; ++i) {
            std::cout << " ";
        }
        std::cout << command->description << std::endl;
    }

    std::cout << "\nGlobal options:\n";
    std::cout << "  --help, -h      Show this help message\n";
    std::cout << "  --version, -v   Show version information\n";
    std::cout << "\nUse 'parallax <command> --help' for more information about "
                 "a command.\n";
}

void CommandParser::ShowVersion() {
    std::cout << "Parallax version 1.0.0\n";
    std::cout << "Distributed Inference Framework\n";
}

Command* CommandParser::FindCommand(const std::string& name) {
    auto it = std::find_if(commands_.begin(), commands_.end(),
                           [&name](const std::unique_ptr<Command>& cmd) {
                               return cmd->name == name;
                           });

    return (it != commands_.end()) ? it->get() : nullptr;
}

void CommandParser::InitializeBuiltinCommands() {
    // Register check command (new architecture)
    RegisterCommand("check", "Check environment requirements",
                    [](const std::vector<std::string>& args) -> int {
                        parallax::commands::CheckCommand check_cmd;
                        auto result = check_cmd.Execute(args);
                        return static_cast<int>(result);
                    });

    // Register install command (new architecture)
    RegisterCommand("install", "Install required environment components",
                    [](const std::vector<std::string>& args) -> int {
                        parallax::commands::InstallCommand install_cmd;
                        auto result = install_cmd.Execute(args);
                        return static_cast<int>(result);
                    });

    // Register config command (new architecture)
    RegisterCommand("config", "Configure parallax settings",
                    [](const std::vector<std::string>& args) -> int {
                        parallax::cli::ConfigCommand config_cmd;
                        auto result = config_cmd.Execute(args);
                        return static_cast<int>(result);
                    });

    // Register run command (WSL direct run version)
    RegisterCommand("run", "Run Parallax inference server directly in WSL",
                    [](const std::vector<std::string>& args) -> int {
                        parallax::commands::ModelRunCommand run_cmd;
                        auto result = run_cmd.Execute(args);
                        return static_cast<int>(result);
                    });

    // Register join command (join distributed inference cluster)
    RegisterCommand("join", "Join distributed inference cluster as a node",
                    [](const std::vector<std::string>& args) -> int {
                        parallax::commands::ModelJoinCommand join_cmd;
                        auto result = join_cmd.Execute(args);
                        return static_cast<int>(result);
                    });

    // Register cmd command (pass-through command to WSL or virtual environment)
    RegisterCommand("cmd",
                    "Execute commands in WSL or Python virtual environment",
                    [](const std::vector<std::string>& args) -> int {
                        parallax::commands::CmdCommand cmd_cmd;
                        auto result = cmd_cmd.Execute(args);
                        return static_cast<int>(result);
                    });
}

}  // namespace cli
}  // namespace parallax
