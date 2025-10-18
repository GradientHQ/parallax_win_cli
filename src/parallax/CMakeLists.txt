cmake_minimum_required(VERSION 3.6.0)

set(PROJECT_NAME parallax)

project(${PROJECT_NAME} C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/wd4819)
    add_compile_options(/wd4996)
    add_compile_options(/wd4127)
    add_compile_options(/wd4267)
    add_compile_options(/wd4244)
    add_compile_options(/wd4834)
    add_compile_options(/wd4477)
    add_compile_options(/wd4100)
    add_compile_options(/wd4101)
    add_compile_options(/wd4456)
    add_compile_options(/wd4457)
    add_compile_options(/wd4505)
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-D_WIN32_WINNT=0x0601 -DWINVER=0x0601)
endif()

# Main program files
set(MAIN_FILES
    main.cpp
)

# Command line parsing module
set(CLI_FILES
    cli/command_parser.cpp
    cli/command_parser.h
)

# CLI commands - Base
set(CLI_COMMANDS_BASE_FILES
    cli/commands/base_command.h
)

# CLI commands - Environment
set(CLI_COMMANDS_ENV_FILES
    cli/commands/check_command.cpp
    cli/commands/check_command.h
    cli/commands/install_command.cpp
    cli/commands/install_command.h
)

# CLI commands - Configuration
set(CLI_COMMANDS_CONFIG_FILES
    cli/commands/config_command.cpp
    cli/commands/config_command.h
)

# CLI commands - Model & Utility
set(CLI_COMMANDS_UTIL_FILES
    cli/commands/model_commands.cpp
    cli/commands/model_commands.h
    cli/commands/cmd_command.cpp
    cli/commands/cmd_command.h
)

# Configuration management module
set(CONFIG_FILES
    config/config_manager.cpp
    config/config_manager.h
)

# Logging module
set(TINYLOG_FILES
    tinylog/tinylog.cpp
    tinylog/tinylog.h
)

# Utility module
set(UTILS_FILES
    utils/utils.cpp
    utils/utils.h
    utils/process.cpp
    utils/process.h
    utils/wsl_process.cpp
    utils/wsl_process.h
)

# Environment main controller
set(ENVIRONMENT_MAIN_FILES
    environment/environment_installer.cpp
    environment/environment_installer.h
)

# Environment base infrastructure
set(ENVIRONMENT_BASE_FILES
    environment/base_component.cpp
    environment/base_component.h
)

# Environment command executor
set(ENVIRONMENT_EXECUTOR_FILES
    environment/command_executor.cpp
    environment/command_executor.h
)

# Environment system checkers
set(ENVIRONMENT_SYSTEM_FILES
    environment/system_checker.cpp
    environment/system_checker.h
)

# Environment Windows feature managers - Part 1
set(ENVIRONMENT_WINDOWS_PART1_FILES
    environment/windows_feature_manager.cpp
    environment/windows_feature_manager.h
)

# Environment Windows feature managers - Part 2
set(ENVIRONMENT_WINDOWS_PART2_FILES
    environment/windows_feature_manager2.cpp
)

# Environment software installers - Part 1
set(ENVIRONMENT_SOFTWARE_PART1_FILES
    environment/software_installer.cpp
    environment/software_installer.h
)

# Environment software installers - Part 2
set(ENVIRONMENT_SOFTWARE_PART2_FILES
    environment/software_installer2.cpp
)

set(ALL_FILES
    ${MAIN_FILES}
    ${CLI_FILES}
    ${CLI_COMMANDS_BASE_FILES}
    ${CLI_COMMANDS_ENV_FILES}
    ${CLI_COMMANDS_CONFIG_FILES}
    ${CLI_COMMANDS_UTIL_FILES}
    ${CONFIG_FILES}
    ${TINYLOG_FILES}
    ${UTILS_FILES}
    ${ENVIRONMENT_MAIN_FILES}
    ${ENVIRONMENT_BASE_FILES}
    ${ENVIRONMENT_EXECUTOR_FILES}
    ${ENVIRONMENT_SYSTEM_FILES}
    ${ENVIRONMENT_WINDOWS_PART1_FILES}
    ${ENVIRONMENT_WINDOWS_PART2_FILES}
    ${ENVIRONMENT_SOFTWARE_PART1_FILES}
    ${ENVIRONMENT_SOFTWARE_PART2_FILES}
)

# Create Visual Studio folder structure
source_group("" FILES ${MAIN_FILES})
source_group("cli" FILES ${CLI_FILES})
source_group("cli\\commands\\base" FILES ${CLI_COMMANDS_BASE_FILES})
source_group("cli\\commands\\environment" FILES ${CLI_COMMANDS_ENV_FILES})
source_group("cli\\commands\\config" FILES ${CLI_COMMANDS_CONFIG_FILES})
source_group("cli\\commands\\utility" FILES ${CLI_COMMANDS_UTIL_FILES})
source_group("config" FILES ${CONFIG_FILES})
source_group("tinylog" FILES ${TINYLOG_FILES})
source_group("utils" FILES ${UTILS_FILES})
source_group("environment" FILES ${ENVIRONMENT_MAIN_FILES})
source_group("environment\\base" FILES ${ENVIRONMENT_BASE_FILES})
source_group("environment\\executor" FILES ${ENVIRONMENT_EXECUTOR_FILES})
source_group("environment\\system" FILES ${ENVIRONMENT_SYSTEM_FILES})
source_group("environment\\windows\\core" FILES ${ENVIRONMENT_WINDOWS_PART1_FILES})
source_group("environment\\windows\\extended" FILES ${ENVIRONMENT_WINDOWS_PART2_FILES})
source_group("environment\\software\\core" FILES ${ENVIRONMENT_SOFTWARE_PART1_FILES})
source_group("environment\\software\\extended" FILES ${ENVIRONMENT_SOFTWARE_PART2_FILES})

add_executable(${PROJECT_NAME} ${ALL_FILES})

include(common_make/properties.cmake)
include(common_make/executable_compile.cmake)
include(common_make/executable_link.cmake)

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "./"
)

# Link system libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    "advapi32"
    "shell32"
    "ntdll"
    "wininet"
)