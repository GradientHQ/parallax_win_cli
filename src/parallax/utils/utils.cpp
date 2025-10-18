#include "utils.h"
#include "process.h"
#include "../config/config_manager.h"
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

#pragma comment(lib, "advapi32.lib")

namespace parallax {
namespace utils {

std::string JoinPath(const std::string& dir, const std::string& filename) {
    if (dir.back() == '\\') {
        return dir + filename;
    } else {
        return dir + '\\' + filename;
    }
}

std::string GetAppBinDir() {
    static std::string bin_path;
    if (!bin_path.empty()) {
        return bin_path;
    }

    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));

    if (GetModuleFileNameA(nullptr, tmp, sizeof(tmp) / sizeof(tmp[0])) > 0) {
        char* dst = nullptr;
        char* p = tmp;
        while (*p) {
            if (*p == '\\') {
                dst = p;
            }
            p = CharNextA(p);
        }
        if (dst) {
            *dst = 0;
        }
    }

    bin_path = tmp;
    return bin_path;
}

std::string GetCurrentExePath() {
    static std::string exe_path;
    if (!exe_path.empty()) {
        return exe_path;
    }

    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));

    if (GetModuleFileNameA(nullptr, tmp, sizeof(tmp) / sizeof(tmp[0])) > 0) {
        exe_path = tmp;
    }

    return exe_path;
}

bool IsAdmin() {
    HANDLE hAccessToken = nullptr;
    PSID psidAdministrators = nullptr;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL bIsAdmin = FALSE;
    bool hasAdminPrivileges = false;
    DWORD dwSize;  // Declared at the beginning of the function

    do {
        // Open the current process token
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY,
                              &hAccessToken)) {
            break;
        }

        // Method 1: Check if belongs to administrator group
        if (AllocateAndInitializeSid(&siaNtAuthority, 2,
                                     SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                     &psidAdministrators)) {
            if (CheckTokenMembership(hAccessToken, psidAdministrators,
                                     &bIsAdmin)) {
                if (bIsAdmin) {
                    hasAdminPrivileges = true;
                }
            }
        }

        // Method 2: Check token elevation status (UAC related)
        if (!hasAdminPrivileges) {
            TOKEN_ELEVATION elevation;
            if (GetTokenInformation(hAccessToken, TokenElevation, &elevation,
                                    sizeof(elevation), &dwSize)) {
                if (elevation.TokenIsElevated) {
                    hasAdminPrivileges = true;
                }
            }
        }

        // Method 3: Try to check specific administrator privileges
        if (!hasAdminPrivileges) {
            TOKEN_ELEVATION_TYPE elevationType;
            if (GetTokenInformation(hAccessToken, TokenElevationType,
                                    &elevationType, sizeof(elevationType),
                                    &dwSize)) {
                // TokenElevationTypeFull indicates fully elevated administrator
                // privileges TokenElevationTypeDefault indicates default
                // privileges (for built-in administrator account)
                if (elevationType == TokenElevationTypeFull ||
                    elevationType == TokenElevationTypeDefault) {
                    hasAdminPrivileges = true;
                }
            }
        }

        // Method 4: Final attempt at actual privilege test - try to adjust
        // process privileges
        if (!hasAdminPrivileges) {
            HANDLE hToken = nullptr;
            if (OpenProcessToken(GetCurrentProcess(),
                                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                 &hToken)) {
                TOKEN_PRIVILEGES tkp;
                if (LookupPrivilegeValue(nullptr, SE_DEBUG_NAME,
                                         &tkp.Privileges[0].Luid)) {
                    tkp.PrivilegeCount = 1;
                    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                    // Try to enable debug privilege (requires administrator
                    // privileges)
                    if (AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr,
                                              nullptr)) {
                        DWORD dwError = GetLastError();
                        if (dwError == ERROR_SUCCESS) {
                            hasAdminPrivileges = true;
                        }
                    }
                }
                CloseHandle(hToken);
            }
        }

    } while (false);

    if (psidAdministrators) {
        FreeSid(psidAdministrators);
    }
    if (hAccessToken) {
        CloseHandle(hAccessToken);
    }

    return hasAdminPrivileges;
}

// ANSI <-> Unicode conversion
std::string UnicodeToAnsi(const std::wstring& wstr) {
    if (wstr.empty()) return "";

    int size_needed =
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";

    std::vector<char> buffer(size_needed);
    int result = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, buffer.data(),
                                     size_needed, NULL, NULL);
    if (result <= 0) return "";

    return std::string(buffer.data());
}

