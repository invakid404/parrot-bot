include(ExternalProject)

find_program(MAKE_EXECUTABLE
        NAMES gmake mingw32-make make
        NAMES_PER_DIR
        DOC "GNU Make"
        )

find_program(AUTOCONF_EXECUTABLE
        NAMES autoconf
        NAMES_PER_DIR
        DOC "Autoconf"
        )

set(CHAN_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/chan-prefix")
set(CHAN_LIBRARY "${CHAN_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}chan${CMAKE_STATIC_LIBRARY_SUFFIX}")

set(CHAN_INSTALL_DIR "${CHAN_INSTALL_DIR}" PARENT_SCOPE)
set(CHAN_LIBRARY "${CHAN_LIBRARY}" PARENT_SCOPE)

ExternalProject_Add(chan
        GIT_REPOSITORY https://github.com/tylertreat/chan.git
        GIT_TAG master
        UPDATE_DISCONNECTED true
        CONFIGURE_COMMAND bash -c "pushd <SOURCE_DIR> && bash ./autogen.sh && ./configure --prefix <INSTALL_DIR> && popd"
        BUILD_COMMAND ${MAKE_EXECUTABLE} -j -C <SOURCE_DIR>
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env PREFIX=<INSTALL_DIR> ${MAKE_EXECUTABLE} -j -C <SOURCE_DIR> install
        BUILD_BYPRODUCTS ${CHAN_LIBRARY}
        )
