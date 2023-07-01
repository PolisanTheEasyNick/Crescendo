cmake_minimum_required(VERSION 3.5)

project(Crescendo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
add_definitions(${GTKMM_CFLAGS_OTHER})

add_executable(crescendo player.h player.cpp helper.h playerwindow.h playerwindow.cpp volumebutton.h volumebutton.cpp playlistrow.h playlistrow.cpp main.cpp)

pkg_check_modules(SDBUS sdbus-c++)


if(SDBUS_FOUND)
    add_definitions(-DHAVE_DBUS)
    target_include_directories(crescendo PRIVATE ${SDBUS_CXX_INCLUDE_DIRS})
    target_link_libraries(crescendo PRIVATE sdbus-c++)
else()
    message(WARNING "sdbus-c++ not found. Building without other players support.")
endif()

find_package(pugixml REQUIRED)
target_link_libraries(crescendo PRIVATE pugixml ${GTKMM_LIBRARIES})

find_package(PulseAudio)
pkg_search_module(PKG_PipeWire QUIET libpipewire-0.3 libpipewire-0.2)
if (PulseAudio_FOUND AND PKG_PipeWire_NOT_FOUND)
    add_definitions(-DHAVE_PULSEAUDIO)
    target_include_directories(crescendo PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    target_link_libraries(crescendo PRIVATE ${PULSEAUDIO_LIBRARIES} pulse)
    message(STATUS "PulseAudio found.")
else()
    message(WARNING "PulseAudio not found or PipeWire installed. Building without PulseAudio support.")
endif()

find_package(SDL2)
find_package(SDL2_mixer)
find_package(SndFile)
pkg_check_modules(TAGLIB taglib)
if (SDL2_FOUND AND SDL2_mixer_FOUND AND SndFile_FOUND AND TAGLIB_FOUND)
    add_definitions(-DSUPPORT_AUDIO_OUTPUT)
    target_include_directories(crescendo PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS})
    target_link_libraries(crescendo PRIVATE SDL2 SDL2_mixer ${SNDFILE_LIBRARIES} ${TAGLIB_LIBRARIES})
    message(STATUS "Supported audio output.")
else()
    message(WARNING "SDL2 or SDL2_mixer or SndFile or taglib not found. Building without audio output support.")
endif()

install(TARGETS crescendo LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(NOT EXISTS $ENV{HOME}/.local/share/icons/hicolor/scalable/apps/org.polisan.crescendo.svg)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon.svg"
            DESTINATION $ENV{HOME}/.local/share/icons/hicolor/scalable/apps/
            RENAME "org.polisan.crescendo.svg")
endif()
