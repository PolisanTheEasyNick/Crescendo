#include "gtkmm/application.h"
#include "playerwindow.h"

int main(int argc, char *argv[]) {
#ifndef HAVE_DBUS
#ifndef HAVE_PULSEAUDIO
#ifndef SUPPORT_AUDIO_OUTPUT
  std::cout
      << "You cant use this player without sdbus-c++, PulseAudio and set of "
         "SDL2, SDL2_mixer, SndFile and taglib. Aborting."
      << std::endl;
  return -1;
#endif
#endif
#endif
  auto app = Gtk::Application::create("org.polisan.crescendo");
  return app->make_window_and_run<PlayerWindow>(argc, argv);
}
