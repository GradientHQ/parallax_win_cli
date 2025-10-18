#pragma once
#include <string>
#include <vector>

// Simplified utils, specifically for parallax project

namespace parallax {
namespace utils {

// Administrator privilege check
bool IsAdmin();

// Path operations
std::string JoinPath(const std::string& dir, const std::string& filename);
std::string GetAppBinDir();
std::string GetCurrentExePath();

// String conversion utilities
std::string UnicodeToAnsi(const std::wstring& wstr);
std::wstring AnsiToUnicode(const std::string& str);
std::string UnicodeToUtf8(const std::wstring& wstr);
std::wstring Utf8ToUnicode(const std::string& str);
std::string AnsiToUtf8(const std::string& str);
std::string Utf8ToAnsi(const std::string& str);

// Time utilities
uint64_t GetTickCountMs();

// PowerShell and WSL output encoding conversion functions
std::string ConvertPowerShellOutputToUtf8(const std::string& powershell_output);
std::string ConvertWslOutputToUtf8(const std::string& wsl_output,
                                   bool is_stderr = false);

// UTF-16 LE encoding conversion helper functions
std::wstring ConvertUtf16LeToWString(const std::string& utf16_bytes);
size_t FindUtf8StartPosition(const std::string& mixed_output);

// String processing utilities
std::string TrimNewlines(const std::string& str);

// File operations
int64_t GetFileSize(const char* path);

// Proxy related functions
std::string GetProxyUrl();

// WSL command building utility functions
std::string BuildWSLCommand(const std::string& ubuntu_version,
                            const std::string& command);
std::string BuildWSLDirectCommand(const std::string& ubuntu_version,
                                  const std::string& command);
std::string GetWSLCommandPrefix(const std::string& ubuntu_version);

// HTTP download functionality
bool DownloadFile(const std::string& url, const std::string& local_path);

// GPU detection related structures and functions
struct GPUInfo {
    std::string name;
    bool is_nvidia;
    bool is_blackwell_series;  // RTX50xx, Bxxx series
};

struct CUDAInfo {
    std::string version;
    bool is_valid_version;  // Whether it's 12.8x or 12.9x version
    std::string driver_version;
};

// Get NVIDIA GPU information
GPUInfo GetNvidiaGPUInfo();

// Get CUDA toolkit version information
CUDAInfo GetCUDAInfo();
}  // namespace utils
}  // namespace parallax
