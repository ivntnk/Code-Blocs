# CPack : cpack -C Release  (depuis le dossier de build)
# L'installateur Windows « Setup.exe » se fait avec Inno Setup : voir installer/Blocs.iss.

set(CPACK_PACKAGE_NAME "Blocs")
set(CPACK_PACKAGE_VENDOR "${BLOCS_COMPANY}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_FILE_NAME "Blocs-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
if(EXISTS "${PROJECT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
endif()

if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_IGNORE_FILES "/build/;/dist/;/\\\\.git/;/\\\\.qtcreator/;\\\\.user$")

include(CPack)
