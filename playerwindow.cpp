#include "playerwindow.h"

PlayerWindow::PlayerWindow() {
  set_title("Universal Player");
  set_default_size(500, 500);
  set_child(main_grid);
  // main_grid.set_row_homogeneous(true);
  main_grid.set_column_homogeneous(true);
  main_grid.set_margin(20);

  shuffle.set_icon_name("media-playlist-shuffle");
  prev.set_icon_name("media-skip-backward");
  playpause.set_icon_name("media-playback-pause");
  next.set_icon_name("media-skip-forward");

  control_buttons.set_orientation(Gtk::Orientation::HORIZONTAL);
  control_buttons.set_halign(Gtk::Align::CENTER);
  control_buttons.set_valign(Gtk::Align::CENTER);
  control_buttons.set_spacing(5);
  control_buttons.append(shuffle);
  control_buttons.append(prev);
  control_buttons.append(playpause);
  control_buttons.append(next);

  device_choose.set_icon_name("multimedia-player");
  player_choose.set_icon_name("audio-headphones");

  volume_bar.set_orientation(Gtk::Orientation::VERTICAL);
  volume_and_player.set_orientation(Gtk::Orientation::HORIZONTAL);
  volume_and_player.set_halign(Gtk::Align::END);
  volume_and_player.set_valign(Gtk::Align::CENTER);
  volume_and_player.append(player_choose);
  volume_and_player.append(device_choose);
  volume_and_player.append(volume_bar);
  volume_and_player.set_spacing(5);

  progress_bar_song.set_halign(Gtk::Align::FILL);
  progress_bar_song.set_valign(Gtk::Align::END);
  progress_bar_song.set_orientation(Gtk::Orientation::HORIZONTAL);
  progress_bar_song.set_adjustment(Gtk::Adjustment::create(0.50, 0, 1));

  song_name.set_label("1");
  song_name.set_halign(Gtk::Align::START);
  song_name.set_valign(Gtk::Align::CENTER);
  main_grid.attach(song_name, 0, 1);
  playpause.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_playpause_clicked));

  main_grid.attach(control_buttons, 1, 1);
  main_grid.attach(volume_and_player, 2, 1);
  main_grid.attach(progress_bar_song, 0, 0, 3, 1);
}

void PlayerWindow::on_playpause_clicked() {
  song_name.set_label("You clicked!");
}

void PlayerWindow::on_prev_clicked() { song_name.set_label("You clicked!"); }

void PlayerWindow::on_next_clicked() { song_name.set_label("You clicked!"); }

void PlayerWindow::on_shuffle_clicked() { song_name.set_label("You clicked!"); }

void PlayerWindow::on_player_choose_clicked() {
  song_name.set_label("You clicked!");
}
