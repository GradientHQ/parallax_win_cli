# MSVC runtime library
# set MT/MTD, if MD/MDD is needed, notes this
set(CompilerFlags
    CMAKE_CXX_FLAGS
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_MINSIZEREL
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_C_FLAGS
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_MINSIZEREL
    CMAKE_C_FLAGS_RELWITHDEBINFO
)

foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
    message("MSVC flags: ${CompilerFlag}:${${CompilerFlag}}")
endforeach()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
    "_DEBUG;"
    "DEBUG;"
    "XP_WIN;"
    ">"
    "$<$<CONFIG:Release>:"
    "NDEBUG;"
    "XP_WIN;"
    ">"
    "WIN32;"
    "_WINSOCK_DEPRECATED_NO_WARNINGS;"
    "_CONSOLE;"
    "_CRT_SECURE_NO_WARNINGS;"
    "_CRT_NONSTDC_NO_WARNINGS;"
    "UNICODE;"
    "_UNICODE;"
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
    /Gm-;
    /WX-;
    /Od;
    >
    $<$<CONFIG:Release>:
    /Oi;
    /Gy;
    /WX-;
    /O2;
    /GL;
    >
    /MP;

    # /sdl;
    /W4;
    ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
    /wd4819;
    /wd4003;
    ${DEFAULT_CXX_EXCEPTION_HANDLING}
    /Brepro;
    /experimental:deterministic
)