# blocs_enable_static_analysis(<cible>) : clang-tidy / cppcheck pendant la compilation (optionnel).
# La configuration de clang-tidy est dans .clang-tidy, celle de cppcheck dans cppcheck-suppressions.txt.

function(blocs_enable_static_analysis target)
    if(BLOCS_ENABLE_CLANG_TIDY)
        find_program(BLOCS_CLANG_TIDY_EXE NAMES clang-tidy)
        if(BLOCS_CLANG_TIDY_EXE)
            set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${BLOCS_CLANG_TIDY_EXE}")
        else()
            message(WARNING "BLOCS_ENABLE_CLANG_TIDY=ON mais clang-tidy est introuvable.")
        endif()
    endif()

    if(BLOCS_ENABLE_CPPCHECK)
        find_program(BLOCS_CPPCHECK_EXE NAMES cppcheck)
        if(BLOCS_CPPCHECK_EXE)
            set_target_properties(${target} PROPERTIES CXX_CPPCHECK
                "${BLOCS_CPPCHECK_EXE};--std=c++17;--library=qt;--enable=warning,style,performance,portability;--inline-suppr;--suppressions-list=${PROJECT_SOURCE_DIR}/cppcheck-suppressions.txt;--quiet")
        else()
            message(WARNING "BLOCS_ENABLE_CPPCHECK=ON mais cppcheck est introuvable.")
        endif()
    endif()

    # Ne pas analyser le code généré par moc (CMake >= 3.27 ; ignoré avant).
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
        set_source_files_properties(
            "${CMAKE_CURRENT_BINARY_DIR}/${target}_autogen/mocs_compilation.cpp"
            PROPERTIES SKIP_LINTING ON)
    endif()
endfunction()