std::wstring AnsiToUnicode(const std::string& str) {
    if (str.empty()) return L"";

    int size_needed = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    if (size_needed <= 0) return L"";

    std::vector<wchar_t> buffer(size_needed);
    int result = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer.data(),
                                     size_needed);
    if (result <= 0) return L"";

    return std::wstring(buffer.data());
}

std::string UnicodeToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";

    int size_needed =
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";

    std::vector<char> buffer(size_needed);
    int result = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
                                     buffer.data(), size_needed, NULL, NULL);
    if (result <= 0) return "";

    return std::string(buffer.data());
}

std::wstring Utf8ToUnicode(const std::string& str) {
    if (str.empty()) return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size_needed <= 0) return L"";

    std::vector<wchar_t> buffer(size_needed);
    int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.data(),
                                     size_needed);
    if (result <= 0) return L"";

    return std::wstring(buffer.data());
}

std::string AnsiToUtf8(const std::string& str) {
    return UnicodeToUtf8(AnsiToUnicode(str));
}

std::string Utf8ToAnsi(const std::string& str) {
    return UnicodeToAnsi(Utf8ToUnicode(str));
}

uint64_t GetTickCountMs() { return GetTickCount64(); }

// PowerShell output encoding conversion function
std::string ConvertPowerShellOutputToUtf8(
    const std::string& powershell_output) {
    if (powershell_output.empty()) {
        return "";
    }

    // PowerShell output is usually UTF-16 encoded (stored in std::string)
    // Detect if it's UTF-16 encoding
    bool looks_like_utf16 = false;
    size_t byte_count = powershell_output.length();

    if (byte_count >= 2 && byte_count % 2 == 0) {
        // Check if the first few characters match UTF-16 LE pattern
        int utf16_candidates = 0;
        size_t check_bytes = (byte_count < 20) ? byte_count : 20;
        for (size_t i = 0; i < check_bytes; i += 2) {
            if (i + 1 < byte_count) {
                unsigned char low =
                    static_cast<unsigned char>(powershell_output[i]);
                unsigned char high =
                    static_cast<unsigned char>(powershell_output[i + 1]);
                // Check if it's UTF-16 representation of ASCII characters
                if (high == 0 && low >= 32 && low <= 126) {
                    utf16_candidates++;
                } else if (high == 0 &&
                           (low == '\n' || low == '\r' || low == '\t')) {
                    utf16_candidates++;
                }
            }
        }
        // If most characters match UTF-16 pattern, consider it UTF-16
        looks_like_utf16 = (utf16_candidates > 1);
    }

    if (looks_like_utf16) {
        // Process as UTF-16
        std::wstring utf16_text = ConvertUtf16LeToWString(powershell_output);
        if (!utf16_text.empty()) {
            return UnicodeToUtf8(utf16_text);
        }
    }

    // Default to UTF-8 processing
    return powershell_output;
}

// WSL output encoding conversion function
std::string ConvertWslOutputToUtf8(const std::string& wsl_output,
                                   bool is_stderr) {
    if (wsl_output.empty()) {
        return "";
    }

    // Based on user discovery: WSL stderr output may be mixed encoding of
    // UTF-16 LE and UTF-8
    if (is_stderr) {
        // For stderr, try to find the start position of UTF-8 part
        size_t utf8_start = FindUtf8StartPosition(wsl_output);

        if (utf8_start != std::string::npos) {
            // Found UTF-8 part, extract and return
            std::string utf8_part = wsl_output.substr(utf8_start);
            return utf8_part;
        }

        // If UTF-8 part not found, try to process entirely as UTF-16
        std::wstring utf16_text = ConvertUtf16LeToWString(wsl_output);
        if (!utf16_text.empty()) {
            return UnicodeToUtf8(utf16_text);
        }

        // If UTF-16 conversion also fails, return original data directly
        return wsl_output;
    }

    // For stdout or UTF-16 conversion failure, detect if it's UTF-16 format
    bool looks_like_utf16 = false;
    size_t byte_count = wsl_output.length();

    if (byte_count >= 4 && byte_count % 2 == 0) {
        // Check if the first few characters match UTF-16 LE pattern
        int utf16_candidates = 0;
        size_t check_bytes = (byte_count < 20) ? byte_count : 20;
        for (size_t i = 0; i < check_bytes; i += 2) {
            if (i + 1 < byte_count) {
                unsigned char low = static_cast<unsigned char>(wsl_output[i]);
                unsigned char high =
                    static_cast<unsigned char>(wsl_output[i + 1]);
                // Check if it's UTF-16 representation of ASCII characters
                // (high byte is 0, low byte is printable character)
                if (high == 0 && low >= 32 && low <= 126) {
                    utf16_candidates++;
                }
            }
        }
        // If most characters match UTF-16 pattern, consider it UTF-16
        looks_like_utf16 = (utf16_candidates > 2);
    }

    if (looks_like_utf16) {
        // Process as UTF-16
        std::wstring utf16_text = ConvertUtf16LeToWString(wsl_output);
        if (!utf16_text.empty()) {
            return UnicodeToUtf8(utf16_text);
        }
    }

    // Default to UTF-8 processing
    return wsl_output;
}

