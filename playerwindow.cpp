#include "playerwindow.h"

PlayerWindow::PlayerWindow() : m_player() {
  m_player.add_observer(this);
  // setup current choosed player
  if (m_player.get_shuffle()) {
    m_shuffle_button.get_style_context()->add_class("shuffle-enabled");
  } else {
    m_shuffle_button.get_style_context()->remove_class("shuffle-enabled");
  }
  bool is_playing = m_player.get_playback_status();
  if (!is_playing) {
    m_playpause_button.set_icon_name("media-playback-start");
  } else {
    m_playpause_button.set_icon_name("media-playback-pause");
  }
  if (m_player.get_is_volume_prop()) {
    double volume = m_player.get_volume();
    m_volume_bar_scale_button.set_value(volume);
  }

  set_icon_name("org.polisan.crescendo");
  set_default_icon_name("org.polisan.crescendo");
  set_title("Crescendo");
  set_default_size(500, 100);
  set_child(m_main_grid);
  // main_grid.set_row_homogeneous(true);
  m_main_grid.set_column_homogeneous(true);
  m_main_grid.set_margin(15);
  m_main_grid.set_valign(Gtk::Align::FILL);
  m_main_grid.set_vexpand(true);
  m_shuffle_button.set_icon_name("media-playlist-shuffle");
  m_prev_button.set_icon_name("media-skip-backward");
  m_playpause_button.set_icon_name("media-playback-start");
  m_next_button.set_icon_name("media-skip-forward");

  m_control_buttons_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_control_buttons_box.set_halign(Gtk::Align::CENTER);
  m_control_buttons_box.set_valign(Gtk::Align::END);
  // m_control_buttons_box.set_vexpand();
  m_control_buttons_box.set_spacing(5);
  m_control_buttons_box.append(m_shuffle_button);
  m_control_buttons_box.append(m_prev_button);
  m_control_buttons_box.append(m_playpause_button);
  m_control_buttons_box.append(m_next_button);

  m_device_choose_button.set_icon_name("audio-headphones");
  m_player_choose_button.set_icon_name("multimedia-player");

  m_volume_bar_scale_button.set_orientation(Gtk::Orientation::VERTICAL);
  m_volume_bar_scale_button.signal_value_changed().connect(
      [this](double value) {
        if (m_lock_volume_changing)
          return;
        m_player.set_volume(value);
      });
  m_volume_and_player_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_volume_and_player_box.set_halign(Gtk::Align::END);
  m_volume_and_player_box.set_valign(Gtk::Align::END);
  // m_volume_and_player_box.set_vexpand();
  m_volume_and_player_box.append(m_player_choose_button);
  m_volume_and_player_box.append(m_device_choose_button);
  m_volume_and_player_box.append(m_volume_bar_scale_button);
  m_volume_and_player_box.set_spacing(5);

  m_current_pos_label.set_label("00:00");
  m_current_pos_label.set_halign(Gtk::Align::START);
  m_current_pos_label.set_valign(Gtk::Align::CENTER);
  // m_current_pos_label.set_vexpand();

  m_progress_bar_song_scale.set_halign(Gtk::Align::FILL);
  m_progress_bar_song_scale.set_valign(Gtk::Align::END);
  // m_progress_bar_song_scale.set_vexpand();
  m_progress_bar_song_scale.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_progress_bar_song_scale.set_adjustment(Gtk::Adjustment::create(0.5, 0, 1));
  m_progress_bar_song_scale.set_margin_start(40);
  m_progress_bar_song_scale.set_margin_end(40);

  m_song_length_label.set_label("00:00");
  m_song_length_label.set_halign(Gtk::Align::END);
  m_song_length_label.set_valign(Gtk::Align::CENTER);
  // m_song_length_label.set_vexpand();

  auto controllers = m_progress_bar_song_scale.observe_controllers();
  GListModel *model = controllers->gobj();
  int n_controllers = g_list_model_get_n_items(model);
  for (int i = 0; i < n_controllers; i++) {
    GObject *controller_gobj = (GObject *)g_list_model_get_item(model, i);
    auto click_controller = Glib::wrap(controller_gobj, false);
    auto gesture_click =
        dynamic_cast<Gtk::GestureClick *>(click_controller.get());
    if (gesture_click) {
      gesture_click->set_button(0);
      gesture_click->signal_pressed().connect(
          [this](int, double, double) { m_wait = true; });
      gesture_click->signal_released().connect([this](int, double, double) {
        if (m_lock_pos_changing)
          return;
        double position = m_progress_bar_song_scale.get_value();
        uint64_t song_length = m_player.get_song_length();
        if (m_player.get_current_player_name() == "Local") {
          m_lock_pos_changing = true;
          m_current_pos_label.set_label(
              Helper::get_instance().format_time(position * song_length));
          m_player.set_position(position * song_length);
          m_lock_pos_changing = false;
          m_wait = false;
        } else {
          m_lock_pos_changing = true;
          m_current_pos_label.set_label(
              Helper::get_instance().format_time(position * song_length));
          m_player.set_position(position * song_length * 1000000);
          m_lock_pos_changing = false;
          m_wait = false;
        }
      });
    }
  }
  m_song_title_label.set_label("");
  m_song_title_label.set_halign(Gtk::Align::START);
  m_song_title_label.set_valign(Gtk::Align::CENTER);
  m_song_title_label.set_ellipsize(Pango::EllipsizeMode::END);
  m_song_artist_label.set_label("");
  m_song_artist_label.set_halign(Gtk::Align::START);
  m_song_artist_label.set_valign(Gtk::Align::CENTER);
  m_song_title_label.set_ellipsize(Pango::EllipsizeMode::END);
  m_song_title_list.append(m_song_title_label);
  m_song_title_list.append(m_song_artist_label);
  m_song_title_list.set_valign(Gtk::Align::END);
  // m_song_title_list.set_vexpand();
  m_song_title_list.set_activate_on_single_click(false);
  m_song_title_list.set_selection_mode(Gtk::SelectionMode::NONE);
  m_song_title_label.set_can_target(false);
  m_song_artist_label.set_can_target(false);
  m_song_title_list.set_can_target(false);
  m_song_title_label.set_focusable(false);
  m_song_artist_label.set_focusable(false);
  m_song_title_list.set_can_focus(false);

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

  m_main_grid.attach(m_current_pos_label, 0, 1);
  m_main_grid.attach(m_song_length_label, 2, 1);
  m_main_grid.attach(m_progress_bar_song_scale, 0, 1, 3, 1);
  m_main_grid.attach(m_control_buttons_box, 1, 2);
  m_main_grid.attach(m_volume_and_player_box, 2, 2);
  m_main_grid.attach(m_song_title_list, 0, 2);

  // add shuffle css for changing colors if shuffle enabled
  //  Create a CSS provider and load a stylesheet
  auto css_provider = Gtk::CssProvider::create();
  css_provider->load_from_data(".shuffle-enabled {\n"
                               "  background-color: @theme_selected_bg_color;\n"
                               "}"
                               ".new-background {\n"
                               "  background-color: @theme_bg_color;\n"
                               "}\n");
  // Apply the stylesheet to our custom widget
  m_song_title_list.get_style_context()->add_provider(
      css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
  m_playlist_listbox.get_style_context()->add_provider(
      css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
  m_song_title_list.get_style_context()->add_class("new-background");
  m_playlist_listbox.get_style_context()->add_class("new-background");
  m_shuffle_button.get_style_context()->add_provider(
      css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
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

  m_playpause_button.grab_focus();

  stop_flag = false;
  if (m_player.get_playback_status()) {
    // Start the idle handler to update the position
    m_position_thread =
        std::thread(&PlayerWindow::update_position_thread, this);
  }

  // adding elements for playlist
  m_add_song_to_playlist_button.set_icon_name("add");
  m_add_song_to_playlist_button.set_size_request(10, 10);
  m_add_song_to_playlist_button.set_halign(Gtk::Align::START);
  m_add_song_to_playlist_button.set_valign(Gtk::Align::START);
  m_playlist_scrolled_window.set_valign(Gtk::Align::FILL);
  m_playlist_scrolled_window.set_halign(Gtk::Align::FILL);
  m_playlist_scrolled_window.set_policy(Gtk::PolicyType::NEVER,
                                        Gtk::PolicyType::AUTOMATIC);
  m_playlist_scrolled_window.set_vexpand();
  m_playlist_scrolled_window.set_hexpand();
  m_playlist_scrolled_window.set_margin_start(50);
  m_playlist_scrolled_window.set_child(m_playlist_listbox);
  m_playlist_listbox.set_show_separators();

  m_playlist_listbox.signal_row_activated().connect(
      [this](Gtk::ListBoxRow *row) {
        auto playlist_row = dynamic_cast<PlaylistRow *>(row);
        m_player.open_audio(playlist_row->get_filename());
        if (m_activated_row != NULL) {
          m_activated_row->stop_highlight();
        }
        playlist_row->highlight();
        m_activated_row = playlist_row;
        if (m_player.get_is_playing()) {
          m_player.play_audio();
        }
      });
  m_main_grid.attach(m_add_song_to_playlist_button, 0, 0);
  m_main_grid.attach(m_playlist_scrolled_window, 0, 0, 3, 1);
  m_add_song_to_playlist_button.signal_clicked().connect([this] {
    auto file_dialog = Gtk::FileDialog::create();
    file_dialog->open(
        [file_dialog, this](Glib::RefPtr<Gio::AsyncResult> &result) -> void {
          try {
            auto file = file_dialog->open_finish(result);
#ifdef SUPPORT_AUDIO_OUTPUT
            auto opened_file = Mix_LoadMUS(file->get_path().c_str());
            if (opened_file) {
              add_song_to_playlist(file->get_path().c_str());
            }
#endif
          } catch (Glib::Error err) {
          }
        });
  });

  if (m_player.get_current_player_name() != "Local") {
    m_playlist_scrolled_window.hide();
    m_add_song_to_playlist_button.hide();
  }

#ifndef HAVE_PULSEAUDIO
  // Code that doesn't uses PulseAudio
  std::cout << "PulseAudio not installed, making button for choosing output "
               "sound device unactive"
            << std::endl;
  m_device_choose_button.set_sensitive(false);
  m_device_choose_button.set_tooltip_text(
      "You must install pulseaudio library for this button");
#endif

  if (m_player.get_current_player_name() != "Local") {
#ifdef HAVE_DBUS
    m_player.start_listening_signals();
    m_player.get_song_data();
#endif
  }
}

void PlayerWindow::on_playpause_clicked() {
#ifdef SUPPORT_AUDIO_OUTPUT

  if (m_activated_row) { // some song is already chosen
    if (m_player.get_is_playing()) {
      m_player.pause_audio();
    } else {
      m_player.play_audio();
    }
    return;
  }
  if (m_player.get_current_player_name() == "Local" &&
      m_player.get_music() ==
          NULL) { // no chosen song and playpause clicked, picking first song

    auto listbox = dynamic_cast<Gtk::ListBox *>(
        m_playlist_scrolled_window.get_child()->get_first_child());
    if (listbox) {
      int n_children = listbox->observe_children()->get_n_items();
      if (n_children != 0) {
        auto listitem =
            dynamic_cast<PlaylistRow *>(listbox->get_row_at_index(0));
        if (listitem) {
          if (m_activated_row)
            m_activated_row->stop_highlight();
          m_activated_row = listitem;
          m_activated_row->highlight();
          m_player.open_audio(listitem->get_filename());
          m_player.play_audio();
          return;
        }
      } else {
        auto error_dialog = Gtk::AlertDialog::create(
            "You need to add song or choose another player");
        error_dialog->show(*this);
        return;
      }
    }
  }
#endif
  m_player.send_play_pause();
}

void PlayerWindow::on_prev_clicked() { m_player.send_previous(); }

void PlayerWindow::on_next_clicked() {

#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_player.get_current_player_name() == "Local") {
    std::cout << "Next clicked" << std::endl;
    auto listbox = dynamic_cast<Gtk::ListBox *>(
        m_playlist_scrolled_window.get_child()->get_first_child());
    if (listbox) {
      int n_children = listbox->observe_children()->get_n_items();
      for (int i = 0; i < n_children; i++) {
        auto listitem =
            dynamic_cast<PlaylistRow *>(listbox->get_row_at_index(i));
        if (listitem == m_activated_row) {
          std::cout << "Current song: " << listitem->get_filename()
                    << std::endl;
          auto next_list_item =
              dynamic_cast<PlaylistRow *>(listbox->get_row_at_index(i + 1));
          if (next_list_item) {
            std::cout << "Next song: " << next_list_item->get_filename()
                      << std::endl;
            m_player.open_audio(next_list_item->get_filename());
            if (m_player.get_is_playing())
              m_player.play_audio();
            if (m_activated_row)
              m_activated_row->stop_highlight();
            m_activated_row = next_list_item;
            m_activated_row->highlight();
            stop_flag = false;
            return;
          } else {
            std::cout << "No next song." << std::endl;
            return;
          }
        }
      }
      std::cout << "No song picked at all." << std::endl;
    }
    return;
  }
#endif
  m_player.send_next();
}

void PlayerWindow::on_shuffle_clicked() {
  bool current_shuffle = m_player.get_shuffle();
  m_player.set_shuffle(!current_shuffle);
}

void PlayerWindow::on_player_choose_clicked() {
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

  m_player.get_song_data();
  if (m_player.get_current_player_name() != "Local") {
#ifdef HAVE_DBUS
    m_player.start_listening_signals();
#endif
  }
  if (m_player.get_current_player_name() == "Local") {
    m_add_song_to_playlist_button.show();
    m_playlist_scrolled_window.show();
    m_main_grid.set_valign(Gtk::Align::FILL);
  } else {
    m_add_song_to_playlist_button.hide();
    m_playlist_scrolled_window.hide();
    m_main_grid.set_valign(Gtk::Align::END);
    set_default_size(500, 100);
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
  } else {
  }
  if (m_player.get_is_volume_prop()) {
    m_volume_bar_scale_button.set_sensitive(true);
  } else {
    m_volume_bar_scale_button.set_sensitive(false);
  }
}

void PlayerWindow::update_position_thread() {
  std::cout << "Started tracking position" << std::endl;
  std::cout << "Stop flag: " << stop_flag << std::endl;
  m_mutex.lock();
  while (!stop_flag) {
    {
      if (m_wait) {
        continue;
      }
      if (!m_player.get_playback_status()) {
        std::cout << "Not playback_status, False, paused" << std::endl;
        m_progress_bar_song_scale.set_value(0.0);
        m_progress_bar_song_scale.queue_draw();
        m_current_pos_label.set_label("00:00");
        m_playpause_button.set_icon_name("media-play");
        break;
      }
      double current_pos = m_player.get_position();
      std::cout << "Current pos: " << current_pos << ", "
                << m_player.get_position_str() << std::endl;
      m_current_pos_label.set_label(m_player.get_position_str());
      m_lock_pos_changing = true;
      m_progress_bar_song_scale.set_value(current_pos /
                                          m_player.get_song_length());
      m_progress_bar_song_scale.queue_draw();
      m_main_grid.queue_draw();
      m_lock_pos_changing = false;
    }
    m_mutex.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    m_mutex.lock();
  }
  m_mutex.unlock();
  std::cout << "Thread stopped" << std::endl;
  stop_flag = false;
}

void PlayerWindow::pause_position_thread() { m_wait = true; }

void PlayerWindow::resume_position_thread() {
  m_wait = false;
  if (!m_position_thread.joinable()) {
    stop_flag = false;
    m_position_thread =
        std::thread(&PlayerWindow::update_position_thread, this);
  } else {
    stop_flag = true;
    m_position_thread.join();
    stop_flag = false;
    m_position_thread =
        std::thread(&PlayerWindow::update_position_thread, this);
  }
}

void PlayerWindow::stop_position_thread() {
  stop_flag = true;
  if (m_position_thread.joinable()) {
    m_position_thread.join();
  }
}

void PlayerWindow::add_song_to_playlist(const std::string &filename) {
#ifdef SUPPORT_AUDIO_OUTPUT
  Mix_Music *music = Mix_LoadMUS(filename.c_str());
  if (!music) {
    std::cout << "Mix_LoadMUS failed: " << Mix_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string song_title = Mix_GetMusicTitle(music);
  std::string song_artist = Mix_GetMusicArtistTag(music);
  std::string song_length =
      Helper::get_instance().format_time(Mix_MusicDuration(music));

  m_playlist_listbox.append(*Gtk::make_managed<PlaylistRow>(
      song_title, song_artist, song_length, filename));
#endif
}
