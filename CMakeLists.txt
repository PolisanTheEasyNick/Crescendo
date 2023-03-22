cmake_minimum_required(VERSION 3.5)

project(Universal-Player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-c++-1)
pkg_check_modules(GTKMM gtkmm-4.0 REQUIRED)

include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
add_definitions(${GTKMM_CFLAGS_OTHER})

add_executable(Universal-Player player.h player.cpp helper.h playerwindow.h playerwindow.cpp main.cpp)

target_include_directories(Universal-Player PRIVATE ${DBUS_INCLUDE_DIRS})
target_link_libraries(Universal-Player PRIVATE ${DBUS_LIBRARIES} pugixml ${GTKMM_LIBRARIES})

install(TARGETS Universal-Player
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
