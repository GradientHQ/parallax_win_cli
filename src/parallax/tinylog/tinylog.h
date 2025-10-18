#pragma once

#include <stdio.h>
#include <stdarg.h>

// Simplified tinylog, specifically for parallax project use

#ifndef MACRO_FILE
#define MACRO_FILE __FILE__
#endif
#ifndef MACRO_LINE
#define MACRO_LINE __LINE__
#endif
#ifndef MACRO_FUNCTION
#define MACRO_FUNCTION __FUNCTION__
#endif

#define debug_log(format, ...) \
    sys_log(0, 4, MACRO_FILE, MACRO_LINE, MACRO_FUNCTION, format, ##__VA_ARGS__)
#define info_log(format, ...) \
    sys_log(0, 3, MACRO_FILE, MACRO_LINE, MACRO_FUNCTION, format, ##__VA_ARGS__)
#define warn_log(format, ...) \
    sys_log(0, 2, MACRO_FILE, MACRO_LINE, MACRO_FUNCTION, format, ##__VA_ARGS__)
#define error_log(format, ...) \
    sys_log(0, 1, MACRO_FILE, MACRO_LINE, MACRO_FUNCTION, format, ##__VA_ARGS__)
#define crit_log(format, ...) \
    sys_log(0, 0, MACRO_FILE, MACRO_LINE, MACRO_FUNCTION, format, ##__VA_ARGS__)

// Initialize log system
// filename: Log file name
// max_file_size: Maximum file size (bytes)
// max_files: Maximum number of files
// console_output: Whether to output to console (1=yes, 0=no)
// sync_write: Whether to write synchronously (1=yes, 0=no)
int tinylog_init(const char* filename, int max_file_size, int max_files,
                 int console_output, int sync_write);

// Uninitialize log system
void tinylog_uninit();

// Set log level (0=CRIT, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG)
void set_log_level(int log_level);
int get_log_level();

// Set quiet mode
void set_log_quiet(int quiet);
int get_log_quiet();

// Core log function
void sys_log(int id, int a_priority, const char* file, const int line,
             const char* func, _Printf_format_string_ const char* a_format,
             ...);

void sys_logv(int id, int a_priority, const char* file, const int line,
              const char* func, const char* a_format, va_list va);
