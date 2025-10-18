#include "cli/command_parser.h"
#include "tinylog/tinylog.h"
#include "utils/utils.h"
#include <iostream>
#include <windows.h>

int main(int argc, char* argv[]) {
    // Set console output to UTF-8
    SetConsoleOutputCP(CP_UTF8);

    // Build log file path (under exe running path)
    std::string log_path = parallax::utils::JoinPath(
        parallax::utils::GetAppBinDir(), "parallax.log");

    // Initialize logging system
    tinylog_init(log_path.c_str(), 1024 * 1024 * 10, 5, 0,
                 1);  // 10MB, 5 files, no console output, synchronous write

    // Build argument string
    std::string args_str =
        "Parallax started with " + std::to_string(argc) + " arguments: ";
    for (int i = 0; i < argc; ++i) {
        if (i > 0) args_str += " ";
        args_str += argv[i];
    }
    info_log("%s", args_str.c_str());

    try {
        parallax::cli::CommandParser parser;
        return parser.Parse(argc, argv);
    } catch (const std::exception& e) {
        error_log("Unhandled exception: %s", e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        error_log("Unknown exception occurred");
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
