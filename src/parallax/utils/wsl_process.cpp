#include "wsl_process.h"
#include "utils.h"
#include "tinylog/tinylog.h"
#include <iostream>
#include <algorithm>

// Static member for console control handler
WSLProcess* WSLProcess::s_instance = nullptr;

WSLProcess::WSLProcess() : running_(false), shouldStop_(false), exitCode_(0) {
    ZeroMemory(&processInfo_, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startupInfo_, sizeof(STARTUPINFOA));
    processHandle_ = INVALID_HANDLE_VALUE;
    threadHandle_ = INVALID_HANDLE_VALUE;
    stdoutWrite_ = INVALID_HANDLE_VALUE;
    stdoutRead_ = INVALID_HANDLE_VALUE;
    stderrWrite_ = INVALID_HANDLE_VALUE;
    stderrRead_ = INVALID_HANDLE_VALUE;
    exitEvent_ = INVALID_HANDLE_VALUE;

    // Create exit event for graceful shutdown
    exitEvent_ = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    // Set this instance as the global instance for console control
    s_instance = this;
}

WSLProcess::~WSLProcess() {
    Stop();
    if (exitEvent_ != INVALID_HANDLE_VALUE) {
        CloseHandle(exitEvent_);
        exitEvent_ = INVALID_HANDLE_VALUE;
    }

    // Clear global instance
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

int WSLProcess::Execute(const std::string& wsl_command) {
    if (running_) {
        error_log("WSLProcess is already running");
        return 1;
    }

    info_log("Executing WSL command: %s", wsl_command.c_str());

    // Set up console control handler for Ctrl+C
    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
        error_log("Failed to set console control handler");
    }

    // Create WSL process
    if (!CreateWSLProcess(wsl_command)) {
        SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
        return 1;
    }

    running_ = true;
    shouldStop_ = false;
    exitCode_ = 0;

    // Start I/O thread
    ioThread_ = std::thread([this]() { IOReaderThread(); });

    // Wait for process to complete
    if (processHandle_ != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(processHandle_, INFINITE);

        DWORD processExitCode = 0;
        if (GetExitCodeProcess(processHandle_, &processExitCode)) {
            exitCode_ = static_cast<int>(processExitCode);
        }
    }

    // Stop I/O thread
    shouldStop_ = true;
    running_ = false;

    if (exitEvent_ != INVALID_HANDLE_VALUE) {
        SetEvent(exitEvent_);
    }

    if (ioThread_.joinable()) {
        ioThread_.join();
    }

    CleanupProcess();

    // Remove console control handler
    SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);

    info_log("WSL command completed with exit code: %d", exitCode_.load());
    return exitCode_;
}

void WSLProcess::Stop() {
    if (!running_) {
        return;
    }

    info_log("Stopping WSL process");

    shouldStop_ = true;
    running_ = false;

    // Signal the exit event to wake up the I/O thread
    if (exitEvent_ != INVALID_HANDLE_VALUE) {
        SetEvent(exitEvent_);
    }

    // Terminate the child process
    if (processHandle_ != INVALID_HANDLE_VALUE) {
        TerminateProcess(processHandle_, 1);
        WaitForSingleObject(processHandle_, 1000);
    }

    // Wait for I/O thread to finish
    if (ioThread_.joinable()) {
        ioThread_.join();
    }

    CleanupProcess();
}

bool WSLProcess::IsRunning() const { return running_.load(); }

