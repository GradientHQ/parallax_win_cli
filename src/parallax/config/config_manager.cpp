#include "config_manager.h"
#include "../tinylog/tinylog.h"
#include "../utils/utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>

namespace parallax {
namespace config {

// Configuration item key name constants
const std::string KEY_PROXY_URL = "proxy_url";
const std::string KEY_WSL_LINUX_DISTRO = "wsl_linux_distro";
const std::string KEY_WSL_INSTALLER_URL = "wsl_installer_url";
const std::string KEY_WSL_KERNEL_URL = "wsl_kernel_url";
const std::string KEY_PARALLAX_GIT_REPO_URL = "parallax_git_repo_url";

// Default configuration file name
const std::string ConfigManager::DEFAULT_CONFIG_PATH = "parallax_config.txt";

// Singleton instance getter
ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

// Constructor
ConfigManager::ConfigManager() {
    // Use configuration file in the same directory as exe
    std::string exe_dir = parallax::utils::GetAppBinDir();
    config_path_ = parallax::utils::JoinPath(exe_dir, DEFAULT_CONFIG_PATH);

    // Initialize default configuration
    InitDefaultConfig();

    // Try to load configuration file (will automatically create default config
    // file if it doesn't exist)
    LoadConfigInternal(config_path_);
}

// Destructor
ConfigManager::~ConfigManager() { SaveConfig(); }

// Initialize default configuration
void ConfigManager::InitDefaultConfig() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    config_values_[KEY_WSL_LINUX_DISTRO] = "Ubuntu-24.04";
    config_values_[KEY_WSL_INSTALLER_URL] =
        "https://github.com/microsoft/WSL/releases/download/2.4.13/"
        "wsl.2.4.13.0.x64.msi";
    config_values_[KEY_WSL_KERNEL_URL] =
        "https://wslstorestorage.blob.core.windows.net/wslblob/"
        "wsl_update_x64.msi";
    config_values_[KEY_PARALLAX_GIT_REPO_URL] =
        "https://github.com/GradientHQ/parallax.git";
    // proxy_url has no default value
}

// Load configuration file
bool ConfigManager::LoadConfig(const std::string& config_path) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::string path_to_load = config_path.empty() ? config_path_ : config_path;

    // Clear existing configuration and reinitialize default configuration
    config_values_.clear();
    InitDefaultConfig();

    // Load configuration file
    bool result = LoadConfigInternal(path_to_load);

    // Update current configuration file path
    if (!config_path.empty()) {
        config_path_ = config_path;
    }

    return result;
}

// Internal configuration file loading method (does not clear existing
// configuration)
bool ConfigManager::LoadConfigInternal(const std::string& config_path) {
    // Check if file exists
    std::ifstream file(config_path);
    if (!file.is_open()) {
        // If file doesn't exist, create default configuration file
        info_log("Config file not found, creating default config: %s",
                 config_path.c_str());
        return SaveConfig(config_path);
    }

    // Save default values of built-in configuration items for protecting
    // built-in configuration items
    std::map<std::string, std::string> builtin_defaults = {
        {KEY_WSL_LINUX_DISTRO, config_values_[KEY_WSL_LINUX_DISTRO]},
        {KEY_WSL_INSTALLER_URL, config_values_[KEY_WSL_INSTALLER_URL]},
        {KEY_WSL_KERNEL_URL, config_values_[KEY_WSL_KERNEL_URL]},
        {KEY_PARALLAX_GIT_REPO_URL, config_values_[KEY_PARALLAX_GIT_REPO_URL]}};

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comment lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::string key, value;
        if (ParseKeyValue(line, key, value)) {
            config_values_[key] = value;
        }
    }

    file.close();

    // Protect built-in configuration items: if a built-in configuration item in
    // user config file is empty, restore default value
    for (const auto& builtin : builtin_defaults) {
        const std::string& key = builtin.first;
        const std::string& default_value = builtin.second;

        if (config_values_[key].empty()) {
            config_values_[key] = default_value;
            info_log(
                "Protected builtin config key '%s' restored to default value",
                key.c_str());
        }
    }

    info_log("Config loaded successfully from %s", config_path.c_str());
    return true;
}

