#include "model_commands.h"
#include "utils/wsl_process.h"
#include "utils/utils.h"
#include "tinylog/tinylog.h"
#include <sstream>

namespace parallax {
namespace commands {

// ModelRunCommand implementation (WSL version)
bool ModelRunCommand::CheckLaunchScriptExists(const CommandContext& context) {
    std::string check_cmd = "test -f ~/parallax/src/parallax/launch.py";
    std::string wsl_command = BuildWSLCommand(context, check_cmd);

    std::string stdout_output, stderr_output;
    int exit_code = parallax::utils::ExecCommandEx(
        wsl_command, 30, stdout_output, stderr_output, false, true);

    return exit_code == 0;
}

bool ModelRunCommand::IsParallaxProcessRunning(const CommandContext& context) {
    // Use pgrep to find processes, matching python/python3 and
    // parallax/launch.py
    std::string check_cmd = "pgrep -f 'python[0-9]*.*parallax/launch.py'";
    std::string wsl_command = BuildWSLCommand(context, check_cmd);

    std::string stdout_output, stderr_output;
    int exit_code = parallax::utils::ExecCommandEx(
        wsl_command, 30, stdout_output, stderr_output, false, true);

    // pgrep returns 0 if matching process is found, returns 1 if not found
    if (exit_code == 0) {
        info_log("Parallax process found: %s", stdout_output.c_str());
    }
    return exit_code == 0;
}

bool ModelRunCommand::RunParallaxScript(const CommandContext& context) {
    std::string launch_cmd = "cd ~/parallax && source ./venv/bin/activate";

    // If proxy is configured, add proxy environment variables
    if (!context.proxy_url.empty()) {
        launch_cmd += " && HTTP_PROXY=\"" + context.proxy_url +
                      "\" HTTPS_PROXY=\"" + context.proxy_url +
                      "\" parallax run";
    } else {
        launch_cmd += " && parallax run";
    }

    std::string wsl_command = BuildWSLCommand(context, launch_cmd);

    info_log("Executing Parallax launch command: %s", wsl_command.c_str());

    WSLProcess wsl_process;
    int exit_code = wsl_process.Execute(wsl_command);

    return exit_code == 0;
}

// ModelJoinCommand implementation
CommandResult ModelJoinCommand::ValidateArgsImpl(CommandContext& context) {
    // Check if it's a help request
    if (context.args.size() == 1 &&
        (context.args[0] == "--help" || context.args[0] == "-h")) {
        ShowHelpImpl();
        return CommandResult::Success;
    }

    // join command can be executed without parameters (using default
    // scripts/join.sh)
    return CommandResult::Success;
}

CommandResult ModelJoinCommand::ExecuteImpl(const CommandContext& context) {
    // Build cluster join command: parallax join [user parameters...]
    std::string join_command = BuildJoinCommand(context);

    // Build complete WSL command: cd ~/parallax && source ./venv/bin/activate
    // && parallax join [args...]
    std::string full_command = "cd ~/parallax && source ./venv/bin/activate";

    // If proxy is configured, add proxy environment variables
    if (!context.proxy_url.empty()) {
        full_command += " && HTTP_PROXY=\"" + context.proxy_url +
                        "\" HTTPS_PROXY=\"" + context.proxy_url + "\" " +
                        join_command;
    } else {
        full_command += " && " + join_command;
    }

    std::string wsl_command = BuildWSLCommand(context, full_command);

    info_log("Executing cluster join command: %s", wsl_command.c_str());

    // Use WSLProcess to execute command for real-time output
    WSLProcess wsl_process;
    int exit_code = wsl_process.Execute(wsl_command);

    if (exit_code == 0) {
        ShowInfo("Successfully joined the distributed inference cluster.");
        return CommandResult::Success;
    } else {
        ShowError("Failed to join cluster with exit code: " +
                  std::to_string(exit_code));
        return CommandResult::ExecutionError;
    }
}

void ModelJoinCommand::ShowHelpImpl() {
    std::cout << "Usage: parallax join [args...]\n\n";
    std::cout << "Join a distributed inference cluster as a compute node.\n\n";
    std::cout << "This command will:\n";
    std::cout << "  1. Change to ~/parallax directory\n";
    std::cout << "  2. Activate the Python virtual environment\n";
    std::cout << "  3. Set proxy environment variables (if configured)\n";
    std::cout << "  4. Execute 'parallax join' with your arguments\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  args...       Arguments to pass to parallax join "
                 "(optional)\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -h    Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout
        << "  parallax join                           # Execute: parallax "
           "join\n";
    std::cout << "  parallax join -m Qwen/Qwen3-0.6B       # Execute: parallax "
                 "join -m Qwen/Qwen3-0.6B\n";
    std::cout
        << "  parallax join -s scheduler-addr         # Execute: parallax "
           "join -s scheduler-addr\n\n";
    std::cout << "Note: All arguments will be passed to the built-in "
                 "parallax join script\n";
    std::cout << "      in the Parallax Python virtual environment.\n";
}

std::string ModelJoinCommand::BuildJoinCommand(const CommandContext& context) {
    std::ostringstream command_stream;

    // Built-in execution of parallax join
    command_stream << "parallax join";

    // If there are user parameters, append them
    for (const auto& arg : context.args) {
        command_stream << " " << EscapeForShell(arg);
    }

    return command_stream.str();
}

std::string ModelJoinCommand::EscapeForShell(const std::string& arg) {
    // If parameter contains spaces, special characters, etc., need to wrap with
    // quotes
    if (arg.find(' ') != std::string::npos ||
        arg.find('\t') != std::string::npos ||
        arg.find('\n') != std::string::npos ||
        arg.find('"') != std::string::npos ||
        arg.find('\'') != std::string::npos ||
        arg.find('&') != std::string::npos ||
        arg.find('|') != std::string::npos ||
        arg.find(';') != std::string::npos ||
        arg.find('<') != std::string::npos ||
        arg.find('>') != std::string::npos ||
        arg.find('(') != std::string::npos ||
        arg.find(')') != std::string::npos ||
        arg.find('$') != std::string::npos ||
        arg.find('`') != std::string::npos ||
        arg.find('*') != std::string::npos ||
        arg.find('?') != std::string::npos ||
        arg.find('[') != std::string::npos ||
        arg.find(']') != std::string::npos ||
        arg.find('{') != std::string::npos ||
        arg.find('}') != std::string::npos) {
        // Use single quotes to wrap and escape internal single quotes
        std::string escaped = "'";
        for (char c : arg) {
            if (c == '\'') {
                escaped += "'\"'\"'";  // End single quote, add escaped single
                                       // quote, restart single quote
            } else {
                escaped += c;
            }
        }
        escaped += "'";
        return escaped;
    }

    // If no special characters, return directly
    return arg;
}

}  // namespace commands
}  // namespace parallax