bool WSLProcess::CreateWSLProcess(const std::string& command) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    // Create pipes for stdout
    if (!CreatePipe(&stdoutRead_, &stdoutWrite_, &saAttr, 0)) {
        error_log("Failed to create stdout pipe: %lu", GetLastError());
        return false;
    }
    if (!SetHandleInformation(stdoutRead_, HANDLE_FLAG_INHERIT, 0)) {
        error_log("Failed to set stdout handle info: %lu", GetLastError());
        CleanupProcess();
        return false;
    }

    // Create pipes for stderr
    if (!CreatePipe(&stderrRead_, &stderrWrite_, &saAttr, 0)) {
        error_log("Failed to create stderr pipe: %lu", GetLastError());
        CleanupProcess();
        return false;
    }
    if (!SetHandleInformation(stderrRead_, HANDLE_FLAG_INHERIT, 0)) {
        error_log("Failed to set stderr handle info: %lu", GetLastError());
        CleanupProcess();
        return false;
    }

    // Set up startup info
    startupInfo_.cb = sizeof(STARTUPINFOA);
    startupInfo_.hStdError = stderrWrite_;
    startupInfo_.hStdOutput = stdoutWrite_;
    startupInfo_.hStdInput =
        GetStdHandle(STD_INPUT_HANDLE);  // Use current stdin
    startupInfo_.dwFlags |= STARTF_USESTDHANDLES;

    // Prepare command line
    char cmdLine[2048];
    strcpy_s(cmdLine, sizeof(cmdLine), command.c_str());

    // Create process
    BOOL result = CreateProcessA(
        nullptr,           // No module name (use command line)
        cmdLine,           // Command line
        nullptr,           // Process handle not inheritable
        nullptr,           // Thread handle not inheritable
        TRUE,              // Set handle inheritance to TRUE
        CREATE_NO_WINDOW,  // Creation flags
        nullptr,           // Use parent's environment block
        nullptr,           // Use parent's starting directory
        &startupInfo_,     // Pointer to STARTUPINFO structure
        &processInfo_      // Pointer to PROCESS_INFORMATION structure
    );

    if (!result) {
        error_log("Failed to create WSL process: %lu", GetLastError());
        CleanupProcess();
        return false;
    }

    processHandle_ = processInfo_.hProcess;
    threadHandle_ = processInfo_.hThread;

    // Close write ends of pipes in parent process
    CloseHandle(stderrWrite_);
    CloseHandle(stdoutWrite_);
    stderrWrite_ = INVALID_HANDLE_VALUE;
    stdoutWrite_ = INVALID_HANDLE_VALUE;

    info_log("WSL process created successfully, PID: %lu",
             processInfo_.dwProcessId);
    return true;
}

void WSLProcess::CleanupProcess() {
    if (processHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(processHandle_);
        processHandle_ = INVALID_HANDLE_VALUE;
    }

    if (threadHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(threadHandle_);
        threadHandle_ = INVALID_HANDLE_VALUE;
    }

    if (stdoutRead_ != INVALID_HANDLE_VALUE) {
        CloseHandle(stdoutRead_);
        stdoutRead_ = INVALID_HANDLE_VALUE;
    }

    if (stdoutWrite_ != INVALID_HANDLE_VALUE) {
        CloseHandle(stdoutWrite_);
        stdoutWrite_ = INVALID_HANDLE_VALUE;
    }

    if (stderrRead_ != INVALID_HANDLE_VALUE) {
        CloseHandle(stderrRead_);
        stderrRead_ = INVALID_HANDLE_VALUE;
    }

    if (stderrWrite_ != INVALID_HANDLE_VALUE) {
        CloseHandle(stderrWrite_);
        stderrWrite_ = INVALID_HANDLE_VALUE;
    }

    ZeroMemory(&processInfo_, sizeof(PROCESS_INFORMATION));
}

