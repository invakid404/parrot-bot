cmake_minimum_required(VERSION 3.19)

project(parrot_bot)

find_package(Threads)
add_subdirectory(third-party)

add_executable(parrot_bot main.c database.c)

set_property(TARGET parrot_bot PROPERTY C_STANDARD 23)

add_dependencies(parrot_bot concord)

target_include_directories(parrot_bot PRIVATE ${CONCORD_INSTALL_DIR}/include)
target_link_libraries(parrot_bot PRIVATE ${CONCORD_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} curl leveldb)
