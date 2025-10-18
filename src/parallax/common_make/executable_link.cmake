target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
    /DEBUG;
    >
    $<$<CONFIG:Release>:
    /DEBUG;
    /OPT:REF;
    /OPT:ICF;
    /LTCG;
    >
    /PDBALTPATH:$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>
    /SUBSYSTEM:CONSOLE,5.02
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