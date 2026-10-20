include("C:/Users/ivann/Downloads/Blocs/build/mingw-debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Blocs-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Users/ivann/Downloads/Blocs/build/mingw-debug/Blocs.exe"
    GENERATE_QT_CONF
    NO_TRANSLATIONS
)
