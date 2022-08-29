cmake_minimum_required(VERSION 3.19)

project(parrot_bot)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/third-party")

set(INIT_SQL "")
file(READ init.sql INIT_SQL)

string(REPLACE \n \\n "INIT_SQL" "${INIT_SQL}")
string(REGEX REPLACE "([][\"])" "\\\\\\1" "INIT_SQL" "${INIT_SQL}")

configure_file(init_sql.h.in init_sql.h @ONLY)

find_package(Threads)
find_package(SQLite3)
find_package(PCRE2)
find_package(OpenSSL)

add_subdirectory(third-party)

add_executable(parrot_bot main.c init_sql.h.in vector.c download.c)

set_property(TARGET parrot_bot PROPERTY C_STANDARD 23)

add_dependencies(parrot_bot concord chan)

target_include_directories(parrot_bot PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CONCORD_INSTALL_DIR}/include ${SQLite3_INCLUDE_DIRS} ${PCRE2_INCLUDE_DIRS} ${CHAN_INSTALL_DIR}/include ${OPENSSL_INCLUDE_DIR})
target_link_libraries(parrot_bot PRIVATE ${CONCORD_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} curl ${SQLite3_LIBRARIES} ${PCRE2_LIBRARIES} ${CHAN_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
