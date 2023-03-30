#include "playerwindow.h"

PlayerWindow::PlayerWindow() {

  set_icon_name("org.polisan.crescendo");
  set_default_icon_name("org.polisan.crescendo");
  set_title("Universal Player");
  set_default_size(500, 100);
  set_child(m_main_grid);
  // main_grid.set_row_homogeneous(true);
  m_main_grid.set_column_homogeneous(true);
  m_main_grid.set_margin(20);

  m_shuffle_button.set_icon_name("media-playlist-shuffle");
  m_prev_button.set_icon_name("media-skip-backward");
  m_playpause_button.set_icon_name("media-playback-pause");
  m_next_button.set_icon_name("media-skip-forward");

  m_control_buttons_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_control_buttons_box.set_halign(Gtk::Align::CENTER);
  m_control_buttons_box.set_valign(Gtk::Align::CENTER);
  m_control_buttons_box.set_spacing(5);
  m_control_buttons_box.append(m_shuffle_button);
  m_control_buttons_box.append(m_prev_button);
  m_control_buttons_box.append(m_playpause_button);
  m_control_buttons_box.append(m_next_button);

  m_device_choose_button.set_icon_name("audio-headphones");
  m_player_choose_button.set_icon_name("multimedia-player");

  m_volume_bar_scale_button.set_orientation(Gtk::Orientation::VERTICAL);
  m_volume_bar_scale_button.signal_value_changed().connect(
      [this](double value) { m_player.set_volume(value); });
  m_volume_and_player_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_volume_and_player_box.set_halign(Gtk::Align::END);
  m_volume_and_player_box.set_valign(Gtk::Align::CENTER);
  m_volume_and_player_box.append(m_player_choose_button);
  m_volume_and_player_box.append(m_device_choose_button);
  m_volume_and_player_box.append(m_volume_bar_scale_button);
  m_volume_and_player_box.set_spacing(5);

  m_progress_bar_song_scale.set_halign(Gtk::Align::FILL);
  m_progress_bar_song_scale.set_valign(Gtk::Align::END);
  m_progress_bar_song_scale.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_progress_bar_song_scale.set_adjustment(Gtk::Adjustment::create(0.5, 0, 1));
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
  m_progress_bar_song_scale.signal_value_changed().connect([&]() {
    double position = m_progress_bar_song_scale.get_value();
    uint64_t song_length = -1;
    auto metadata = m_player.get_metadata();
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
    m_player.set_position(position * song_length);
  });

  m_song_name_label.set_label("1");
  m_song_name_label.set_halign(Gtk::Align::START);
  m_song_name_label.set_valign(Gtk::Align::CENTER);
  m_main_grid.attach(m_song_name_label, 0, 1);
  m_playpause_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_playpause_clicked));
  m_prev_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_prev_clicked));
  m_next_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_next_clicked));
  m_shuffle_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_shuffle_clicked));
  m_player_choose_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_player_choose_clicked));
  m_device_choose_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_device_choose_clicked));

  m_main_grid.attach(m_control_buttons_box, 1, 1);
  m_main_grid.attach(m_volume_and_player_box, 2, 1);
  m_main_grid.attach(m_progress_bar_song_scale, 0, 0, 3, 1);

  // add shuffle css for changing colors if shuffle enabled
  //  Create a CSS provider and load a stylesheet
  auto css_provider = Gtk::CssProvider::create();
  css_provider->load_from_data(".shuffle-enabled {\n"
                               "  background-color: green;\n"
                               "}\n");
  // Apply the stylesheet to our custom widget
  m_shuffle_button.get_style_context()->add_provider(css_provider, 600);
  if (m_player.get_shuffle()) {
    m_shuffle_button.get_style_context()->add_class("shuffle-enabled");
  }
  // check if buttons accessible or not
  check_buttons_features();

  m_player_choose_popover.signal_closed().connect(
      [this] { m_player_choose_popover.unparent(); });
  m_player_choose_popover.set_halign(Gtk::Align::FILL);

  m_device_choose_popover.signal_closed().connect(
      [this] { m_player_choose_popover.unparent(); });
  m_device_choose_popover.set_halign(Gtk::Align::FILL);

#ifdef HAVE_PULSEAUDIO
#else
  // Code that doesn't uses PulseAudio
  std::cout << "PulseAudio not installed, making button for choosing output "
               "sound device unactive"
            << std::endl;
  m_device_choose_button.set_sensitive(false);
  m_device_choose_button.set_tooltip_text(
      "You must install pulseaudio library for this button");
#endif
}

