#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include "player.h"
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/popover.h>
#include <gtkmm/scale.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/volumebutton.h>

class PlayerWindow : public Gtk::ApplicationWindow {
public:
  PlayerWindow();
  virtual ~PlayerWindow() {}

protected:
  Player player;
  void on_playpause_clicked(), on_prev_clicked(), on_next_clicked(),
      on_shuffle_clicked(), on_player_choose_clicked(),
      on_device_choose_clicked();
  Gtk::Grid main_grid;
  Gtk::Box control_buttons_box, volume_and_player_box;
  Gtk::Button playpause_button, prev_button, next_button, shuffle_button,
      player_choose_button, device_choose_button;
  Gtk::Label song_name_label;
  Gtk::Scale progress_bar_song_scale;
  Gtk::VolumeButton volume_bar_volume_button;
  Gtk::Popover player_choose_popover, device_choose_popover;

private:
  void check_buttons_features();
  void on_player_choosed(unsigned short);
  void on_device_choosed(unsigned short);
};

#endif // PLAYERWINDOW_H
