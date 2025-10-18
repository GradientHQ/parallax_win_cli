#include "install_command.h"
#include "environment/environment_installer.h"
#include "tinylog/tinylog.h"
#include <iostream>
#include <iomanip>

namespace parallax {
namespace commands {

InstallCommand::InstallCommand() = default;
InstallCommand::~InstallCommand() = default;

CommandResult InstallCommand::ValidateArgsImpl(CommandContext& context) {
    // install command does not accept additional parameters
    for (const auto& arg : context.args) {
        if (arg != "--help" && arg != "-h") {
            this->ShowError("Unknown parameter: " + arg);
            this->ShowError(
                "The 'install' command does not accept additional parameters.");
            this->ShowError("Usage: parallax install [--help|-h]");
            return CommandResult::InvalidArgs;
        }
    }
    return CommandResult::Success;
}

CommandResult InstallCommand::ExecuteImpl(const CommandContext& context) {
    std::cout << "Parallax Environment Installation\n";
    std::cout << "================================\n\n";

    // Administrator privileges already checked by base class
    this->ShowInfo("Running with Administrator privileges OK");
    this->ShowInfo(
        "This will install and configure the required components for "
        "Parallax:");
    std::cout << "1. System requirements (OS version, NVIDIA GPU & driver)\n";
    std::cout << "2. Windows Subsystem for Linux 2 (WSL2) and Virtual Machine "
                 "Platform\n";
    std::cout << "3. WSL package, kernel and Ubuntu distribution\n";
    std::cout << "4. CUDA Toolkit 12.8\n";
    std::cout << "5. Development tools (Rust Cargo, Ninja build system)\n";
    std::cout << "6. Python pip upgrade\n";
    std::cout << "7. Parallax distributed inference framework\n\n";

    this->ShowWarning("System reboot may be required during the process.");

    // Execute installation
    int result = InstallAllComponents();

    if (result == 0) {
        this->ShowInfo("Installation completed successfully!");
        return CommandResult::Success;
    } else if (result == 2) {
        this->ShowWarning(
            "Installation completed but system reboot is required.");
        return CommandResult::Success;
    } else {
        this->ShowError(
            "Installation failed. Please check the logs for details.");
        return CommandResult::ExecutionError;
    }
}

void InstallCommand::ShowHelpImpl() {
    std::cout << "Usage: parallax install [options]\n\n";
    std::cout << "Install and configure the Parallax distributed inference "
                 "framework environment.\n\n";
    std::cout << "This command will install and configure:\n";
    std::cout << "  1. Windows Subsystem for Linux 2 (WSL2)\n";
    std::cout << "  2. Virtual Machine Platform\n";
    std::cout << "  3. WSL package and kernel updates\n";
    std::cout << "  4. Ubuntu distribution\n\n";
    std::cout << "Prerequisites:\n";
    std::cout << "  - Windows 10 build 18362+ or Windows 11\n";
    std::cout << "  - Administrator privileges\n";
    std::cout << "  - Internet connection\n";
    std::cout << "  - At least 4GB free disk space\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -h      Show this help message\n\n";
    std::cout << "Exit codes:\n";
    std::cout << "  0    Installation completed successfully\n";
    std::cout << "  1    Invalid arguments\n";
    std::cout << "  3    Installation failed\n\n";
    std::cout << "Examples:\n";
    std::cout << "  parallax install           Install all components\n";
    std::cout << "  parallax install --help    Show this help message\n\n";
    std::cout << "Note: This process may require multiple reboots and can take "
                 "15-30 minutes.\n";
}

int InstallCommand::InstallAllComponents() {
    environment::EnvironmentInstaller installer;

    auto result = installer.InstallEnvironment(ProgressCallback);

    DisplayResults(result);

    // Return appropriate exit code based on installation results
    bool all_success = true;
    for (const auto& comp_result : result.component_results) {
        if (comp_result.status == environment::InstallationStatus::kFailed) {
            all_success = false;
            break;
        }
    }

    if (result.reboot_required) {
        std::cout << "\n[REBOOT] SYSTEM REBOOT REQUIRED\n";
        std::cout << "Some components have been installed but require a system "
                     "restart to take effect.\n";
        std::cout << "Please restart your computer and run 'parallax install' "
                     "again to continue.\n";
        return 2;  // Special exit code indicating reboot required
    }

    if (all_success) {
        std::cout << "\n[SUCCESS] All environment components installed "
                     "successfully!\n";
        std::cout << "Your system is now ready to run Parallax.\n";
        std::cout << "\nNext steps:\n";
        std::cout << "  1. Run 'parallax check' to verify the installation\n";
        std::cout << "  2. Test Parallax model server:\n";
        std::cout << "     parallax run\n";
        return 0;
    } else {
        std::cout << "\n[ERROR] Some components failed to install.\n";
        std::cout << "Please check the error messages above and try again.\n";
        std::cout << "You may need to:\n";
        std::cout << "  1. Run as Administrator\n";
        std::cout << "  2. Check your internet connection\n";
        std::cout << "  3. Enable virtualization in BIOS\n";
        return 1;
    }
}

void InstallCommand::DisplayResults(
    const parallax::environment::EnvironmentResult& result) {
    std::cout << "\nInstallation Results:\n";
    std::cout << "--------------------\n\n";

    for (const auto& comp_result : result.component_results) {
        std::string component_name =
            environment::ComponentToString(comp_result.component);

        // Set status display symbol
        std::string status_symbol;
        std::string status_text;
        switch (comp_result.status) {
            case environment::InstallationStatus::kSuccess:
                status_symbol = "[OK]";
                status_text = "INSTALLED";
                break;
            case environment::InstallationStatus::kSkipped:
                status_symbol = "[OK]";
                status_text = "ALREADY INSTALLED";
                break;
            case environment::InstallationStatus::kFailed:
                status_symbol = "[FAIL]";
                status_text = "FAILED";
                break;
            case environment::InstallationStatus::kInProgress:
                status_symbol = "[PROGRESS]";
                status_text = "IN PROGRESS";
                break;
        }

        // Format output
        std::cout << status_symbol << " " << std::left << std::setw(25)
                  << component_name;
        std::cout << " " << status_text;

        if (!comp_result.message.empty()) {
            std::cout << "\n   " << comp_result.message;
        }

        std::cout << std::endl;
    }

    if (!result.overall_message.empty()) {
        std::cout << "\nOverall Status: " << result.overall_message
                  << std::endl;
    }
}

void InstallCommand::ProgressCallback(const std::string& step,
                                      const std::string& message,
                                      int progress_percent) {
    static int last_progress = -1;

    if (progress_percent != last_progress) {
        std::cout << "\r[" << std::setw(3) << progress_percent << "%] "
                  << message << std::flush;
        last_progress = progress_percent;

        if (progress_percent == 100) {
            std::cout << std::endl;
        }
    }
}

}  // namespace commands
}  // namespace parallax
