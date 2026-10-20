# blocs_enable_warnings(<cible>) : avertissements du compilateur.
# Les en-têtes Qt sont inclus en « SYSTEM » par les cibles importées : ils ne polluent pas la sortie.

function(blocs_enable_warnings target)
    if(MSVC)
        set(flags /W4 /permissive- /utf-8 /Zc:__cplusplus)
        if(BLOCS_WARNINGS_AS_ERRORS)
            list(APPEND flags /WX)
        endif()
    else()
        set(flags
            -Wall -Wextra -Wpedantic
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wcast-align
            -Wunused
            -Wformat=2
            -Wmisleading-indentation)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            list(APPEND flags -Wduplicated-cond -Wlogical-op)
        endif()
        if(BLOCS_STRICT_WARNINGS)
            # -Wshadow : signale p. ex. graph.h:56 (paramètre « id » = membre) et exporter.cpp:159.
            list(APPEND flags -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wuseless-cast)
        endif()
        if(BLOCS_WARNINGS_AS_ERRORS)
            list(APPEND flags -Werror)
        endif()
    endif()
    target_compile_options(${target} PRIVATE ${flags})
endfunction()

# blocs_enable_sanitizers(<cible>) : AddressSanitizer + UBSan (pas disponibles avec MinGW).
function(blocs_enable_sanitizers target)
    if(NOT BLOCS_ENABLE_SANITIZERS)
        return()
    endif()
    if(MSVC OR MINGW)
        message(WARNING "BLOCS_ENABLE_SANITIZERS ignoré avec ce compilateur (${CMAKE_CXX_COMPILER_ID}, MinGW/MSVC).")
        return()
    endif()
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()
