#include "system_checker.h"
#include "environment_installer.h"
#include "utils/utils.h"
#include "utils/process.h"
#include "tinylog/tinylog.h"
#include <windows.h>
#include <winternl.h>
#include <regex>
#include <algorithm>

// Use Windows SDK definition to declare RtlGetVersion function
extern "C" NTSTATUS NTAPI
RtlGetVersion(PRTL_OSVERSIONINFOW lpVersionInformation);

namespace parallax {
namespace environment {

// OSVersionChecker implementation
OSVersionChecker::OSVersionChecker(std::shared_ptr<ExecutionContext> context)
    : BaseEnvironmentComponent(context) {}

ComponentResult OSVersionChecker::Check() {
    LogOperationStart("Checking");

    // Use RtlGetVersion to get real Windows version
    RTL_OSVERSIONINFOW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    NTSTATUS status = RtlGetVersion(&osvi);
    if (status != 0) {
        return CreateFailureResult("Failed to get OS version", 10);
    }

    // According to Microsoft official documentation, WSL2 system requirements:
    // - Windows 10: version 2004 and later (build 19041 and later) or
    // - Windows 10: version 1909 and later (build 18362 and later) for x64
    // systems
    // - Windows 11: all versions supported

    bool is_supported = false;
    char version_info[512];

    if (osvi.dwMajorVersion >= 11) {
        // Windows 11 and later versions fully supported
        is_supported = true;
        sprintf_s(version_info, sizeof(version_info),
                  "Windows %lu.%lu.%lu (supported)", osvi.dwMajorVersion,
                  osvi.dwMinorVersion, osvi.dwBuildNumber);
    } else if (osvi.dwMajorVersion == 10) {
        // Windows 10 needs to check specific build version
        if (osvi.dwBuildNumber >= 19041) {
            // Version 2004 (build 19041) and later, fully supports all
            // architectures
            is_supported = true;
        } else if (osvi.dwBuildNumber >= 18362) {
            // Version 1909 (build 18362), only supports x64 architecture
            // Simplified handling here, assuming x64 system
            is_supported = true;
        }

        sprintf_s(version_info, sizeof(version_info), "Windows 10.%lu.%lu (%s)",
                  osvi.dwMinorVersion, osvi.dwBuildNumber,
                  is_supported ? "supported" : "unsupported");
    } else {
        // Windows 9 and earlier versions not supported
        sprintf_s(version_info, sizeof(version_info),
                  "Windows %lu.%lu.%lu (unsupported - requires Windows 10 "
                  "build 18362+ or Windows 11)",
                  osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
    }

    ComponentResult result =
        is_supported ? CreateSuccessResult(std::string(version_info))
                     : CreateFailureResult(std::string(version_info), 10);

    LogOperationResult("Checking", result);
    return result;
}

EnvironmentComponent OSVersionChecker::GetComponentType() const {
    return EnvironmentComponent::kOSVersion;
}

std::string OSVersionChecker::GetComponentName() const { return "OS Version"; }

// NvidiaGPUChecker implementation
NvidiaGPUChecker::NvidiaGPUChecker(std::shared_ptr<ExecutionContext> context)
    : BaseEnvironmentComponent(context) {}

ComponentResult NvidiaGPUChecker::Check() {
    LogOperationStart("Checking");

    info_log("[ENV] Starting NVIDIA GPU hardware detection");

    // Use encapsulated function to detect GPU
    parallax::utils::GPUInfo gpu_info = parallax::utils::GetNvidiaGPUInfo();

    if (!gpu_info.is_nvidia) {
        error_log("[ENV] No NVIDIA GPU found in the system");
        ComponentResult result =
            CreateFailureResult("No NVIDIA GPU detected", 7);
        LogOperationResult("Checking", result);
        return result;
    }

    info_log("[ENV] Found NVIDIA GPU: %s", gpu_info.name.c_str());

    if (!IsGPUMeetsMinimumRequirement(gpu_info.name)) {
        error_log("[ENV] NVIDIA GPU does not meet minimum requirements: %s",
                  gpu_info.name.c_str());
        ComponentResult result = CreateFailureResult(
            "GPU below minimum requirement: " + gpu_info.name, 8);
        LogOperationResult("Checking", result);
        return result;
    }

    std::string result_message =
        "Compatible NVIDIA GPU detected: " + gpu_info.name;
    if (gpu_info.is_blackwell_series) {
        result_message += " (Blackwell series - will use blackwell image)";
    } else {
        result_message += " (will use hopper image)";
    }

    info_log("[ENV] NVIDIA GPU meets requirements: %s", gpu_info.name.c_str());
    ComponentResult result = CreateSuccessResult(result_message);
    LogOperationResult("Checking", result);
    return result;
}

bool NvidiaGPUChecker::IsGPUMeetsMinimumRequirement(
    const std::string& gpu_name) {
    std::string gpu_upper = gpu_name;
    std::transform(gpu_upper.begin(), gpu_upper.end(), gpu_upper.begin(),
                   ::toupper);

    info_log("[ENV] Checking GPU requirement for: %s", gpu_name.c_str());

    // 1. Check high-end professional and data center cards (all meet
    // requirements)
    std::vector<std::string> high_end_cards = {
        "TESLA", "QUADRO RTX", "RTX A", "A100", "H100",
        "A40",   "A30",        "A10",   "V100", "P100"};

    for (const auto& card : high_end_cards) {
        if (gpu_upper.find(card) != std::string::npos) {
            info_log("[ENV] GPU identified as high-end/professional card: %s",
                     card.c_str());
            return true;
        }
    }

    // 2. Handle GeForce RTX series (consumer graphics cards)
    if (gpu_upper.find("GEFORCE") != std::string::npos ||
        gpu_upper.find("RTX") != std::string::npos) {
        // Extract RTX series and model numbers
        std::regex rtx_pattern(R"(RTX\s*(\d+)(\d{2,3})(?:\s*(TI|SUPER))?)",
                               std::regex_constants::icase);
        std::smatch match;

        if (std::regex_search(gpu_upper, match, rtx_pattern)) {
            int series = std::stoi(match[1].str());
            int model = std::stoi(match[2].str());
            std::string suffix = match[3].str();

            info_log("[ENV] GPU parsed - Series: %d, Model: %d, Suffix: %s",
                     series, model, suffix.c_str());

            // RTX 50 series and above (future graphics cards)
            if (series >= 50) {
                info_log(
                    "[ENV] GPU is RTX %d series (future generation), accepting",
                    series);
                return true;
            }

            // RTX 40 series - accept all (4060 and above)
            if (series == 40) {
                if (model >= 60) {
                    info_log(
                        "[ENV] GPU is RTX 40 series with model >= 60, "
                        "accepting");
                    return true;
                }
                info_log(
                    "[ENV] GPU is RTX 40 series but model < 60, rejecting");
                return false;
            }

            // RTX 30 series - requires 3060 Ti or higher
            if (series == 30) {
                if (model > 60) {
                    // 3070, 3080, 3090, etc. are all acceptable
                    info_log(
                        "[ENV] GPU is RTX 30 series with model > 60, "
                        "accepting");
                    return true;
                } else if (model == 60) {
                    // Only Ti version of 3060 is acceptable
                    if (suffix.find("TI") != std::string::npos) {
                        info_log("[ENV] GPU is RTX 3060 Ti, accepting");
                        return true;
                    } else {
                        info_log("[ENV] GPU is RTX 3060 (non-Ti), rejecting");
                        return false;
                    }
                } else {
                    info_log(
                        "[ENV] GPU is RTX 30 series with model < 60, "
                        "rejecting");
                    return false;
                }
            }

            // RTX 20 series and below do not meet requirements
            if (series <= 20) {
                info_log("[ENV] GPU is RTX %d series (too old), rejecting",
                         series);
                return false;
            }
        }
    }

    // 3. Handle GTX series (none meet requirements, as RTX 3060 Ti is minimum)
    if (gpu_upper.find("GTX") != std::string::npos) {
        info_log("[ENV] GPU is GTX series (too old), rejecting");
        return false;
    }

    // 4. Other unknown NVIDIA graphics cards, conservatively reject
    info_log("[ENV] GPU type unknown or unrecognized, rejecting");
    return false;
}

EnvironmentComponent NvidiaGPUChecker::GetComponentType() const {
    return EnvironmentComponent::kNvidiaGPU;
}

std::string NvidiaGPUChecker::GetComponentName() const {
    return "NVIDIA GPU Hardware";
}

// NvidiaDriverChecker implementation
NvidiaDriverChecker::NvidiaDriverChecker(
    std::shared_ptr<ExecutionContext> context)
    : BaseEnvironmentComponent(context) {}

ComponentResult NvidiaDriverChecker::Check() {
    LogOperationStart("Checking");

    // Check if NVIDIA driver is installed through nvidia-smi command
    std::string stdout_output, stderr_output;
    int exit_code = parallax::utils::ExecCommandEx(
        "nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits",
        30, stdout_output, stderr_output, false, true);

    if (exit_code == 0 && !stdout_output.empty()) {
        // Parse driver version information
        std::string driver_version = stdout_output;
        // Remove newlines and whitespace characters
        driver_version.erase(std::remove_if(driver_version.begin(),
                                            driver_version.end(), ::isspace),
                             driver_version.end());

        if (!driver_version.empty()) {
            // Use encapsulated function to check CUDA version
            parallax::utils::CUDAInfo cuda_info =
                parallax::utils::GetCUDAInfo();

            std::string result_message = "NVIDIA driver: " + driver_version +
                                         ", CUDA toolkit: " + cuda_info.version;

            if (!cuda_info.is_valid_version &&
                cuda_info.version != "Not detected") {
                result_message +=
                    " (WARNING: CUDA version should be 12.8.x or 12.9.x)";
            }

            ComponentResult result = CreateSuccessResult(result_message);
            LogOperationResult("Checking", result);
            return result;
        }
    }

    // If nvidia-smi command fails, try checking through registry
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\NVIDIA Corporation\\Global\\Display Driver", 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {
        char version_buffer[256];
        DWORD buffer_size = sizeof(version_buffer);
        if (RegQueryValueExA(hKey, "Version", NULL, NULL,
                             (LPBYTE)version_buffer,
                             &buffer_size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            ComponentResult result = CreateSuccessResult(
                "NVIDIA driver installed (registry version: " +
                std::string(version_buffer) + ")");
            LogOperationResult("Checking", result);
            return result;
        }
        RegCloseKey(hKey);
    }

    std::string error_message =
        "NVIDIA driver not found. Please install NVIDIA graphics driver first.";

    ComponentResult result = CreateFailureResult(error_message, 20);
    LogOperationResult("Checking", result);
    return result;
}

EnvironmentComponent NvidiaDriverChecker::GetComponentType() const {
    return EnvironmentComponent::kNvidiaDriver;
}

std::string NvidiaDriverChecker::GetComponentName() const {
    return "NVIDIA Driver";
}

// BIOSVirtualizationChecker implementation
BIOSVirtualizationChecker::BIOSVirtualizationChecker(
    std::shared_ptr<ExecutionContext> context,
    std::shared_ptr<CommandExecutor> executor)
    : BaseEnvironmentComponent(context), executor_(executor) {}

ComponentResult BIOSVirtualizationChecker::Check() {
    LogOperationStart("Checking");

    // Use systeminfo command to check virtualization support - this method
    // doesn't depend on WSL distribution
    auto [systeminfo_code, systeminfo_output] =
        executor_->ExecutePowerShell("systeminfo");

    if (systeminfo_code == 0) {
        // Check Hyper-V requirements line, this reflects virtualization support
        // status
        if (systeminfo_output.find("Virtualization Enabled In Firmware: Yes") !=
            std::string::npos) {
            ComponentResult result =
                CreateSuccessResult("BIOS virtualization is enabled");
            LogOperationResult("Checking", result);
            return result;
        } else if (systeminfo_output.find(
                       "Virtualization Enabled In Firmware: No") !=
                   std::string::npos) {
            ComponentResult result = CreateFailureResult(
                "BIOS virtualization is not enabled. Please restart your "
                "computer and enable virtualization in BIOS settings.",
                20);
            LogOperationResult("Checking", result);
            return result;
        }
    }

    // Backup method: use wsl --status command to check virtualization support
    auto [wsl_status_code, wsl_status_output] =
        executor_->ExecutePowerShell("wsl --status");

    // Check if there are virtualization error prompts in wsl --status output
    if (wsl_status_code == 0) {
        // Check if there are error messages prompting to enable BIOS
        // virtualization
        if (wsl_status_output.find(
                "ensure virtualization is enabled in the BIOS") !=
                std::string::npos ||
            wsl_status_output.find("WSL2 is not supported with your current "
                                   "machine configuration") !=
                std::string::npos ||
            wsl_status_output.find("virtualization is not enabled") !=
                std::string::npos) {
            ComponentResult result = CreateFailureResult(
                "BIOS virtualization is not enabled. Please restart your "
                "computer and enable virtualization in BIOS settings.",
                20);
            LogOperationResult("Checking", result);
            return result;
        }

        // If no virtualization errors, consider BIOS virtualization enabled
        ComponentResult result =
            CreateSuccessResult("BIOS virtualization is enabled");
        LogOperationResult("Checking", result);
        return result;
    }

    // If all above methods cannot determine, return success but indicate status
    // cannot be confirmed
    ComponentResult result = CreateSuccessResult(
        "BIOS virtualization status check completed (unable "
        "to verify definitively)");
    LogOperationResult("Checking", result);
    return result;
}

EnvironmentComponent BIOSVirtualizationChecker::GetComponentType() const {
    return EnvironmentComponent::kBIOSVirtualization;
}

std::string BIOSVirtualizationChecker::GetComponentName() const {
    return "BIOS Virtualization";
}

}  // namespace environment
}  // namespace parallax
