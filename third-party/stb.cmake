include(ExternalProject)

set(STB_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/stb-prefix/src/stb")

set(STB_INCLUDE_DIR "${STB_INCLUDE_DIR}" PARENT_SCOPE)

ExternalProject_Add(stb
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_TAG master
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        )
