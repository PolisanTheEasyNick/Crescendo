#include "gtkmm/application.h"
#include "playerwindow.h"
#include <iostream>

int main(int argc, char *argv[]) {
  auto app = Gtk::Application::create("org.polisan.player");
  return app->make_window_and_run<PlayerWindow>(argc, argv);
  return 0;
}
