cmake_minimum_required(VERSION 3.21)

project(Blocs
    VERSION 1.0.0
    DESCRIPTION "Programmation visuelle par blocs qui génère du C#"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# ---------------------------------------------------------------------------
#  Options
# ---------------------------------------------------------------------------
option(BLOCS_BUILD_TESTS        "Compiler les tests (CTest + Qt Test)"                 ON)
option(BLOCS_WIN_ICON           "Intégrer l'icône et les infos de version (Windows)"   ON)
option(BLOCS_DEPLOY_QT          "Copier les DLL/plugins Qt à l'installation"           ON)
option(BLOCS_STRICT_WARNINGS    "Avertissements supplémentaires (conversions, casts)"  OFF)
option(BLOCS_WARNINGS_AS_ERRORS "Traiter les avertissements comme des erreurs"         OFF)
option(BLOCS_ENABLE_CLANG_TIDY  "Lancer clang-tidy à chaque compilation"               OFF)
option(BLOCS_ENABLE_CPPCHECK    "Lancer cppcheck à chaque compilation"                 OFF)
option(BLOCS_ENABLE_SANITIZERS  "ASan + UBSan (GCC/Clang, hors MinGW)"                 OFF)
option(BLOCS_ENABLE_LTO         "Optimisation à l'édition de liens (Release)"          OFF)

include(ProjectSettings)
include(CompilerWarnings)
include(StaticAnalysis)

# Sous Windows on installe « à plat » (Blocs.exe + DLL au même endroit),
# comme le faisait scripts/deploy.bat. Ailleurs : disposition GNU habituelle.
if(WIN32)
    set(CMAKE_INSTALL_BINDIR ".")
endif()
include(GNUInstallDirs)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)

# ---------------------------------------------------------------------------
#  Bibliothèque « blocs_core » : tout le code sauf main.cpp
#  (permet de la tester sans lancer l'application)
# ---------------------------------------------------------------------------
set(BLOCS_CORE_SOURCES
    src/graph.h src/graph.cpp
    src/editor.h src/editor.cpp
    src/codegen.h src/codegen.cpp
    src/runner.h src/runner.cpp
    src/gamewindow.h src/gamewindow.cpp
    src/exporter.h src/exporter.cpp
    src/assetsdialog.h src/assetsdialog.cpp
    src/explorer.h src/explorer.cpp
    src/history.h
    src/examples.h src/examples.cpp
    src/highlighter.h
    src/tutorials.h src/tutorials.cpp
    src/tutorialdialog.h src/tutorialdialog.cpp
    src/mainwindow.h src/mainwindow.cpp
)

add_library(blocs_core STATIC ${BLOCS_CORE_SOURCES})
add_library(Blocs::core ALIAS blocs_core)
target_include_directories(blocs_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(blocs_core PUBLIC Qt6::Widgets)
if(WIN32)
    target_link_libraries(blocs_core PUBLIC winmm)   # sons WAV de la fenêtre de jeu
endif()
target_compile_definitions(blocs_core PUBLIC BLOCS_VERSION="${PROJECT_VERSION}")
blocs_enable_warnings(blocs_core)
blocs_enable_static_analysis(blocs_core)
blocs_enable_sanitizers(blocs_core)

# ---------------------------------------------------------------------------
#  Exécutable
# ---------------------------------------------------------------------------
set(BLOCS_COMPANY   "Blocs"               CACHE STRING "Éditeur (propriétés du .exe, installateur)")
set(BLOCS_COPYRIGHT "Copyright (C) Blocs" CACHE STRING "Mention de copyright du .exe (ASCII)")
set(BLOCS_APP_SOURCES src/main.cpp resources/resources.qrc)

# Icône + « Propriétés > Détails » du .exe sous Windows
if(WIN32 AND BLOCS_WIN_ICON)
    enable_language(RC)
    configure_file("${PROJECT_SOURCE_DIR}/cmake/version.rc.in"
                   "${PROJECT_BINARY_DIR}/blocs_version.rc" @ONLY)
    list(APPEND BLOCS_APP_SOURCES resources/blocs.rc "${PROJECT_BINARY_DIR}/blocs_version.rc")
endif()

qt_add_executable(Blocs ${BLOCS_APP_SOURCES})
target_link_libraries(Blocs PRIVATE blocs_core)
blocs_enable_warnings(Blocs)
blocs_enable_sanitizers(Blocs)

# Pas de fenêtre console noire derrière l'application sous Windows
set_target_properties(Blocs PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

if(BLOCS_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _blocs_ipo OUTPUT _blocs_ipo_msg)
    if(_blocs_ipo)
        set_property(TARGET blocs_core Blocs PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    else()
        message(WARNING "LTO non supporté : ${_blocs_ipo_msg}")
    endif()
endif()

# ---------------------------------------------------------------------------
#  Tests
# ---------------------------------------------------------------------------
if(BLOCS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
#  Outils de développement : format, format-check, tidy, cppcheck
# ---------------------------------------------------------------------------
include(DevTools)

# ---------------------------------------------------------------------------
#  Installation + empaquetage  (cmake --install  /  cpack)
# ---------------------------------------------------------------------------
install(TARGETS Blocs
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE  DESTINATION .)

if(WIN32)
    install(FILES README.md CHANGELOG.md DESTINATION .)
else()
    install(FILES README.md CHANGELOG.md DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif()

if(BLOCS_DEPLOY_QT AND (WIN32 OR APPLE) AND COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
        TARGET Blocs
        OUTPUT_SCRIPT blocs_deploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR)
    install(SCRIPT "${blocs_deploy_script}")
endif()

include(Packaging)
