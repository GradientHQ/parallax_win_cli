if(UNIX)
    # 没有使用target_link_options，该语法在cmake 3.13引入
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS
        "-static-libstdc++ -static-libgcc -Wl,--gc-sections -Wl,--no-undefined"
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        get_target_property(LINK_VALUES ${PROJECT_NAME} LINK_FLAGS)
        set(LINK_VALUES "${LINK_VALUES} -flto")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LINK_VALUES}")
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        "dl"
        "pthread"
        "rt"
    )
elseif(WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
        /DEBUG;
        >
        $<$<CONFIG:Release>:
        /DEBUG;
        /OPT:REF;
        /OPT:ICF;

        # /LTCG; # 可能影响加壳，先屏蔽
        >
        /SUBSYSTEM:CONSOLE,5.02
        /PDBALTPATH:$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>
        /BREPRO
        /INCREMENTAL:NO
        /experimental:deterministic
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        "psapi;"
        "version;"
        "ws2_32;"
        "IPHLPAPI;"
        "Crypt32;"
    )
endif()