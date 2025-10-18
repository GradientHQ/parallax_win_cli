#include "check_command.h"
#include "environment/environment_installer.h"
#include "tinylog/tinylog.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace parallax {
namespace commands {

CheckCommand::CheckCommand() = default;
CheckCommand::~CheckCommand() = default;

CommandResult CheckCommand::ValidateArgsImpl(CommandContext& context) {
    // check command does not accept additional parameters
    for (const auto& arg : context.args) {
        if (arg != "--help" && arg != "-h") {
            this->ShowError("Unknown parameter: " + arg);
            this->ShowError(
                "The 'check' command does not accept additional parameters.");
            this->ShowError("Usage: parallax check [--help|-h]");
            return CommandResult::InvalidArgs;
        }
    }
    return CommandResult::Success;
}

CommandResult CheckCommand::ExecuteImpl(const CommandContext& context) {
    std::cout << "Parallax Environment Check\n";
    std::cout << "=========================\n\n";

    // Administrator privileges already checked by base class
    this->ShowInfo("Running with Administrator privileges OK");

    // Execute environment check
    int result = CheckAllComponents();

    if (result == 0) {
        // Success information already displayed in CheckAllComponents(), no
        // need to repeat here
        std::cout << "\nNext steps:\n";
        std::cout
            << "  1. You can now run Parallax distributed inference tasks:\n";
        std::cout << "     parallax run\n";
        std::cout
            << "  2. Use 'parallax --help' to see all available commands\n";
        return CommandResult::Success;
    } else if (result == 2) {
        // Restart information already displayed in CheckAllComponents(), no
        // need to repeat here
        return CommandResult::EnvironmentError;  // Reboot required, return
                                                 // environment error
    } else if (result == 3) {
        // Warning information already displayed in CheckAllComponents(), no
        // need to repeat here
        std::cout << "\nNext steps:\n";
        std::cout
            << "  1. You can run Parallax tasks (environment is ready):\n";
        std::cout << "     parallax run\n";
        std::cout << "  2. Consider running 'parallax install' to update "
                     "components\n";
        std::cout
            << "  3. Use 'parallax --help' to see all available commands\n";
        return CommandResult::Success;  // Warnings don't count as errors, still
                                        // return success
    } else {
        // Error messages already displayed in CheckAllComponents(), no need to
        // repeat here
        return CommandResult::EnvironmentError;
    }
}

void CheckCommand::ShowHelpImpl() {
    std::cout << "Usage: parallax check [options]\n\n";
    std::cout
        << "Check Parallax distributed inference framework environment.\n\n";
    std::cout << "This command verifies all required components:\n";
    std::cout << "  1. System requirements (OS version, NVIDIA GPU & driver)\n";
    std::cout << "  2. Windows Subsystem for Linux 2 (WSL2) and Virtual "
                 "Machine Platform\n";
    std::cout << "  3. WSL package, kernel and Ubuntu distribution\n";
    std::cout << "  4. CUDA Toolkit 12.8\n";
    std::cout << "  5. Development tools (Rust Cargo, Ninja build system)\n";
    std::cout << "  6. Python pip upgrade\n";
    std::cout << "  7. Parallax distributed inference framework\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -h      Show this help message\n\n";
    std::cout << "Exit codes:\n";
    std::cout << "  0    All checks passed (including warnings)\n";
    std::cout << "  1    Invalid arguments\n";
    std::cout << "  2    Environment issues found or reboot required\n\n";
    std::cout << "Examples:\n";
    std::cout << "  parallax check             Run environment check\n";
    std::cout << "  parallax check --help      Show this help message\n";
}

int CheckCommand::CheckAllComponents() {
    environment::EnvironmentInstaller installer;

    // Define component check callback function to display check results in
    // real-time
    auto component_callback = [](const environment::ComponentResult&
                                     comp_result) {
        std::string component_name =
            environment::ComponentToString(comp_result.component);

        // Set status display symbol
        std::string status_symbol;
        switch (comp_result.status) {
            case environment::InstallationStatus::kSuccess:
                status_symbol = "[OK]";
                break;
            case environment::InstallationStatus::kSkipped:
                status_symbol = "[OK]";
                break;
            case environment::InstallationStatus::kFailed:
                status_symbol = "[FAIL]";
                break;
            case environment::InstallationStatus::kInProgress:
                status_symbol = "[...]";
                break;
            case environment::InstallationStatus::kWarning:
                status_symbol = "[WARN]";
                break;
        }

        // Output check results in real-time
        std::cout << status_symbol << " " << std::left << std::setw(25)
                  << component_name;

        if (comp_result.status == environment::InstallationStatus::kFailed) {
            std::cout << " FAILED";
        } else if (comp_result.status ==
                   environment::InstallationStatus::kSkipped) {
            std::cout << " OK (Already installed)";
        } else if (comp_result.status ==
                   environment::InstallationStatus::kWarning) {
            std::cout << " WARNING";
        } else {
            std::cout << " OK";
        }

        // Display detailed messages (failure and warning status)
        if (!comp_result.message.empty() &&
            (comp_result.status == environment::InstallationStatus::kFailed ||
             comp_result.status == environment::InstallationStatus::kWarning)) {
            std::cout << "\n   " << comp_result.message;
        }

        std::cout << std::endl;

        // Add appropriate delay to improve user experience
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    };

    // Execute environment check using callback mechanism
    auto result = installer.CheckEnvironment(component_callback);

    DisplayResults(result);

    // Return appropriate exit code based on check results
    bool has_failures = false;
    bool has_warnings = false;
    for (const auto& comp_result : result.component_results) {
        if (comp_result.status == environment::InstallationStatus::kFailed) {
            has_failures = true;
            break;
        } else if (comp_result.status ==
                   environment::InstallationStatus::kWarning) {
            has_warnings = true;
        }
    }

    if (result.reboot_required) {
        std::cout << "\n[WARNING] SYSTEM REBOOT REQUIRED\n";
        std::cout
            << "Some components require a system restart to take effect.\n";
        std::cout << "Please restart your computer before proceeding.\n";
        return 2;  // Special exit code indicating reboot required
    }

    if (has_failures) {
        std::cout << "\n[ERROR] Some environment requirements are not met.\n";
        std::cout << "Run 'parallax install' to install missing components.\n";
        return 1;
    } else if (has_warnings) {
        std::cout << "\n[WARNING] Environment is ready but some components "
                     "have updates available.\n";
        std::cout
            << "Consider running 'parallax install' to update components.\n";
        return 3;  // Special exit code indicating warnings
    } else {
        std::cout
            << "\n[SUCCESS] All environment requirements are satisfied!\n";
        std::cout << "Your system is ready to run Parallax.\n";
        return 0;
    }
}

void CheckCommand::DisplayResults(
    const parallax::environment::EnvironmentResult& result) {
    std::cout << "\nEnvironment Check Summary:\n";
    std::cout << "-------------------------\n";

    if (!result.overall_message.empty()) {
        std::cout << "Overall Status: " << result.overall_message << std::endl;
    }
}

}  // namespace commands
}  // namespace parallax
