#
# Rosalie's Mupen GUI CMakeLists.txt
#
cmake_minimum_required(VERSION 3.15)

project(RMG)

set(CMAKE_INSTALL_PREFIX "")

add_subdirectory(Source/RMG)
install(TARGETS RMG
    DESTINATION rmg
)

add_subdirectory(External)
install(FILES ${MUPEN64PLUSCORE_LIB}
    DESTINATION rmg/Core/
)
install(FILES ${MUPEN64PLUSCORE_INI} ${MUPEN64PLUSCORE_CHT}
    DESTINATION rmg/Data
)
install(FILES ${MUPEN64PLUS_PLUGIN_AUDIO}
    DESTINATION rmg/Plugin/Audio
)
install(FILES ${MUPEN64PLUS_PLUGIN_RSP}
    DESTINATION rmg/Plugin/RSP
)
install(FILES ${MUPEN64PLUS_PLUGIN_INPUT}
    DESTINATION rmg/Plugin/Input
)
install(FILES ${MUPEN64PLUS_PLUGIN_GFX}
    DESTINATION rmg/Plugin/GFX
)
