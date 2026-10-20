# Réglages communs : standard C++, type de build par défaut, Qt (moc/rcc), compile_commands.json.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Compilation dans les sources interdite : utilise un dossier build/ (voir README).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Pour clang-tidy, cppcheck, clangd, Qt Creator…
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_property(_blocs_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _blocs_multi AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Type de build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
