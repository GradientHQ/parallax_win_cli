#include "software_installer.h"
#include "environment_installer.h"
#include "config/config_manager.h"
#include "utils/wsl_process.h"
#include "utils/utils.h"
#include "utils/process.h"
#include "tinylog/tinylog.h"
#include <algorithm>

namespace parallax {
namespace environment {

// PipUpgradeManager implementation
PipUpgradeManager::PipUpgradeManager(std::shared_ptr<ExecutionContext> context,
                                     std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult PipUpgradeManager::Check() {
    LogOperationStart("Checking");

    bool pip_available = IsPipUpToDate();

    ComponentResult result =
        pip_available ? CreateSkippedResult("pip is available")
                      : CreateFailureResult("pip is not installed", 24);

    LogOperationResult("Checking", result);
    return result;
}

ComponentResult PipUpgradeManager::Install() {
    LogOperationStart("Upgrading");

    const std::string& proxy_url = context_->GetProxyUrl();

    // Check if pip is installed
    if (!IsPipUpToDate()) {
        info_log("[ENV] Installing python3-pip in WSL...");

        // First install python3-pip
        std::string install_pip_cmd =
            proxy_url.empty()
                ? "apt-get install -y python3-pip"
                : "apt-get -o Acquire::http::proxy=\"" + proxy_url +
                      "\" -o Acquire::https::proxy=\"" + proxy_url +
                      "\" install -y python3-pip";

        auto [install_code, install_output] =
            executor_->ExecuteWSL(install_pip_cmd, 300);
        if (install_code != 0) {
            ComponentResult result = CreateFailureResult(
                "Failed to install python3-pip: " + install_output, 24);
            LogOperationResult("Upgrading", result);
            return result;
        }
    }

    info_log("[ENV] Upgrading pip in WSL...");

    // Upgrade pip
    std::string upgrade_cmd =
        "pip install --upgrade pip --break-system-packages --ignore-installed";
    if (!proxy_url.empty()) {
        upgrade_cmd =
            "pip install --proxy " + proxy_url +
            " --upgrade pip --break-system-packages --ignore-installed";
    }

    auto [upgrade_code, upgrade_output] =
        executor_->ExecuteWSL(upgrade_cmd, 300);

    ComponentResult result =
        (upgrade_code != 0)
            ? CreateFailureResult("Failed to upgrade pip: " + upgrade_output,
                                  24)
            : CreateSuccessResult("pip installed and upgraded successfully");

    LogOperationResult("Upgrading", result);
    return result;
}

bool PipUpgradeManager::IsPipUpToDate() {
    // Check if pip is installed and up to date
    auto [pip_code, pip_output] = executor_->ExecuteWSL("pip --version");
    return (pip_code == 0 && !pip_output.empty());
}

EnvironmentComponent PipUpgradeManager::GetComponentType() const {
    return EnvironmentComponent::kPipUpgrade;
}

std::string PipUpgradeManager::GetComponentName() const {
    return "pip Upgrade";
}

// ParallaxProjectInstaller implementation
ParallaxProjectInstaller::ParallaxProjectInstaller(
    std::shared_ptr<ExecutionContext> context,
    std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult ParallaxProjectInstaller::Check() {
    LogOperationStart("Checking");

    // Check if Parallax project is installed
    bool is_installed = IsParallaxProjectInstalled();

    if (is_installed) {
        // Check if there are git updates
        if (HasParallaxProjectGitUpdates()) {
            ComponentResult result = CreateWarningResult(
                "Parallax project is installed but has git updates available");
            LogOperationResult("Checking", result);
            return result;
        } else {
            ComponentResult result = CreateSkippedResult(
                "Parallax project is already installed and up to date");
            LogOperationResult("Checking", result);
            return result;
        }
    }

    ComponentResult result =
        CreateFailureResult("Parallax project is not installed", 25);
    LogOperationResult("Checking", result);
    return result;
}

ComponentResult ParallaxProjectInstaller::Install() {
    LogOperationStart("Installing");

    // Check if Parallax project is installed
    bool is_installed = IsParallaxProjectInstalled();

    // Handle install command
    if (is_installed) {
        // Project is installed, check if there are updates
        if (!HasParallaxProjectGitUpdates()) {
            ComponentResult result = CreateSkippedResult(
                "Parallax project is already installed and up to date");
            LogOperationResult("Installing", result);
            return result;
        }
        // Has updates, continue with update process
        info_log("[ENV] Parallax project has updates available, updating...");
    } else {
        // Project not installed, perform full installation
        info_log("[ENV] Installing Parallax project in WSL...");
    }

    // Get configured git repository URL
    std::string repo_url =
        parallax::config::ConfigManager::GetInstance().GetConfigValue(
            parallax::config::KEY_PARALLAX_GIT_REPO_URL);

    const std::string& proxy_url = context_->GetProxyUrl();

    // Install command sequence (step name, command, timeout seconds, use
    // real-time output)
    std::vector<std::tuple<std::string, std::string, int, bool>> commands;

    // Determine if it's update mode or fresh installation mode
    bool is_update_mode = (is_installed);

    if (is_update_mode) {
        // Project is installed and has updates, only execute git pull update
        std::string pull_cmd = "cd ~/parallax && git pull";
        if (!proxy_url.empty()) {
            pull_cmd = "cd ~/parallax && ALL_PROXY=" + proxy_url + " git pull";
        }
        commands.emplace_back("update_parallax", pull_cmd, 300, false);
    } else {
        // Project not installed, check if parallax directory exists, decide
        // whether to clone or pull
        auto [check_dir_code, check_dir_output] = executor_->ExecuteWSL(
            "ls -la ~/parallax/.git 2>/dev/null || echo 'not found'", 30);

        if (check_dir_code == 0 &&
            check_dir_output.find("not found") == std::string::npos) {
            // Directory exists, check if it's a git repository
            auto [check_git_code, check_git_output] = executor_->ExecuteWSL(
                "cd ~/parallax && git branch 2>/dev/null || echo 'not git'",
                30);

            if (check_git_code == 0 &&
                check_git_output.find("not git") == std::string::npos) {
                // Is a git repository, execute pull
                info_log(
                    "[ENV] Parallax directory exists, updating with git "
                    "pull...");
                std::string pull_cmd = "cd ~/parallax && git pull";
                if (!proxy_url.empty()) {
                    pull_cmd =
                        "cd ~/parallax && ALL_PROXY=" + proxy_url + " git pull";
                }
                commands.emplace_back("update_parallax", pull_cmd, 300, false);
            } else {
                // Not a git repository, remove directory and clone again
                info_log(
                    "[ENV] Parallax directory exists but is not a git "
                    "repository, "
                    "removing and cloning...");
                commands.emplace_back("remove_old_parallax",
                                      "rm -rf ~/parallax", 60, false);

                std::string clone_cmd = "cd ~ && ";
                if (!proxy_url.empty()) {
                    clone_cmd += "ALL_PROXY=" + proxy_url + " ";
                }
                clone_cmd += "git clone " + repo_url;
                commands.emplace_back("clone_parallax", clone_cmd, 600, false);
            }
        } else {
            // Directory does not exist, execute clone
            info_log("[ENV] Parallax directory not found, cloning...");

            std::string clone_cmd = "cd ~ && ";
            if (!proxy_url.empty()) {
                clone_cmd += "ALL_PROXY=" + proxy_url + " ";
            }
            clone_cmd += "git clone " + repo_url;
            commands.emplace_back("clone_parallax", clone_cmd, 600, false);
        }
    }

    if (!is_update_mode) {
        // Only install python3-venv dependency package during first
        // installation
        std::string install_venv_cmd =
            "apt update && apt-get install -y python3-venv";
        if (!proxy_url.empty()) {
            install_venv_cmd =
                "apt -o Acquire::http::proxy=\"" + proxy_url +
                "\" -o Acquire::https::proxy=\"" + proxy_url +
                "\" update && apt-get -o Acquire::http::proxy=\"" + proxy_url +
                "\" -o Acquire::https::proxy=\"" + proxy_url +
                "\" install -y python3-venv";
        }
        commands.emplace_back("install_python3_venv", install_venv_cmd, 300,
                              false);
    }

    // Install Parallax project (use real-time output)
    std::string install_base_cmd =
        "cd ~/parallax && ([ -d ./venv ] || python3 -m venv ./venv) && source "
        "./venv/bin/activate "
        "&& pip install -e '.[gpu]'";
    if (!proxy_url.empty()) {
        install_base_cmd =
            "cd ~/parallax && ([ -d ./venv ] || python3 -m venv ./venv) && "
            "source "
            "./venv/bin/activate && HTTP_PROXY=\"" +
            proxy_url + "\" HTTPS_PROXY=\"" + proxy_url +
            "\" pip install -e '.[gpu]'";
    }
    commands.emplace_back("install_parallax_base", install_base_cmd, 1800,
                          true);

    if (!is_update_mode) {
        // Only install sgl_kernel during first installation (use real-time
        // output)
        std::string install_sgl_cmd =
            "cd ~/parallax && source ./venv/bin/activate && pip install "
            "https://github.com/sgl-project/whl/releases/download/v0.3.7/"
            "sgl_kernel-0.3.7+cu128-cp310-abi3-manylinux2014_x86_64.whl "
            "--force-reinstall";
        if (!proxy_url.empty()) {
            install_sgl_cmd =
                "cd ~/parallax && source ./venv/bin/activate && HTTP_PROXY=\"" +
                proxy_url + "\" HTTPS_PROXY=\"" + proxy_url +
                "\" pip install "
                " https://github.com/sgl-project/whl/releases/download/v0.3.7/"
                "sgl_kernel-0.3.7+cu128-cp310-abi3-manylinux2014_x86_64.whl "
                "--force-reinstall";
        }
        commands.emplace_back("install_sgl_kernel", install_sgl_cmd, 600, true);
    }

    // Execute command sequence
    ComponentResult cmd_result =
        ExecuteCommandSequence(commands, "Parallax project installation");
    if (cmd_result.status != InstallationStatus::kSuccess) {
        LogOperationResult("Installing", cmd_result);
        return cmd_result;
    }

    // Verify installation
    ComponentResult result =
        IsParallaxProjectInstalled()
            ? (is_update_mode ? CreateSuccessResult(
                                    "Parallax project updated successfully")
                              : CreateSuccessResult(
                                    "Parallax project installed successfully"))
            : CreateFailureResult(is_update_mode
                                      ? "Parallax project update completed but "
                                        "verification failed"
                                      : "Parallax project installation "
                                        "completed but verification failed",
                                  25);

    LogOperationResult("Installing", result);
    return result;
}

ComponentResult ParallaxProjectInstaller::ExecuteCommandSequence(
    const std::vector<std::tuple<std::string, std::string, int, bool>>&
        commands,
    const std::string& operation_name) {
    // Execute all commands
    for (const auto& [step_name, cmd, timeout, use_realtime] : commands) {
        info_log("[ENV] %s step: %s", operation_name.c_str(),
                 step_name.c_str());

        int cmd_exit_code = 0;
        if (use_realtime) {
            // Use WSLProcess to get real-time output
            std::string wsl_cmd = parallax::utils::BuildWSLCommand(
                context_->GetUbuntuVersion(), cmd);
            WSLProcess wsl_process;
            cmd_exit_code = wsl_process.Execute(wsl_cmd);
        } else {
            // Use regular execution method
            auto [exit_code, output] = executor_->ExecuteWSL(cmd, timeout);
            cmd_exit_code = exit_code;
        }

        if (cmd_exit_code != 0) {
            std::string error_msg =
                "Failed at step '" + step_name + "': " + cmd;
            return CreateFailureResult(error_msg, 25);
        }
    }

    return CreateSuccessResult("Command sequence completed successfully");
}

bool ParallaxProjectInstaller::IsParallaxProjectInstalled() {
    // Check if parallax project is installed (need to check in virtual
    // environment)
    auto [check_code, check_output] = executor_->ExecuteWSL(
        "cd ~/parallax && [ -d ./venv ] && source ./venv/bin/activate && pip "
        "list | grep parallax");
    return (check_code == 0 && !check_output.empty());
}

bool ParallaxProjectInstaller::HasParallaxProjectGitUpdates() {
    const std::string& proxy_url = context_->GetProxyUrl();

    // Check if Parallax project has git updates
    // First check if git repository exists
    auto [check_git_code, check_git_output] = executor_->ExecuteWSL(
        "cd ~/parallax && git rev-parse --is-inside-work-tree 2>/dev/null", 30);

    if (check_git_code != 0) {
        // Not a git repository or directory does not exist
        return false;
    }

    // Get remote update information
    std::string fetch_cmd = "cd ~/parallax && git fetch origin";
    if (!proxy_url.empty()) {
        fetch_cmd =
            "cd ~/parallax && ALL_PROXY=" + proxy_url + " git fetch origin";
    }

    auto [fetch_code, fetch_output] = executor_->ExecuteWSL(fetch_cmd, 60);
    if (fetch_code != 0) {
        // Failed to get remote information, possibly network issue
        return false;
    }

    // Check if there are differences between local and remote
    auto [diff_code, diff_output] = executor_->ExecuteWSL(
        "cd ~/parallax && git rev-list HEAD...origin/main --count 2>/dev/null",
        30);

    if (diff_code == 0 && !diff_output.empty()) {
        try {
            // Remove possible whitespace characters
            std::string trimmed_output = diff_output;
            trimmed_output.erase(
                std::remove_if(trimmed_output.begin(), trimmed_output.end(),
                               ::isspace),
                trimmed_output.end());

            if (!trimmed_output.empty()) {
                int update_count = std::stoi(trimmed_output);
                return update_count > 0;
            }
        } catch (const std::exception&) {
            // Conversion failed, possibly incorrect output format
            return false;
        }
    }

    return false;
}

EnvironmentComponent ParallaxProjectInstaller::GetComponentType() const {
    return EnvironmentComponent::kParallaxProject;
}

std::string ParallaxProjectInstaller::GetComponentName() const {
    return "Parallax Project";
}

}  // namespace environment
}  // namespace parallax
