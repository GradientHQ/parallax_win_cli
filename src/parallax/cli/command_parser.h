#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "tinylog/tinylog.h"

namespace parallax {
namespace cli {

// Command handler function type
using CommandHandler = std::function<int(const std::vector<std::string>& args)>;

// Command definition structure
struct Command {
    std::string name;
    std::string description;
    CommandHandler handler;

    Command(const std::string& n, const std::string& desc, CommandHandler h)
        : name(n), description(desc), handler(h) {}

    ~Command() {}
};

// Command line parser class
class CommandParser {
 public:
    CommandParser();
    ~CommandParser();

    // Parse command line arguments and execute corresponding command
    int Parse(int argc, char* argv[]);

    // Register command
    void RegisterCommand(const std::string& name,
                         const std::string& description,
                         CommandHandler handler);

    // Show help information
    void ShowHelp();

    // Show version information
    void ShowVersion();

 private:
    std::vector<std::unique_ptr<Command>> commands_;
    std::string program_name_;

    // Find command
    Command* FindCommand(const std::string& name);

    // Initialize built-in commands
    void InitializeBuiltinCommands();
};

}  // namespace cli
}  // namespace parallax
