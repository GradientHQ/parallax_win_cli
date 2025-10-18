#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

#include <windows.h>

// WSL process executor with real-time output
class WSLProcess {
 public:
    WSLProcess();
    ~WSLProcess();

    // Execute WSL command with real-time output
    int Execute(const std::string& wsl_command);

    // Stop the running process (for Ctrl+C handling)
    void Stop();

    // Check if process is running
    bool IsRunning() const;

 private:
    // Process management
    bool CreateWSLProcess(const std::string& command);
    void CleanupProcess();

    // I/O thread for real-time output
    void IOReaderThread();

    // Helper functions for I/O
    bool ReadFromPipe(HANDLE pipeHandle, std::vector<uint8_t>& buffer,
                      DWORD& bytesRead, const char* pipeName);
    void ProcessOutput(const std::vector<uint8_t>& buffer, DWORD bytesRead,
                       const char* source);

    // Console control handler for Ctrl+C
    static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);
    static WSLProcess* s_instance;

 private:
    std::atomic<bool> running_;
    std::atomic<bool> shouldStop_;

    // Process handles
    HANDLE processHandle_;
    HANDLE threadHandle_;
    HANDLE stdoutWrite_;
    HANDLE stdoutRead_;
    HANDLE stderrWrite_;
    HANDLE stderrRead_;
    PROCESS_INFORMATION processInfo_;
    STARTUPINFOA startupInfo_;

    // I/O thread
    std::thread ioThread_;
    HANDLE exitEvent_;  // Event handle for graceful shutdown

    // Buffer size
    static const int BUFFER_SIZE = 4096;

    // Exit code
    std::atomic<int> exitCode_;
};