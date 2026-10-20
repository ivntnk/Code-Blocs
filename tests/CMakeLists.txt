find_package(Qt6 REQUIRED COMPONENTS Test)

# blocs_add_test(<nom>) : compile tests/<nom>.cpp, le lie à blocs_core et l'enregistre dans CTest.
function(blocs_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE blocs_core Qt6::Test)
    blocs_enable_warnings(${name})
    blocs_enable_sanitizers(${name})
    add_test(NAME ${name} COMMAND ${name})
    # Pas d'écran sur un serveur d'intégration continue : rendu « offscreen ».
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

blocs_add_test(test_history)
blocs_add_test(test_graph)
blocs_add_test(test_editor)
blocs_add_test(test_codegen)
blocs_add_test(test_exporter)
blocs_add_test(test_explorer)
blocs_add_test(test_tutorials)
