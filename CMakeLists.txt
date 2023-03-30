cmake_minimum_required(VERSION 3.5)

project(Crescendo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(PkgConfig REQUIRED)


pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)
pkg_check_modules(SDBUS REQUIRED sdbus-c++)

include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
add_definitions(${GTKMM_CFLAGS_OTHER})


# Install the icon file using xdg-desktop-icon
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon.svg"
        DESTINATION $ENV{HOME}/.local/share/icons/hicolor/scalable/apps/
        RENAME "org.polisan.crescendo.svg")

add_executable(Crescendo player.h player.cpp helper.h playerwindow.h playerwindow.cpp volumebutton.h volumebutton.cpp main.cpp)

target_include_directories(Crescendo PRIVATE ${SDBUS_CXX_INCLUDE_DIRS})
target_link_libraries(Crescendo PRIVATE pugixml sdbus-c++ ${GTKMM_LIBRARIES})

find_package(PulseAudio)
if (PulseAudio_FOUND)
    add_definitions(-DHAVE_PULSEAUDIO)
    target_include_directories(Crescendo PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    target_link_libraries(Crescendo PRIVATE ${PULSEAUDIO_LIBRARIES} pulse)
else()
    message(WARNING "PulseAudio not found. Building without PulseAudio support.")
endif()

install(TARGETS Crescendo LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
