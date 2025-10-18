#pragma once
#include <string>
#include <map>
#include <mutex>

namespace parallax {
namespace config {

// Configuration item key name constants
extern const std::string KEY_PROXY_URL;
extern const std::string KEY_WSL_LINUX_DISTRO;
extern const std::string KEY_WSL_INSTALLER_URL;
extern const std::string KEY_WSL_KERNEL_URL;
extern const std::string KEY_PARALLAX_GIT_REPO_URL;

// Configuration file manager class
class ConfigManager {
 public:
    // Get singleton instance
    static ConfigManager& GetInstance();

    // Load configuration file (mainly for reloading or loading configuration
    // file from specified path)
    bool LoadConfig(const std::string& config_path = "");

    // Save configuration file
    bool SaveConfig(const std::string& config_path = "");

    // Get configuration item value, return default value if not exists
    std::string GetConfigValue(const std::string& key,
                               const std::string& default_value = "") const;

    // Set configuration item value
    void SetConfigValue(const std::string& key, const std::string& value);

    // Check if configuration item exists
    bool HasConfigValue(const std::string& key) const;

    // Check if it's a valid configuration key
    bool IsValidConfigKey(const std::string& key) const;

    // Get current loaded configuration file path
    std::string GetConfigPath() const;

    // Default configuration file path
    static const std::string DEFAULT_CONFIG_PATH;

    // Reset configuration to default values
    void ResetToDefaults();

    // Get all configuration items (for list command)
    std::map<std::string, std::string> GetAllConfigValues() const;

 private:
    ConfigManager();
    ~ConfigManager();

    // Prohibit copying and assignment
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Initialize default configuration (private method)
    void InitDefaultConfig();

    // Internal configuration file loading method (does not clear existing
    // configuration)
    bool LoadConfigInternal(const std::string& config_path);

    // Parse key-value pairs from string
    bool ParseKeyValue(const std::string& line, std::string& key,
                       std::string& value);

    // Escape special characters
    std::string EscapeValue(const std::string& value);

    // Configuration item storage
    std::map<std::string, std::string> config_values_;

    // Current configuration file path
    std::string config_path_;

    // Thread safety lock
    mutable std::recursive_mutex mutex_;
};

}  // namespace config
}  // namespace parallax