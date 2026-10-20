# Cibles utilitaires (lancées à la demande, jamais pendant un build normal) :
#   cmake --build <build> --target format        reformate src/ et tests/ SUR PLACE
#   cmake --build <build> --target format-check  vérifie le format sans rien modifier
#   cmake --build <build> --target tidy          clang-tidy sur src/ et tests/ (lecture seule)
#   cmake --build <build> --target cppcheck      cppcheck sur src/ et tests/ (lecture seule)

file(GLOB_RECURSE BLOCS_FORMAT_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/tests/*.cpp" "${PROJECT_SOURCE_DIR}/tests/*.h")
file(GLOB_RECURSE BLOCS_ANALYSIS_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/tests/*.cpp")

find_program(BLOCS_CLANG_FORMAT_EXE NAMES clang-format)
if(BLOCS_CLANG_FORMAT_EXE)
    add_custom_target(format
        COMMAND "${BLOCS_CLANG_FORMAT_EXE}" -i ${BLOCS_FORMAT_FILES}
        COMMENT "clang-format : reformatage de src/ et tests/ (modifie les fichiers)"
        VERBATIM)
    add_custom_target(format-check
        COMMAND "${BLOCS_CLANG_FORMAT_EXE}" --dry-run --Werror ${BLOCS_FORMAT_FILES}
        COMMENT "clang-format : vérification du format"
        VERBATIM)
endif()

find_program(BLOCS_CLANG_TIDY_EXE NAMES clang-tidy)
if(BLOCS_CLANG_TIDY_EXE)
    add_custom_target(tidy
        COMMAND "${BLOCS_CLANG_TIDY_EXE}" -p "${PROJECT_BINARY_DIR}" --quiet ${BLOCS_ANALYSIS_FILES}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "clang-tidy : analyse statique"
        VERBATIM)
endif()

find_program(BLOCS_CPPCHECK_EXE NAMES cppcheck)
if(BLOCS_CPPCHECK_EXE)
    add_custom_target(cppcheck
        COMMAND "${BLOCS_CPPCHECK_EXE}" --std=c++17 --library=qt
                --enable=warning,style,performance,portability --inline-suppr
                "--suppressions-list=${PROJECT_SOURCE_DIR}/cppcheck-suppressions.txt"
                "-I${PROJECT_SOURCE_DIR}/src" --quiet ${BLOCS_ANALYSIS_FILES}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "cppcheck : analyse statique"
        VERBATIM)
endif()
