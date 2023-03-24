#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include "player.h"
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/volumebutton.h>

class PlayerWindow : public Gtk::ApplicationWindow {
public:
  PlayerWindow();
  virtual ~PlayerWindow() {}

protected:
  Player player;
  void on_playpause_clicked(), on_prev_clicked(), on_next_clicked(),
      on_shuffle_clicked(), on_player_choose_clicked();
  Gtk::Grid main_grid;
  Gtk::Box control_buttons, volume_and_player;
  Gtk::Button playpause, prev, next, shuffle, player_choose, device_choose;
  Gtk::Label song_name;
  Gtk::Scale progress_bar_song;
  Gtk::VolumeButton volume_bar;
};

#endif // PLAYERWINDOW_H
