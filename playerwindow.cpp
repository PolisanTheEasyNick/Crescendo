#include "playerwindow.h"

PlayerWindow::PlayerWindow() {
  set_title("Universal Player");
  set_default_size(500, 100);
  set_child(main_grid);
  // main_grid.set_row_homogeneous(true);
  main_grid.set_column_homogeneous(true);
  main_grid.set_margin(20);

  shuffle_button.set_icon_name("media-playlist-shuffle");
  prev_button.set_icon_name("media-skip-backward");
  playpause_button.set_icon_name("media-playback-pause");
  next_button.set_icon_name("media-skip-forward");

  control_buttons_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  control_buttons_box.set_halign(Gtk::Align::CENTER);
  control_buttons_box.set_valign(Gtk::Align::CENTER);
  control_buttons_box.set_spacing(5);
  control_buttons_box.append(shuffle_button);
  control_buttons_box.append(prev_button);
  control_buttons_box.append(playpause_button);
  control_buttons_box.append(next_button);

  device_choose_button.set_icon_name("audio-headphones");
  player_choose_button.set_icon_name("multimedia-player");

  volume_bar_volume_button.set_orientation(Gtk::Orientation::VERTICAL);
  volume_and_player_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  volume_and_player_box.set_halign(Gtk::Align::END);
  volume_and_player_box.set_valign(Gtk::Align::CENTER);
  volume_and_player_box.append(player_choose_button);
  volume_and_player_box.append(device_choose_button);
  volume_and_player_box.append(volume_bar_volume_button);
  volume_and_player_box.set_spacing(5);

  progress_bar_song_scale.set_halign(Gtk::Align::FILL);
  progress_bar_song_scale.set_valign(Gtk::Align::END);
  progress_bar_song_scale.set_orientation(Gtk::Orientation::HORIZONTAL);
  progress_bar_song_scale.set_adjustment(Gtk::Adjustment::create(0.50, 0, 1));
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
  //  progress_bar_song_scale.add_controller(gesture_click);
  progress_bar_song_scale.signal_value_changed().connect([&]() {
    double position = progress_bar_song_scale.get_value();
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

  song_name_label.set_label("1");
  song_name_label.set_halign(Gtk::Align::START);
  song_name_label.set_valign(Gtk::Align::CENTER);
  main_grid.attach(song_name_label, 0, 1);
  playpause_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_playpause_clicked));
  prev_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_prev_clicked));
  next_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_next_clicked));
  shuffle_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_shuffle_clicked));
  player_choose_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_player_choose_clicked));
  device_choose_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_device_choose_clicked));

  main_grid.attach(control_buttons_box, 1, 1);
  main_grid.attach(volume_and_player_box, 2, 1);
  main_grid.attach(progress_bar_song_scale, 0, 0, 3, 1);

  // add shuffle css for changing colors if shuffle enabled
  //  Create a CSS provider and load a stylesheet
  auto css_provider = Gtk::CssProvider::create();
  css_provider->load_from_data(".shuffle-enabled {\n"
                               "  background-color: green;\n"
                               "}\n");
  // Apply the stylesheet to our custom widget
  shuffle_button.get_style_context()->add_provider(css_provider, 600);
  if (player.get_shuffle()) {
    shuffle_button.get_style_context()->add_class("shuffle-enabled");
  }
  // check if buttons accessible or not
  check_buttons_features();

  player_choose_popover.signal_closed().connect(
      [this] { player_choose_popover.unparent(); });
  player_choose_popover.set_halign(Gtk::Align::FILL);

  device_choose_popover.signal_closed().connect(
      [this] { player_choose_popover.unparent(); });
  device_choose_popover.set_halign(Gtk::Align::FILL);
}

void PlayerWindow::on_playpause_clicked() {
  bool is_playing = player.get_playback_status();
  bool success = player.send_play_pause();
  if (is_playing && success) {
    playpause_button.set_icon_name("media-playback-start");
  } else if (!is_playing && success) {
    playpause_button.set_icon_name("media-playback-pause");
  }
}

void PlayerWindow::on_prev_clicked() { player.send_previous(); }

void PlayerWindow::on_next_clicked() { player.send_next(); }

void PlayerWindow::on_shuffle_clicked() {
  bool current_shuffle = player.get_shuffle();
  bool success = player.set_shuffle(!current_shuffle);
  if (!current_shuffle && success) {
    auto style_context = shuffle_button.get_style_context();
    style_context->add_class("shuffle-enabled");
  } else if (current_shuffle && success) {
    auto style_context = shuffle_button.get_style_context();
    style_context->remove_class("shuffle-enabled");
  }
}

