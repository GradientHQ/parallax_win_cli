if(MACRO_HAS_FILENAMES)
    ADD_DEFINITIONS(-DMACRO_FILE=__FILE__ -DMACRO_FUNCTION=__FUNCTION__ -DMACRO_LINE=__LINE__)
else()
    ADD_DEFINITIONS(-DMACRO_FILE="" -DMACRO_FUNCTION="" -DMACRO_LINE=0)
endif()

if(NOT DEFINED TARGET_TYPE)
    get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
endif()

# Output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)

set(WINDOWS_SDK_PATHS
    "C:/Program Files (x86)/Windows Kits/10"
    "C:/Program Files/Windows Kits/10"
    "D:/Windows Kits/10"
)

find_program(CMAKE_C_COMPILER NAMES cl)

if(CMAKE_C_COMPILER)
    get_filename_component(VS_PATH "${CMAKE_C_COMPILER}" DIRECTORY)
    file(GLOB WIN_SDK_DIRS "${VS_PATH}/../../Windows Kits/10/Include/*")

    if(WIN_SDK_DIRS)
        get_filename_component(WIN_SDK_PATH "${WIN_SDK_DIRS}" DIRECTORY)
        list(APPEND WINDOWS_SDK_PATHS "${WIN_SDK_PATH}")
    endif()
endif()

set(WINDOWS_SDK_PATH "")

foreach(path ${WINDOWS_SDK_PATHS})
    if(EXISTS "${path}/Include")
        set(WINDOWS_SDK_PATH "${path}")
        break()
    endif()
endforeach()

if(NOT WINDOWS_SDK_PATH)
    message(FATAL_ERROR "Windows SDK not found. Please install Windows SDK 10 or later.")
endif()

message(STATUS "Found Windows SDK at: ${WINDOWS_SDK_PATH}")

file(GLOB sdk_dirs "${WINDOWS_SDK_PATH}/Include/*")

set(sdk_versions "")

foreach(dir ${sdk_dirs})
    get_filename_component(ver ${dir} NAME)

    if(ver MATCHES "^10\\.0\\.[0-9]+\\.[0-9]+$")
        if(ver VERSION_GREATER_EQUAL "10.0.17134.0")
            list(APPEND sdk_versions ${ver})
        endif()
    endif()
endforeach()

if(sdk_versions)
    list(SORT sdk_versions)
    list(GET sdk_versions 0 WINDOWS_SDK_VERSION)

    message(STATUS "Selected Windows SDK version: ${WINDOWS_SDK_VERSION}")

    set(CMAKE_SYSTEM_VERSION ${WINDOWS_SDK_VERSION})
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION ${WINDOWS_SDK_VERSION})

    include_directories("${WINDOWS_SDK_PATH}/Include/${WINDOWS_SDK_VERSION}/um")
    include_directories("${WINDOWS_SDK_PATH}/Include/${WINDOWS_SDK_VERSION}/shared")
    include_directories("${WINDOWS_SDK_PATH}/Include/${WINDOWS_SDK_VERSION}/ucrt")

else()
    message(FATAL_ERROR "No suitable Windows SDK version >= 10.0.17134.0 was found.")
endif()

if(FOLDER_NAME)
    set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${FOLDER_NAME})
endif()

# Global c++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
