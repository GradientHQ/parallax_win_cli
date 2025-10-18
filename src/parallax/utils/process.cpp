#include "process.h"
#include "utils.h"
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>  // Added for std::atomic

namespace parallax {
namespace utils {

// Get system directory path
static std::string GetSystemPath() {
    char system_path[MAX_PATH];
    GetSystemDirectoryA(system_path, MAX_PATH);
    return std::string(system_path);
}

int ExecCommandEx(const std::string& cmd, int timeout,
                  std::string& stdout_output, std::string& stderr_output,
                  bool elevate /* = false*/,
                  bool skip_encoding_conversion /* = false*/) {
    if (cmd.empty() || timeout <= 0) {
        return -1;
    }

    int ret = 0;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

    HANDLE hReadOut = nullptr, hWriteOut = nullptr;
    HANDLE hReadErr = nullptr, hWriteErr = nullptr;
    HANDLE hReadInput = nullptr, hWriteInput = nullptr;

    stdout_output.clear();
    stderr_output.clear();

    std::string system_path = GetSystemPath();
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi = {0};
    std::atomic<bool> process_terminated(false);

    // Create stdout pipe
    if (!CreatePipe(&hReadOut, &hWriteOut, &sa, 0)) return -1;
    // Create stderr pipe
    if (!CreatePipe(&hReadErr, &hWriteErr, &sa, 0)) return -1;
    // Create stdin pipe
    if (!CreatePipe(&hReadInput, &hWriteInput, &sa, 0)) return -1;

    // Prevent parent process from inheriting handles it shouldn't
    SetHandleInformation(hReadOut, HANDLE_FLAG_INHERIT,
                         0);  // Parent reads stdout
    SetHandleInformation(hReadErr, HANDLE_FLAG_INHERIT,
                         0);  // Parent reads stderr
    SetHandleInformation(hWriteInput, HANDLE_FLAG_INHERIT,
                         0);  // Parent writes stdin

    // Set child process stdxxx
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWriteOut;
    si.hStdError = hWriteErr;
    si.hStdInput = hReadInput;

    std::string cmdline = "cmd /C " + cmd;
    bool created = false;

    // Create read thread (before process creation)
    std::thread thread_read_out([&]() {
        char buffer[4096];
        DWORD bytesRead;
        while (true) {
            DWORD available = 0;
            if (PeekNamedPipe(hReadOut, nullptr, 0, nullptr, &available,
                              nullptr) &&
                available > 0) {
                if (ReadFile(hReadOut, buffer, sizeof(buffer), &bytesRead,
                             nullptr)) {
                    if (bytesRead > 0) {
                        stdout_output.append(buffer, bytesRead);
                        continue;
                    }
                } else {
                    break;  // ReadFile failed (pipe closed)
                }
            }

            // If no data to read and termination flag set, try reading one more
            // time
            if (process_terminated.load()) {
                DWORD available = 0;
                if (PeekNamedPipe(hReadOut, nullptr, 0, nullptr, &available,
                                  nullptr) &&
                    available > 0) {
                    if (ReadFile(hReadOut, buffer, sizeof(buffer), &bytesRead,
                                 nullptr)) {
                        if (bytesRead > 0) {
                            stdout_output.append(buffer, bytesRead);
                        }
                    }
                }
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    std::thread thread_read_err([&]() {
        char buffer[4096];
        DWORD bytesRead;
        while (true) {
            DWORD available = 0;
            if (PeekNamedPipe(hReadErr, nullptr, 0, nullptr, &available,
                              nullptr) &&
                available > 0) {
                if (ReadFile(hReadErr, buffer, sizeof(buffer), &bytesRead,
                             nullptr)) {
                    if (bytesRead > 0) {
                        stderr_output.append(buffer, bytesRead);
                        continue;
                    }
                } else {
                    break;  // ReadFile failed (pipe closed)
                }
            }

            // If no data to read and termination flag set, try reading one more
            // time
            if (process_terminated.load()) {
                DWORD available = 0;
                if (PeekNamedPipe(hReadErr, nullptr, 0, nullptr, &available,
                                  nullptr) &&
                    available > 0) {
                    if (ReadFile(hReadErr, buffer, sizeof(buffer), &bytesRead,
                                 nullptr)) {
                        if (bytesRead > 0) {
                            stderr_output.append(buffer, bytesRead);
                        }
                    }
                }
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Wait briefly for threads to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Create process (simplified version, no token used)
    created = CreateProcessA(nullptr, const_cast<char*>(cmdline.c_str()),
                             nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                             system_path.c_str(), &si, &pi);

    if (!created) {
        ret = -1;
        stderr_output =
            "create process fail: " + std::to_string(GetLastError());

        // Process creation failed, need to clean up threads
        process_terminated = true;

        // Close write end to let threads exit
        if (hWriteOut) {
            CloseHandle(hWriteOut);
            hWriteOut = nullptr;
        }
        if (hWriteErr) {
            CloseHandle(hWriteErr);
            hWriteErr = nullptr;
        }

        // Wait for threads to exit
        thread_read_out.join();
        thread_read_err.join();

        // Clean up remaining pipe resources
        if (hReadInput) CloseHandle(hReadInput);
        if (hWriteInput) CloseHandle(hWriteInput);
        if (hReadOut) CloseHandle(hReadOut);
        if (hReadErr) CloseHandle(hReadErr);
        return ret;
    }

    // Process created, now enter waiting logic
    DWORD startTime = GetTickCount();
    bool timeout_occurred = false;

    while (true) {
        // Check if process exited naturally
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 100);

        if (waitResult == WAIT_OBJECT_0) {
            // Process has exited
            break;
        } else if (waitResult == WAIT_TIMEOUT) {
            // Check if timeout occurred
            if ((GetTickCount() - startTime) > (timeout * 1000)) {
                timeout_occurred = true;
                break;
            }
        } else {
            // Wait error occurred
            break;
        }
    }

    // If timeout, force terminate process
    if (timeout_occurred) {
        TerminateProcess(pi.hProcess, static_cast<UINT>(-1));
        stderr_output =
            "cmd is auto killed, timeout: " + std::to_string(timeout) + "\n>" +
            stderr_output;
        ret = -2;
    }

    // Notify read threads to stop
    process_terminated = true;

    // Close write end
    if (hWriteErr) {
        CloseHandle(hWriteErr);
        hWriteErr = nullptr;
    }
    if (hWriteOut) {
        CloseHandle(hWriteOut);
        hWriteOut = nullptr;
    }
    if (hWriteInput) {
        CloseHandle(hWriteInput);
        hWriteInput = nullptr;
    }

    // Wait for read threads to complete
    thread_read_out.join();
    thread_read_err.join();

    // Get process exit code
    if (ret == 0) {
        DWORD exit_code;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        ret = static_cast<int>(exit_code);
    }

    // Decide whether to perform encoding conversion based on parameters
    if (!skip_encoding_conversion) {
        // Ensure output is UTF-8 encoded
        stdout_output = ConvertPowerShellOutputToUtf8(stdout_output);
        stderr_output = ConvertPowerShellOutputToUtf8(stderr_output);
    }

    // Clean up resources
    if (hReadInput) CloseHandle(hReadInput);
    if (hReadOut) CloseHandle(hReadOut);
    if (hReadErr) CloseHandle(hReadErr);
    if (pi.hThread) CloseHandle(pi.hThread);
    if (pi.hProcess) CloseHandle(pi.hProcess);

    return ret;
}

int ExecCommandEx2(const std::string& cmd, int timeout,
                   std::string& stdout_output, std::string& stderr_output,
                   std::function<bool()> check_callback, bool elevate,
                   bool skip_encoding_conversion) {
    if (cmd.empty() || timeout <= 0) {
        return -1;
    }

    int ret = 0;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

    HANDLE hReadOut = nullptr, hWriteOut = nullptr;
    HANDLE hReadErr = nullptr, hWriteErr = nullptr;
    HANDLE hReadInput = nullptr, hWriteInput = nullptr;

    stdout_output.clear();
    stderr_output.clear();

    std::string system_path = GetSystemPath();
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi = {0};
    std::atomic<bool> process_terminated(false);

    // Create stdout pipe
    if (!CreatePipe(&hReadOut, &hWriteOut, &sa, 0)) return -1;
    // Create stderr pipe
    if (!CreatePipe(&hReadErr, &hWriteErr, &sa, 0)) return -1;
    // Create stdin pipe
    if (!CreatePipe(&hReadInput, &hWriteInput, &sa, 0)) return -1;

    // Prevent parent process from inheriting handles it shouldn't
    SetHandleInformation(hReadOut, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hReadErr, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hWriteInput, HANDLE_FLAG_INHERIT, 0);

    // Set child process stdxxx
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWriteOut;
    si.hStdError = hWriteErr;
    si.hStdInput = hReadInput;

    std::string cmdline = "cmd /C " + cmd;
    bool created = false;

    // Create read thread
    std::thread thread_read_out([&]() {
        char buffer[4096];
        DWORD bytesRead;
        while (true) {
            DWORD available = 0;
            if (PeekNamedPipe(hReadOut, nullptr, 0, nullptr, &available,
                              nullptr) &&
                available > 0) {
                if (ReadFile(hReadOut, buffer, sizeof(buffer), &bytesRead,
                             nullptr)) {
                    if (bytesRead > 0) {
                        stdout_output.append(buffer, bytesRead);
                        continue;
                    }
                } else {
                    break;
                }
            }

            if (process_terminated.load()) {
                DWORD available = 0;
                if (PeekNamedPipe(hReadOut, nullptr, 0, nullptr, &available,
                                  nullptr) &&
                    available > 0) {
                    if (ReadFile(hReadOut, buffer, sizeof(buffer), &bytesRead,
                                 nullptr)) {
                        if (bytesRead > 0) {
                            stdout_output.append(buffer, bytesRead);
                        }
                    }
                }
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    std::thread thread_read_err([&]() {
        char buffer[4096];
        DWORD bytesRead;
        while (true) {
            DWORD available = 0;
            if (PeekNamedPipe(hReadErr, nullptr, 0, nullptr, &available,
                              nullptr) &&
                available > 0) {
                if (ReadFile(hReadErr, buffer, sizeof(buffer), &bytesRead,
                             nullptr)) {
                    if (bytesRead > 0) {
                        stderr_output.append(buffer, bytesRead);
                        continue;
                    }
                } else {
                    break;
                }
            }

            if (process_terminated.load()) {
                DWORD available = 0;
                if (PeekNamedPipe(hReadErr, nullptr, 0, nullptr, &available,
                                  nullptr) &&
                    available > 0) {
                    if (ReadFile(hReadErr, buffer, sizeof(buffer), &bytesRead,
                                 nullptr)) {
                        if (bytesRead > 0) {
                            stderr_output.append(buffer, bytesRead);
                        }
                    }
                }
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Wait briefly for threads to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Create process
    created = CreateProcessA(nullptr, const_cast<char*>(cmdline.c_str()),
                             nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                             system_path.c_str(), &si, &pi);

    if (!created) {
        ret = -1;
        stderr_output =
            "create process fail: " + std::to_string(GetLastError());

        // Process creation failed, clean up threads
        process_terminated = true;

        if (hWriteOut) {
            CloseHandle(hWriteOut);
            hWriteOut = nullptr;
        }
        if (hWriteErr) {
            CloseHandle(hWriteErr);
            hWriteErr = nullptr;
        }

        thread_read_out.join();
        thread_read_err.join();

        // Clean up remaining resources
        if (hReadInput) CloseHandle(hReadInput);
        if (hWriteInput) CloseHandle(hWriteInput);
        if (hReadOut) CloseHandle(hReadOut);
        if (hReadErr) CloseHandle(hReadErr);
        return ret;
    }

    // Process created, enter waiting logic (with callback support)
    DWORD startTime = GetTickCount();
    bool callback_result = false;
    bool timeout_occurred = false;

    while (true) {
        // Check if process exited naturally
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 100);

        if (waitResult == WAIT_OBJECT_0) {
            // Process has exited
            break;
        } else if (waitResult == WAIT_TIMEOUT) {
            // Check if timeout occurred
            if ((GetTickCount() - startTime) > (timeout * 1000)) {
                timeout_occurred = true;
                break;
            }

            // Call callback function to check if termination is needed
            if (check_callback && check_callback()) {
                callback_result = true;
                break;
            }
        } else {
            // Wait error occurred
            break;
        }
    }

    // If timeout or callback requests termination, force terminate the process
    if (timeout_occurred || callback_result) {
        TerminateProcess(pi.hProcess, static_cast<UINT>(-1));

        if (timeout_occurred) {
            stderr_output =
                "cmd is auto killed, timeout: " + std::to_string(timeout) +
                "\n>" + stderr_output;
            ret = -2;
        } else {
            stderr_output = "cmd is terminated by callback\n>" + stderr_output;
            ret = -3;
        }
    }

    // Notify read threads to stop
    process_terminated = true;

    // Close write end
    if (hWriteErr) {
        CloseHandle(hWriteErr);
        hWriteErr = nullptr;
    }
    if (hWriteOut) {
        CloseHandle(hWriteOut);
        hWriteOut = nullptr;
    }
    if (hWriteInput) {
        CloseHandle(hWriteInput);
        hWriteInput = nullptr;
    }

    // Wait for read threads to complete
    thread_read_out.join();
    thread_read_err.join();

    // Get process exit code
    if (ret == 0) {
        DWORD exit_code;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        ret = static_cast<int>(exit_code);
    }

    // Decide whether to perform encoding conversion based on parameters
    if (!skip_encoding_conversion) {
        // Ensure output is UTF-8 encoded
        stdout_output = ConvertPowerShellOutputToUtf8(stdout_output);
        stderr_output = ConvertPowerShellOutputToUtf8(stderr_output);
    }

    // Clean up resources
    if (hReadInput) CloseHandle(hReadInput);
    if (hReadOut) CloseHandle(hReadOut);
    if (hReadErr) CloseHandle(hReadErr);
    if (pi.hThread) CloseHandle(pi.hThread);
    if (pi.hProcess) CloseHandle(pi.hProcess);

    return ret;
}

}  // namespace utils
}  // namespace parallax