void PlayerWindow::on_player_choose_clicked() {
  if (!player_choose_popover.get_visible()) {
    std::cout << "Not visible" << std::endl;
  } else {
    std::cout << "Visiable" << std::endl;
  }
  // Set up popover for playerc choose button
  player_choose_popover.set_parent(player_choose_button);
  Gtk::ListBox players_list;
  players_list.set_margin_bottom(5);
  players_list.set_halign(Gtk::Align::FILL);
  std::string selected_player_interface = player.get_current_player_name();
  auto players_vec = player.get_players();

  for (const auto &pl : players_vec) {
    auto player_choosing_button = Gtk::ToggleButton(pl.first);
    if (pl.first == selected_player_interface) {
      player_choosing_button.set_active();
    }
    player_choosing_button.set_has_frame(false);
    player_choosing_button.set_can_focus();
    player_choosing_button.set_halign(Gtk::Align::FILL);
    Gtk::ListBoxRow row;
    row.set_selectable(false);
    row.set_child(player_choosing_button);
    row.set_halign(Gtk::Align::FILL);
    row.set_valign(Gtk::Align::CENTER);
    players_list.append(row);
  }
  Gtk::ToggleButton *first_button = dynamic_cast<Gtk::ToggleButton *>(
      players_list.get_row_at_index(0)->get_child());
  for (unsigned short i = 0; i < players_vec.size(); i++) {
    Gtk::ListBoxRow *row = players_list.get_row_at_index(i);
    Gtk::ToggleButton *button =
        dynamic_cast<Gtk::ToggleButton *>(row->get_child());
    if (button) {
      if (button != first_button) {
        button->set_group(*first_button);
      }
      button->signal_clicked().connect(sigc::bind(
          sigc::mem_fun(*this, &PlayerWindow::on_player_choosed), i));
    }
  }

  player_choose_popover.set_child(players_list);
  player_choose_popover.popup();
}

void PlayerWindow::on_player_choosed(unsigned short player_index) {
  player_choose_popover.popdown();
  player.select_player(player_index);
  if (player.get_shuffle()) {
    shuffle_button.get_style_context()->add_class("shuffle-enabled");
  } else {
    shuffle_button.get_style_context()->remove_class("shuffle-enabled");
  }
  check_buttons_features();
  bool is_playing = player.get_playback_status();
  if (!is_playing) {
    playpause_button.set_icon_name("media-playback-start");
  } else {
    playpause_button.set_icon_name("media-playback-pause");
  }
}

void PlayerWindow::on_device_choose_clicked() {
  // Set up popover for device choose button
  device_choose_popover.set_parent(device_choose_button);
  Gtk::ListBox devices_list;
  devices_list.set_margin_bottom(5);
  devices_list.set_halign(Gtk::Align::FILL);
  unsigned short selected_device_sink_index =
      player.get_current_device_sink_index();
  std::cout << "Player selected device: " << selected_device_sink_index
            << std::endl;
  auto devices_vec = player.get_output_devices();

  for (const auto &dev : devices_vec) {
    auto device_choosing_button = Gtk::ToggleButton(dev.first);
    if (dev.second == selected_device_sink_index) {
      device_choosing_button.set_active();
    }
    device_choosing_button.set_has_frame(false);
    device_choosing_button.set_can_focus();
    device_choosing_button.set_halign(Gtk::Align::FILL);
    Gtk::ListBoxRow row;
    row.set_selectable(false);
    row.set_child(device_choosing_button);
    row.set_halign(Gtk::Align::FILL);
    row.set_valign(Gtk::Align::CENTER);
    devices_list.append(row);
  }
  Gtk::ToggleButton *first_button = dynamic_cast<Gtk::ToggleButton *>(
      devices_list.get_row_at_index(0)->get_child());
  for (unsigned short i = 0; i < devices_vec.size(); i++) {
    Gtk::ListBoxRow *row = devices_list.get_row_at_index(i);
    Gtk::ToggleButton *button =
        dynamic_cast<Gtk::ToggleButton *>(row->get_child());
    if (button) {
      if (button != first_button) {
        button->set_group(*first_button);
      }
      button->signal_clicked().connect(
          sigc::bind(sigc::mem_fun(*this, &PlayerWindow::on_device_choosed),
                     devices_vec[i].second));
    }
  }

  device_choose_popover.set_child(devices_list);
  device_choose_popover.popup();
}

void PlayerWindow::on_device_choosed(unsigned short device_sink_index) {
  device_choose_popover.popdown();
  player.set_output_device(device_sink_index);
}

void PlayerWindow::check_buttons_features() {
  if (player.get_play_pause_method()) {
    playpause_button.set_sensitive(true);
  } else {
    playpause_button.set_sensitive(false);
  }
  if (player.get_next_method()) {
    next_button.set_sensitive(true);
  } else {
    next_button.set_sensitive(false);
  }
  if (player.get_previous_method()) {
    prev_button.set_sensitive(true);
  } else {
    prev_button.set_sensitive(false);
  }
  if (player.get_setpos_method()) {
    progress_bar_song_scale.set_sensitive(true);
  } else {
    progress_bar_song_scale.set_sensitive(false);
  }
  if (player.get_is_shuffle_prop()) {
    shuffle_button.set_sensitive(true);
  } else {
    shuffle_button.set_sensitive(false);
  }
  if (player.get_is_pos_prop()) {
    // update time of song
  } else {
    // not update time of song
  }
  if (player.get_is_volume_prop()) {
    volume_bar_volume_button.set_sensitive(true);
  } else {
    volume_bar_volume_button.set_sensitive(false);
  }
}