// UTF-16 LE encoding conversion helper function
std::wstring ConvertUtf16LeToWString(const std::string& utf16_bytes) {
    if (utf16_bytes.empty()) {
        return L"";
    }

    // UTF-16 LE: Each character takes 2 bytes, low byte first, high byte second
    size_t byte_count = utf16_bytes.length();

    // Ensure byte count is even (UTF-16 characteristic)
    if (byte_count % 2 != 0) {
        byte_count -= 1;  // Truncate the last byte
    }

    if (byte_count == 0) {
        return L"";
    }

    try {
        std::wstring result;
        result.reserve(byte_count / 2);  // Pre-allocate space

        for (size_t i = 0; i < byte_count; i += 2) {
            // UTF-16 LE: Low byte first, high byte second
            unsigned char low_byte = static_cast<unsigned char>(utf16_bytes[i]);
            unsigned char high_byte =
                static_cast<unsigned char>(utf16_bytes[i + 1]);

            // Combine into UTF-16 character
            wchar_t utf16_char =
                static_cast<wchar_t>((high_byte << 8) | low_byte);

            // Stop when encountering null terminator
            if (utf16_char == 0) {
                break;
            }

            // Filter out some control characters, but keep common whitespace
            // characters
            if (utf16_char >= 32 || utf16_char == L'\n' ||
                utf16_char == L'\r' || utf16_char == L'\t') {
                result += utf16_char;
            }
        }

        return result;

    } catch (const std::exception& e) {
        return L"";
    }
}

// Helper function to find UTF-8 start position
size_t FindUtf8StartPosition(const std::string& mixed_output) {
    const size_t len = mixed_output.size();
    if (len < 4) return std::string::npos;

    // Find 0D 00 0A 00 (UTF-16-LE \r\n) as UTF-16 paragraph end marker
    for (size_t i = 0; i < len - 4; ++i) {
        if ((unsigned char)mixed_output[i] == 0x0D &&
            (unsigned char)mixed_output[i + 1] == 0x00 &&
            (unsigned char)mixed_output[i + 2] == 0x0A &&
            (unsigned char)mixed_output[i + 3] == 0x00) {
            // Start from i+4 to find UTF-8 string (usually no 0x00, ASCII or
            // valid UTF-8)
            size_t utf8_candidate = i + 4;

            // Try to find a reasonable continuous ASCII string
            size_t printable_count = 0;
            for (size_t j = utf8_candidate; j < len && printable_count < 10;
                 ++j) {
                unsigned char byte =
                    static_cast<unsigned char>(mixed_output[j]);
                if (byte >= 32 && byte <= 126) {
                    printable_count++;
                } else if (byte == 0) {
                    // UTF-8 should not have null byte
                    printable_count = 0;
                    break;
                } else if (byte >= 0xC0 && byte <= 0xF7) {
                    // One of UTF-8 multi-byte sequence start bytes
                    printable_count++;
                } else {
                    break;
                }
            }

            if (printable_count >= 5) {
                return utf8_candidate;
            }
        }
    }

    return len;
}

std::string TrimNewlines(const std::string& str) {
    if (str.empty()) return str;

    size_t start = 0;
    size_t end = str.length();

    // Remove newlines from the end
    while (end > start && (str[end - 1] == '\n' || str[end - 1] == '\r')) {
        end--;
    }

    // Remove newlines from the beginning
    while (start < end && (str[start] == '\n' || str[start] == '\r')) {
        start++;
    }

    return str.substr(start, end - start);
}

int64_t GetFileSize(const char* path) {
    if (!path) return -1;

    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fileData)) {
        return -1;
    }

    LARGE_INTEGER size;
    size.HighPart = fileData.nFileSizeHigh;
    size.LowPart = fileData.nFileSizeLow;

    return size.QuadPart;
}

std::string GetProxyUrl() {
    auto& config_manager = parallax::config::ConfigManager::GetInstance();
    return config_manager.GetConfigValue(parallax::config::KEY_PROXY_URL);
}

