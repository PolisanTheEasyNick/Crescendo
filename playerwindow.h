#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include "player.h"
#include "volumebutton.h"
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
#include <gtkmm/scalebutton.h>
#include <gtkmm/togglebutton.h>

class PlayerWindow : public Gtk::ApplicationWindow {
public:
  PlayerWindow();
  virtual ~PlayerWindow() {}

protected:
  Player m_player;
  void on_playpause_clicked(), on_prev_clicked(), on_next_clicked(),
      on_shuffle_clicked(), on_player_choose_clicked(),
      on_device_choose_clicked();
  Gtk::Grid m_main_grid;
  Gtk::Box m_control_buttons_box, m_volume_and_player_box;
  Gtk::Button m_playpause_button, m_prev_button, m_next_button, m_shuffle_button,
      m_player_choose_button, m_device_choose_button;
  Gtk::Label m_song_name_label;
  Gtk::Scale m_progress_bar_song_scale;
  VolumeButton m_volume_bar_scale_button;
  Gtk::Popover m_player_choose_popover, m_device_choose_popover;

private:
  void check_buttons_features();
  void on_player_choosed(unsigned short);
  void on_device_choosed(unsigned short);
};

#endif // PLAYERWINDOW_H
