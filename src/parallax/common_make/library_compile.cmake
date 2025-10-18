if(NOT DEFINED TARGET_TYPE)
    get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
endif()

message("target ${PROJECT_NAME} is ${TARGET_TYPE}")

if(UNIX)
    target_compile_options(${PROJECT_NAME} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:"
        "-fPIC"
        "-pthread"
        "-ffunction-sections"
        "-fdata-sections"
        "-fstack-protector-all"
        "-fno-common"
        "-Wall"
        "-std=c++1y"
        "-fvisibility=hidden"
        ">"
    )
    target_compile_options(${PROJECT_NAME} PRIVATE
        "$<$<COMPILE_LANGUAGE:C>:"
        "-fPIC"
        "-pthread"
        "-ffunction-sections"
        "-fdata-sections"
        "-fstack-protector-all"
        "-fno-common"
        "-Wall"
        "-std=gnu11"
        "-fgnu89-inline"
        ">"
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<COMPILE_LANGUAGE:CXX>:"
            "-flto"
            ">"
        )
        target_compile_options(${PROJECT_NAME} PRIVATE
            "$<$<COMPILE_LANGUAGE:C>:"
            "-flto"
            ">"
        )
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
        "_DEBUG"
        "DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
        "NDEBUG"
        ">"
        "RAPIDJSON_HAS_STDSTRING"
    )
elseif(WIN32)
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

        # /GL;  # 可能影响加壳，先屏蔽
        >
        /MP;

        # /sdl;
        /W4;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        /wd4819; # 警告指示源文件包含了不能在当前代码页中表示的字符
        /wd4003; # 警告指示在 __LINE__ 和 __FILE__ 宏之间存在多行的 C 预处理器指令
        /wd4002; # 警告指示编译器发现了一个函数的返回值没有被使用。这可能是一个潜在的错误，因为函数返回值没有被使用可能是不正确的行为
        /wd4996; # 警告指示编译器发现了对被标记为不安全的函数的调用，这些函数通常被视为不推荐使用，因为它们可能存在安全性问题或者被更加安全的替代函数所取代
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
        /Brepro;
        /experimental:deterministic
    )
endif()