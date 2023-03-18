cmake_minimum_required(VERSION 3.5)

project(Universal-Player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-c++-1)

add_executable(Universal-Player player.h player.cpp helper.h main.cpp)

target_include_directories(Universal-Player PRIVATE ${DBUS_INCLUDE_DIRS})
target_link_libraries(Universal-Player PRIVATE ${DBUS_LIBRARIES})

install(TARGETS Universal-Player
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
