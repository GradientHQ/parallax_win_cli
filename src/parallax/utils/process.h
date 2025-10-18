#pragma once
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

// Simplified process module, specifically for parallax project

namespace parallax {
namespace utils {

/**
 * Execute command line and get output result (synchronous version)
 *
 * @param cmd Command to execute
 * @param timeout Timeout in seconds
 * @param stdout_output Standard output content
 * @param stderr_output Standard error output content
 * @param elevate Whether to run with elevated privileges (parameter kept but
 * not used)
 * @param skip_encoding_conversion Whether to skip encoding conversion
 * (optional)
 * @return Command execution return code, <0 indicates execution failure
 */
int ExecCommandEx(const std::string& cmd, int timeout,
                  std::string& stdout_output, std::string& stderr_output,
                  bool elevate = false, bool skip_encoding_conversion = false);

/**
 * Execute command line and support callback to check if process should be
 * terminated
 *
 * @param cmd Command to execute
 * @param timeout Timeout in seconds
 * @param stdout_output Standard output content
 * @param stderr_output Standard error output content
 * @param check_callback Check callback function, returns true if process should
 * be terminated
 * @param elevate Whether to run with elevated privileges (parameter kept but
 * not used)
 * @param skip_encoding_conversion Whether to skip encoding conversion
 * @return Command execution return code, <0 indicates execution failure, -3
 * indicates terminated by callback
 */
int ExecCommandEx2(const std::string& cmd, int timeout,
                   std::string& stdout_output, std::string& stderr_output,
                   std::function<bool()> check_callback, bool elevate = false,
                   bool skip_encoding_conversion = false);

}  // namespace utils
}  // namespace parallax
