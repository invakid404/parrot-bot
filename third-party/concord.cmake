include(ExternalProject)

find_program(MAKE_EXECUTABLE
        NAMES gmake mingw32-make make
        NAMES_PER_DIR
        DOC "GNU Make"
        )

set(CONCORD_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/concord-prefix")
set(CONCORD_LIBRARY "${CONCORD_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}discord${CMAKE_STATIC_LIBRARY_SUFFIX}")

set(CONCORD_INSTALL_DIR "${CONCORD_INSTALL_DIR}" PARENT_SCOPE)
set(CONCORD_LIBRARY "${CONCORD_LIBRARY}" PARENT_SCOPE)

ExternalProject_Add(concord
        GIT_REPOSITORY https://github.com/Cogmasters/concord.git
        GIT_TAG dev
        UPDATE_DISCONNECTED true
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E env CFLAGS="-DCCORD_SIGINTCATCH" ${MAKE_EXECUTABLE} -j -C <SOURCE_DIR>
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env PREFIX=<INSTALL_DIR> ${MAKE_EXECUTABLE} -j -C <SOURCE_DIR> install
        BUILD_BYPRODUCTS ${CONCORD_LIBRARY}
        )