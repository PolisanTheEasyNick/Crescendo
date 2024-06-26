#include <csignal>
#include <iostream>
#include <string>

bool withGui = true;

#include "gtkmm/application.h"
#include "player.h"
#include "playerwindow.h"

bool appRunning = true;

// Signal handler function to catch the termination signal
void signalHandler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    // Set the flag to false to stop the application
    Helper::get_instance().log("Quitting...");
    appRunning = false;
  }
}

void runNoGuiMode() {
  Helper::get_instance().log("Starting in no GUI mode.");
  Player player(false);
  while(player.get_players().size() <= 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));  // wait 5 sec
  }
  player.select_player(0);

  // Run the application within the while loop
#ifdef HAVE_DBUS
  player.start_listening_signals();  // start listening signals
  Helper::get_instance().log("Current song: " + player.get_song_name() +
                             " by " + player.get_song_author());
#endif
  while (appRunning) {
#ifdef HAVE_DBUS
    player.update_position_thread();
#endif
    if (!appRunning) {
        Helper::get_instance().log("Stopping server");
      player.stop_server();
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  // Check if the "--no-gui" argument is present
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--no-gui") {
      withGui = false;
      std::signal(SIGINT, signalHandler);
      std::signal(SIGTERM, signalHandler);
      break;
    }
  }

#ifndef HAVE_DBUS
#ifndef HAVE_PULSEAUDIO
#ifndef SUPPORT_AUDIO_OUTPUT
  if (!with_gui) {
    Helper::get_instance().log(
        "You can't use this player without sdbus-c++, PulseAudio, and "
        "the set of "
        "SDL2, SDL2_mixer, SndFile, and taglib. Aborting.");
    return -1;
  }

#endif
#endif
#endif

  if (!withGui) {
    std::thread noGuiThread(runNoGuiMode);
    if (noGuiThread.joinable()) noGuiThread.join();
  } else {
    // Run the GUI version of the application
    auto app = Gtk::Application::create("org.polisan.crescendo");
    return app->make_window_and_run<PlayerWindow>(argc, argv);
  }
  return 0;
}