void WSLProcess::IOReaderThread() {
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    DWORD bytesRead;
    DWORD stdoutBytesAvailable = 0;
    DWORD stderrBytesAvailable = 0;

    // Prepare handles for WaitForMultipleObjects
    HANDLE handles[3];
    handles[0] = exitEvent_;   // Exit event (highest priority)
    handles[1] = stdoutRead_;  // Stdout pipe
    handles[2] = stderrRead_;  // Stderr pipe

    info_log("WSL I/O reader thread started");

    while (!shouldStop_ && IsRunning()) {
        // Wait for any handle to be signaled
        DWORD waitResult =
            WaitForMultipleObjects(3, handles, FALSE, 100);  // 100ms timeout

        switch (waitResult) {
            case WAIT_OBJECT_0:  // Exit event signaled
                goto exit_loop;

            case WAIT_OBJECT_0 + 1:  // Stdout ready
                if (PeekNamedPipe(stdoutRead_, nullptr, 0, nullptr,
                                  &stdoutBytesAvailable, nullptr) &&
                    stdoutBytesAvailable > 0) {
                    if (ReadFromPipe(stdoutRead_, buffer, bytesRead,
                                     "Stdout")) {
                        ProcessOutput(buffer, bytesRead, "Stdout");
                    }
                }
                // Also check stderr
                if (PeekNamedPipe(stderrRead_, nullptr, 0, nullptr,
                                  &stderrBytesAvailable, nullptr) &&
                    stderrBytesAvailable > 0) {
                    if (ReadFromPipe(stderrRead_, buffer, bytesRead,
                                     "Stderr")) {
                        ProcessOutput(buffer, bytesRead, "Stderr");
                    }
                }
                break;

            case WAIT_OBJECT_0 + 2:  // Stderr ready
                if (PeekNamedPipe(stderrRead_, nullptr, 0, nullptr,
                                  &stderrBytesAvailable, nullptr) &&
                    stderrBytesAvailable > 0) {
                    if (ReadFromPipe(stderrRead_, buffer, bytesRead,
                                     "Stderr")) {
                        ProcessOutput(buffer, bytesRead, "Stderr");
                    }
                }
                // Also check stdout
                if (PeekNamedPipe(stdoutRead_, nullptr, 0, nullptr,
                                  &stdoutBytesAvailable, nullptr) &&
                    stdoutBytesAvailable > 0) {
                    if (ReadFromPipe(stdoutRead_, buffer, bytesRead,
                                     "Stdout")) {
                        ProcessOutput(buffer, bytesRead, "Stdout");
                    }
                }
                break;

            case WAIT_TIMEOUT:
                // Check both pipes on timeout
                if (PeekNamedPipe(stdoutRead_, nullptr, 0, nullptr,
                                  &stdoutBytesAvailable, nullptr) &&
                    stdoutBytesAvailable > 0) {
                    if (ReadFromPipe(stdoutRead_, buffer, bytesRead,
                                     "Stdout")) {
                        ProcessOutput(buffer, bytesRead, "Stdout");
                    }
                }
                if (PeekNamedPipe(stderrRead_, nullptr, 0, nullptr,
                                  &stderrBytesAvailable, nullptr) &&
                    stderrBytesAvailable > 0) {
                    if (ReadFromPipe(stderrRead_, buffer, bytesRead,
                                     "Stderr")) {
                        ProcessOutput(buffer, bytesRead, "Stderr");
                    }
                }
                continue;

            default:
                // Error or other result
                DWORD error = GetLastError();
                if (error == ERROR_BROKEN_PIPE ||
                    error == ERROR_INVALID_HANDLE) {
                    // Normal pipe closure
                } else {
                    error_log("WaitForMultipleObjects error: %lu", error);
                }
                goto exit_loop;
        }
    }

exit_loop:
    info_log("WSL I/O reader thread finished");
}

bool WSLProcess::ReadFromPipe(HANDLE pipeHandle, std::vector<uint8_t>& buffer,
                              DWORD& bytesRead, const char* pipeName) {
    if (pipeHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    BOOL result =
        ReadFile(pipeHandle, buffer.data(), BUFFER_SIZE, &bytesRead, nullptr);

    if (!result) {
        DWORD error = GetLastError();
        if (error != ERROR_BROKEN_PIPE && error != ERROR_INVALID_HANDLE) {
            error_log("%s read error: %lu", pipeName, error);
        }
        return false;
    }

    return bytesRead > 0;
}

void WSLProcess::ProcessOutput(const std::vector<uint8_t>& buffer,
                               DWORD bytesRead, const char* source) {
    if (bytesRead == 0) return;

    // Convert output to string
    std::string outputStr(reinterpret_cast<const char*>(buffer.data()),
                          bytesRead);

    // Convert WSL output encoding if needed
    bool is_stderr = (strcmp(source, "Stderr") == 0);
    std::string convertedOutput =
        parallax::utils::ConvertWslOutputToUtf8(outputStr, is_stderr);
    debug_log("WSL original output: %s", outputStr.c_str());
    debug_log("WSL output: %s", convertedOutput.c_str());
    if (convertedOutput.empty()) {
        convertedOutput = outputStr;
    }

    // Output to appropriate stream
    if (is_stderr) {
        std::cerr << convertedOutput << std::flush;
    } else {
        std::cout << convertedOutput << std::flush;
    }
}

BOOL WINAPI WSLProcess::ConsoleCtrlHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            if (s_instance && s_instance->IsRunning()) {
                std::cerr << "\n[Ctrl+C] Stopping WSL process...\n"
                          << std::flush;
                s_instance->Stop();
                return TRUE;  // We handled it
            }
            break;
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            if (s_instance && s_instance->IsRunning()) {
                s_instance->Stop();
                return TRUE;
            }
            break;
    }
    return FALSE;  // Let default handler process it
}