// Save configuration file
bool ConfigManager::SaveConfig(const std::string& config_path) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::string path_to_save = config_path.empty() ? config_path_ : config_path;

    // Open file
    std::ofstream file(path_to_save);
    if (!file.is_open()) {
        error_log("Failed to open config file for writing: %s",
                  path_to_save.c_str());
        return false;
    }

    // Write configuration header comments
    file << "# Parallax Configuration File\n";
    file << "# Generated automatically, do not edit manually\n\n";

    // Write configuration items sorted by key name
    for (const auto& kv : config_values_) {
        file << kv.first << "=" << EscapeValue(kv.second) << "\n";
    }

    file.close();

    // Update current configuration file path
    if (!config_path.empty()) {
        config_path_ = config_path;
    }

    info_log("Config saved successfully to %s", path_to_save.c_str());
    return true;
}

// Parse key-value pairs
bool ConfigManager::ParseKeyValue(const std::string& line, std::string& key,
                                  std::string& value) {
    size_t pos = line.find('=');
    if (pos == std::string::npos) {
        return false;
    }

    key = line.substr(0, pos);
    value = line.substr(pos + 1);

    // Remove leading and trailing spaces
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);

    // Check escape characters
    size_t i = 0;
    std::string unescaped_value;
    while (i < value.length()) {
        if (value[i] == '\\' && i + 1 < value.length()) {
            char next = value[i + 1];
            switch (next) {
                case 'n':
                    unescaped_value += '\n';
                    break;
                case 'r':
                    unescaped_value += '\r';
                    break;
                case 't':
                    unescaped_value += '\t';
                    break;
                case '\\':
                    unescaped_value += '\\';
                    break;
                case '"':
                    unescaped_value += '"';
                    break;
                case '\'':
                    unescaped_value += '\'';
                    break;
                case '=':
                    unescaped_value += '=';
                    break;
                default:
                    unescaped_value += next;
                    break;
            }
            i += 2;
        } else {
            unescaped_value += value[i];
            i++;
        }
    }

    value = unescaped_value;
    return true;
}

// Escape special characters
std::string ConfigManager::EscapeValue(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        switch (c) {
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\'':
                escaped += "\\\'";
                break;
            case '=':
                escaped += "\\=";
                break;
            default:
                escaped += c;
                break;
        }
    }
    return escaped;
}

// Get configuration item value
std::string ConfigManager::GetConfigValue(
    const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    auto it = config_values_.find(key);
    if (it != config_values_.end()) {
        return it->second;
    }
    return default_value;
}

// Set configuration item value
void ConfigManager::SetConfigValue(const std::string& key,
                                   const std::string& value) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    config_values_[key] = value;
}

// Check if configuration item exists
bool ConfigManager::HasConfigValue(const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return config_values_.find(key) != config_values_.end();
}

// Check if it's a valid configuration key
bool ConfigManager::IsValidConfigKey(const std::string& key) const {
    static const std::set<std::string> valid_keys = {
        KEY_PROXY_URL, KEY_WSL_LINUX_DISTRO, KEY_WSL_INSTALLER_URL,
        KEY_WSL_KERNEL_URL, KEY_PARALLAX_GIT_REPO_URL};

    return valid_keys.find(key) != valid_keys.end();
}

// Get current configuration file path
std::string ConfigManager::GetConfigPath() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return config_path_;
}

// Reset configuration to default values
void ConfigManager::ResetToDefaults() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    config_values_.clear();
    InitDefaultConfig();
    info_log("Configuration reset to default values");
}

// Get all configuration items (for list command)
std::map<std::string, std::string> ConfigManager::GetAllConfigValues() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return config_values_;
}

}  // namespace config
}  // namespace parallax