void PlayerWindow::on_playpause_clicked() {
  bool is_playing = m_player.get_playback_status();
  bool success = m_player.send_play_pause();
  if (is_playing && success) {
    m_playpause_button.set_icon_name("media-playback-start");
  } else if (!is_playing && success) {
    m_playpause_button.set_icon_name("media-playback-pause");
  }
}

void PlayerWindow::on_prev_clicked() { m_player.send_previous(); }

void PlayerWindow::on_next_clicked() { m_player.send_next(); }

void PlayerWindow::on_shuffle_clicked() {
  bool current_shuffle = m_player.get_shuffle();
  bool success = m_player.set_shuffle(!current_shuffle);
  if (!current_shuffle && success) {
    auto style_context = m_shuffle_button.get_style_context();
    style_context->add_class("shuffle-enabled");
  } else if (current_shuffle && success) {
    auto style_context = m_shuffle_button.get_style_context();
    style_context->remove_class("shuffle-enabled");
  }
}

void PlayerWindow::on_player_choose_clicked() {
  if (!m_player_choose_popover.get_visible()) {
    std::cout << "Not visible" << std::endl;
  } else {
    std::cout << "Visiable" << std::endl;
  }
  // Set up popover for playerc choose button
  m_player_choose_popover.set_parent(m_player_choose_button);
  Gtk::ListBox players_list;
  players_list.set_margin_bottom(5);
  players_list.set_halign(Gtk::Align::FILL);
  std::string selected_player_interface = m_player.get_current_player_name();
  auto players_vec = m_player.get_players();

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

  m_player_choose_popover.set_child(players_list);
  m_player_choose_popover.popup();
}

void PlayerWindow::on_player_choosed(unsigned short player_index) {
  m_player_choose_popover.popdown();
  m_player.select_player(player_index);
  if (m_player.get_shuffle()) {
    m_shuffle_button.get_style_context()->add_class("shuffle-enabled");
  } else {
    m_shuffle_button.get_style_context()->remove_class("shuffle-enabled");
  }
  check_buttons_features();
  bool is_playing = m_player.get_playback_status();
  if (!is_playing) {
    m_playpause_button.set_icon_name("media-playback-start");
  } else {
    m_playpause_button.set_icon_name("media-playback-pause");
  }
  if (m_player.get_is_volume_prop()) {
    std::cout << "Player changed, new player volume: ";
    double volume = m_player.get_volume();
    std::cout << volume << std::endl;
    m_volume_bar_scale_button.set_value(volume);
  }
}

void PlayerWindow::on_device_choose_clicked() {
#ifdef HAVE_PULSEAUDIO
  // Code thatuse PulseAudio
  std::cout << "PulseAudio installed" << std::endl;
#else
  // Code that doesn't uses PulseAudio
  std::cout << "PulseAudio not installed, can't continue." << std::endl;
  return;
#endif
  // Set up popover for device choose button
  m_device_choose_popover.set_parent(m_device_choose_button);
  Gtk::ListBox devices_list;
  devices_list.set_margin_bottom(5);
  devices_list.set_halign(Gtk::Align::FILL);
  unsigned short selected_device_sink_index =
      m_player.get_current_device_sink_index();
  std::cout << "Player selected device: " << selected_device_sink_index
            << std::endl;
  auto devices_vec = m_player.get_output_devices();

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

  m_device_choose_popover.set_child(devices_list);
  m_device_choose_popover.popup();
}

void PlayerWindow::on_device_choosed(unsigned short device_sink_index) {
  m_device_choose_popover.popdown();
  m_player.set_output_device(device_sink_index);
}

void PlayerWindow::check_buttons_features() {
  if (m_player.get_play_pause_method()) {
    m_playpause_button.set_sensitive(true);
  } else {
    m_playpause_button.set_sensitive(false);
  }
  if (m_player.get_next_method()) {
    m_next_button.set_sensitive(true);
  } else {
    m_next_button.set_sensitive(false);
  }
  if (m_player.get_previous_method()) {
    m_prev_button.set_sensitive(true);
  } else {
    m_prev_button.set_sensitive(false);
  }
  if (m_player.get_setpos_method()) {
    m_progress_bar_song_scale.set_sensitive(true);
  } else {
    m_progress_bar_song_scale.set_sensitive(false);
  }
  if (m_player.get_is_shuffle_prop()) {
    m_shuffle_button.set_sensitive(true);
  } else {
    m_shuffle_button.set_sensitive(false);
  }
  if (m_player.get_is_pos_prop()) {
    // update time of song
  } else {
    // not update time of song
  }
  if (m_player.get_is_volume_prop()) {
    m_volume_bar_scale_button.set_sensitive(true);
  } else {
    m_volume_bar_scale_button.set_sensitive(false);
  }
}
