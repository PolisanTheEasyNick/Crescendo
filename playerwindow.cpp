#include "playerwindow.h"

PlayerWindow::PlayerWindow() {
  set_title("Universal Player");
  set_default_size(500, 100);
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
  /* GTK4 is missing pressed/released signals for GtkRange/GtkScale.
   * We need to wait when GTK4 command will fix this bug
     https://gitlab.gnome.org/GNOME/gtk/-/issues/4939  */

  //  auto gesture_click = Gtk::GestureClick::create();
  //  gesture_click->signal_pressed().connect([](int, double, double) {
  //    std::cout << "Left mouse button clicked!" << std::endl;
  //  });
  //  gesture_click->signal_released().connect([](int, double, double) {
  //    std::cout << "Left mouse button released!" << std::endl;
  //  });
  //  progress_bar_song.add_controller(gesture_click);
  progress_bar_song.signal_value_changed().connect([&]() {
    double position = progress_bar_song.get_value();
    uint64_t song_length = -1;
    auto metadata = player.get_metadata();
    auto it = std::find_if(
        metadata.begin(), metadata.end(),
        [song_length](const auto &p) { return p.first == "mpris:length"; });
    if (it != metadata.end()) {
      song_length = std::stoul(it->second);
    } else {
      std::cout << "Can't get maximum length of song! Can't continue."
                << std::endl;
      return;
    }
    player.set_position(position * song_length);
  });

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