bool DownloadFile(const std::string& url, const std::string& local_path) {
    // Use PowerShell's Invoke-WebRequest to download file
    std::string powershell_cmd = "Invoke-WebRequest -Uri \"" + url +
                                 "\" -OutFile \"" + local_path + "\"";

    std::string stdout_output, stderr_output;
    int result =
        ExecCommandEx("powershell.exe -Command \"" + powershell_cmd + "\"", 600,
                      stdout_output, stderr_output, false, true);

    // Check if file was downloaded successfully
    if (result == 0) {
        DWORD file_attributes = GetFileAttributesA(local_path.c_str());
        return (file_attributes != INVALID_FILE_ATTRIBUTES);
    }

    return false;
}

// GPU detection related function implementations
GPUInfo GetNvidiaGPUInfo() {
    GPUInfo gpu_info = {};
    gpu_info.is_nvidia = false;
    gpu_info.is_blackwell_series = false;

    HRESULT hres;

    // Initialize COM
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return gpu_info;
    }

    // Initialize COM security
    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,
                                NULL);

    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        CoUninitialize();
        return gpu_info;
    }

    // Get WMI locator
    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres)) {
        CoUninitialize();
        return gpu_info;
    }

    // Connect to WMI
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0,
                               0, &pSvc);

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return gpu_info;
    }

    // Set proxy security level
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                             RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return gpu_info;
    }

    // Query graphics card information
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"), bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
        &pEnumerator);

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return gpu_info;
    }

    // Enumerate graphics cards
    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (0 == uReturn) {
            break;
        }

        VARIANT vtProp;
        VariantInit(&vtProp);

        // Get graphics card name
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
            std::wstring wname(vtProp.bstrVal);
            std::string name(wname.begin(), wname.end());

            // Check if it's an NVIDIA graphics card
            if (name.find("NVIDIA") != std::string::npos ||
                name.find("GeForce") != std::string::npos ||
                name.find("RTX") != std::string::npos ||
                name.find("GTX") != std::string::npos ||
                name.find("Quadro") != std::string::npos ||
                name.find("Tesla") != std::string::npos) {
                gpu_info.is_nvidia = true;
                gpu_info.name = name;

                // Check if it's Blackwell series (RTX50xx, Bxxx)
                if (name.find("RTX 50") != std::string::npos ||
                    name.find("RTX50") != std::string::npos ||
                    name.find("B100") != std::string::npos ||
                    name.find("B200") != std::string::npos ||
                    name.find("B40") != std::string::npos) {
                    gpu_info.is_blackwell_series = true;
                }

                VariantClear(&vtProp);
                pclsObj->Release();
                break;
            }
        }
        VariantClear(&vtProp);
        pclsObj->Release();
    }

    // Clean up resources
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return gpu_info;
}

CUDAInfo GetCUDAInfo() {
    CUDAInfo cuda_info = {};
    cuda_info.is_valid_version = false;

    // First get driver version
    std::string stdout_output, stderr_output;
    int exit_code = ExecCommandEx(
        "nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits",
        30, stdout_output, stderr_output, false, true);

    if (exit_code == 0 && !stdout_output.empty()) {
        cuda_info.driver_version = stdout_output;
        // Remove newlines and whitespace characters
        cuda_info.driver_version.erase(
            std::remove_if(cuda_info.driver_version.begin(),
                           cuda_info.driver_version.end(), ::isspace),
            cuda_info.driver_version.end());
    }

    // Check CUDA toolkit version
    exit_code = ExecCommandEx("nvcc --version", 30, stdout_output,
                              stderr_output, false, true);

    if (exit_code == 0 && !stdout_output.empty()) {
        // Parse CUDA version number, format like: Cuda compilation tools,
        // release 12.8, V12.8.123
        std::regex version_regex(R"(release\s+(\d+\.\d+))");
        std::smatch match;

        if (std::regex_search(stdout_output, match, version_regex)) {
            cuda_info.version = match[1].str();

            // Check if version is 12.8x or 12.9x
            if (cuda_info.version.find("12.8") == 0 ||
                cuda_info.version.find("12.9") == 0) {
                cuda_info.is_valid_version = true;
            }
        }
    }

    return cuda_info;
}

// WSL command building utility function implementations
std::string GetWSLCommandPrefix(const std::string& ubuntu_version) {
    return "wsl -d " + ubuntu_version + " -u root";
}

std::string BuildWSLCommand(const std::string& ubuntu_version,
                            const std::string& command) {
    return GetWSLCommandPrefix(ubuntu_version) + " bash -c \"" + command + "\"";
}

std::string BuildWSLDirectCommand(const std::string& ubuntu_version,
                                  const std::string& command) {
    return GetWSLCommandPrefix(ubuntu_version) + " " + command;
}

}  // namespace utils
}  // namespace parallax
