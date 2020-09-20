#
# InnoSetup Installer CMakeLists.txt
#
add_custom_target(create_installer
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating InnoSetup Installer"
    COMMAND ISCC.exe .\\RMG.iss /DMyAppDir="${CMAKE_SOURCE_DIR}\\Bin\\${CMAKE_BUILD_TYPE}\\rmg" /DMyAppVer="${GIT_VERSION}" /DMyOutDir="${CMAKE_SOURCE_DIR}\\Bin" /DMySrcDir="${CMAKE_SOURCE_DIR}"
)
