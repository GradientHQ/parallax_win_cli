#include "tinylog.h"
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <string>
#include <iostream>
#include <atomic>

// Log level strings
static const char* const priorities[] = {"CRIT", "ERROR", "WARN",
                                         "INFO", "DEBUG", "TRACE"};

// Global variables
static char* g_filename = nullptr;
static FILE* g_file = nullptr;
static int g_log_max_level = 3;                 // Default INFO level
static int g_console_output = 1;                // Default output to console
static int g_sync_write = 1;                    // Default synchronous write
static int g_quiet = 0;                         // Default not quiet
static int g_max_file_size = 10 * 1024 * 1024;  // Default 10MB
static int g_max_files = 5;                     // Default 5 files
static std::mutex g_log_mutex;                  // Log mutex
static bool g_initialized = false;
static std::atomic<int> g_log_index{1};  // Log sequence number

// Get current time string
static void get_time_string(char* buffer, size_t buffer_size) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
             st.wMilliseconds);
}

// Get file size
static long get_file_size(FILE* file) {
    if (!file) return 0;

    long current_pos = ftell(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, current_pos, SEEK_SET);
    return size;
}

// Rotate log files
static void rotate_log_files() {
    if (!g_filename) return;

    // Close current file
    if (g_file) {
        fclose(g_file);
        g_file = nullptr;
    }

    // Rotate files (log.4 -> log.5, log.3 -> log.4, ..., log.1 -> log.2)
    for (int i = g_max_files - 1; i > 0; i--) {
        char old_name[512], new_name[512];
        snprintf(old_name, sizeof(old_name), "%s.%d", g_filename, i);
        snprintf(new_name, sizeof(new_name), "%s.%d", g_filename, i + 1);

        // Delete the last file
        if (i == g_max_files - 1) {
            DeleteFileA(new_name);
        }

        // Rename file
        MoveFileA(old_name, new_name);
    }

    // Rename current log file to .1
    char backup_name[512];
    snprintf(backup_name, sizeof(backup_name), "%s.1", g_filename);
    MoveFileA(g_filename, backup_name);

    // Reopen new log file
    g_file = fopen(g_filename, "w");
}

// Write log to file
static void write_to_file(const char* log_message) {
    if (!g_file || !log_message) return;

    // Check file size, rotate if exceeds limit
    if (get_file_size(g_file) > g_max_file_size) {
        rotate_log_files();
    }

    if (g_file) {
        fputs(log_message, g_file);
        if (g_sync_write) {
            fflush(g_file);
        }
    }
}

// Initialize log system
int tinylog_init(const char* filename, int max_file_size, int max_files,
                 int console_output, int sync_write) {
    std::lock_guard<std::mutex> lock(g_log_mutex);

    if (g_initialized) {
        tinylog_uninit();
    }

    g_max_file_size = max_file_size;
    g_max_files = max_files;
    g_console_output = console_output;
    g_sync_write = sync_write;

    if (filename) {
        size_t len = strlen(filename) + 1;
        g_filename = (char*)malloc(len);
        if (g_filename) {
            strcpy_s(g_filename, len, filename);
            g_file = fopen(g_filename, "a");
        }
    }

    g_initialized = true;
    return 0;
}

// Uninitialize log system
void tinylog_uninit() {
    std::lock_guard<std::mutex> lock(g_log_mutex);

    if (g_file) {
        fclose(g_file);
        g_file = nullptr;
    }

    if (g_filename) {
        free(g_filename);
        g_filename = nullptr;
    }

    g_initialized = false;
}

// Set log level
void set_log_level(int log_level) { g_log_max_level = log_level; }

int get_log_level() { return g_log_max_level; }

// Set quiet mode
void set_log_quiet(int quiet) { g_quiet = quiet; }

int get_log_quiet() { return g_quiet; }

// Core log function
void sys_log(int id, int a_priority, const char* file, const int line,
             const char* func, const char* a_format, ...) {
    va_list va;
    va_start(va, a_format);
    sys_logv(id, a_priority, file, line, func, a_format, va);
    va_end(va);
}

void sys_logv(int id, int a_priority, const char* file, const int line,
              const char* func, const char* a_format, va_list va) {
    (void)id;  // Unused parameter

    // Check log level
    if (a_priority > g_log_max_level || g_quiet) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_log_mutex);

    // Get time
    char time_str[64];
    get_time_string(time_str, sizeof(time_str));

    // Format user message
    char user_message[2048];
    vsnprintf(user_message, sizeof(user_message), a_format, va);

    // Get process ID, thread ID and log sequence number
    int pid = GetCurrentProcessId();
    int tid = GetCurrentThreadId();
    int log_idx = g_log_index.fetch_add(1);
    if (log_idx > 500000) {
        g_log_index = 1;
        log_idx = 1;
    }

    // Assemble complete log message (completely following gradient project
    // format)
    char log_message[4096];
    snprintf(log_message, sizeof(log_message), "[%d-%d:%d] %s [%s] - %s\n", pid,
             tid, log_idx, time_str,
             (a_priority >= 0 && a_priority < 6) ? priorities[a_priority]
                                                 : "UNKNOWN",
             user_message);

    // Output to console
    if (g_console_output) {
        // Use different colors based on log level
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD color = FOREGROUND_RED | FOREGROUND_GREEN |
                     FOREGROUND_BLUE;  // Default white

        switch (a_priority) {
            case 0:  // CRIT - red background
                color = FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_RED;
                break;
            case 1:  // ERROR - red
                color = FOREGROUND_RED | FOREGROUND_INTENSITY;
                break;
            case 2:  // WARN - yellow
                color =
                    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case 3:  // INFO - white
                color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                break;
            case 4:  // DEBUG - gray
                color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
                        FOREGROUND_INTENSITY;
                break;
        }

        SetConsoleTextAttribute(hConsole, color);
        fputs(log_message, stdout);
        SetConsoleTextAttribute(
            hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        fflush(stdout);
    }

    // Write to file
    if (g_initialized) {
        write_to_file(log_message);
    }
}
