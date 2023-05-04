#include "playerwindow.h"

// define static variables
Player PlayerWindow::m_player;
Gtk::ScrolledWindow *PlayerWindow::m_playlist_scrolled_window = nullptr;
PlaylistRow *PlayerWindow::m_activated_row = nullptr;

#ifdef SUPPORT_AUDIO_OUTPUT
unsigned int PlayerWindow::m_current_track = -1;
#endif

/**
 * Constructs a new PlayerWindow object.
 */
PlayerWindow::PlayerWindow() {
  m_player.add_observer(this); // make this observer of Player
  m_playlist_scrolled_window = new Gtk::ScrolledWindow();
  // setup stock choosed player
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
  set_child(m_main_grid); // set child of window m_main_grid
  m_main_grid.set_column_homogeneous(true);
  m_main_grid.set_margin(15);
  m_main_grid.set_valign(Gtk::Align::FILL);
  m_main_grid.set_vexpand(true);
  m_shuffle_button.set_icon_name("media-playlist-shuffle");
  m_prev_button.set_icon_name("media-skip-backward");
  m_playpause_button.set_icon_name("media-playback-start");
  m_next_button.set_icon_name("media-skip-forward");
  m_loop_button.set_icon_name("media-repeat-none");

  m_control_buttons_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_control_buttons_box.set_halign(Gtk::Align::CENTER);
  m_control_buttons_box.set_valign(Gtk::Align::END);
  m_control_buttons_box.set_spacing(5);
  m_control_buttons_box.append(m_shuffle_button);
  m_control_buttons_box.append(m_prev_button);
  m_control_buttons_box.append(m_playpause_button);
  m_control_buttons_box.append(m_next_button);
  m_control_buttons_box.append(m_loop_button);

  m_device_choose_button.set_icon_name("audio-headphones");
  m_player_choose_button.set_icon_name("multimedia-player");

  m_volume_bar_scale_button.set_orientation(Gtk::Orientation::VERTICAL);
  m_volume_bar_scale_button.signal_value_changed()
      .connect( // set signal for changing volume
          [this](double value) {
            if (m_lock_volume_changing)
              return;
            m_player.set_volume(value);
          });
  m_volume_and_player_box.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_volume_and_player_box.set_halign(Gtk::Align::END);
  m_volume_and_player_box.set_valign(Gtk::Align::END);
  m_volume_and_player_box.append(m_player_choose_button);
  m_volume_and_player_box.append(m_device_choose_button);
  m_volume_and_player_box.append(m_volume_bar_scale_button);
  m_volume_and_player_box.set_spacing(5);

  m_current_pos_label.set_label("00:00");
  m_current_pos_label.set_halign(Gtk::Align::START);
  m_current_pos_label.set_valign(Gtk::Align::CENTER);

  m_progress_bar_song_scale.set_halign(Gtk::Align::FILL);
  m_progress_bar_song_scale.set_valign(Gtk::Align::END);
  m_progress_bar_song_scale.set_orientation(Gtk::Orientation::HORIZONTAL);
  m_progress_bar_song_scale.set_adjustment(Gtk::Adjustment::create(0.5, 0, 1));
  m_progress_bar_song_scale.set_margin_start(40);
  m_progress_bar_song_scale.set_margin_end(40);

  m_song_length_label.set_label("00:00");
  m_song_length_label.set_halign(Gtk::Align::END);
  m_song_length_label.set_valign(Gtk::Align::CENTER);

  // Since GTK4 have bug https://gitlab.gnome.org/GNOME/gtk/-/issues/4939
  // We will create our own signal for button release in created by stock
  // GestureClick controller of Gtk::Scale
  auto controllers = m_progress_bar_song_scale
                         .observe_controllers(); // get controllers of progress
                                                 // bar scale object
  GListModel *model = controllers->gobj();       // get model
  int n_controllers = g_list_model_get_n_items(model); // get count of children
  for (int i = 0; i < n_controllers; i++) {            // go through all
    GObject *controller_gobj =
        (GObject *)g_list_model_get_item(model, i); // get i'th child
    auto click_controller =
        Glib::wrap(controller_gobj, false); // get click controller
    auto gesture_click = dynamic_cast<Gtk::GestureClick *>(
        click_controller.get());    // try to get Gtk::GestureClick
    if (gesture_click) {            // if we found Gtk::GestureClick
      gesture_click->set_button(0); // set button for main mouse mutton
      gesture_click->signal_pressed().connect( // redefine singal pressed
          [this](int, double, double) { m_wait = true; });
      gesture_click->signal_released().connect(
          [this](int, double, double) { // redefine signal released
            m_wait = true;
            if (m_lock_pos_changing)
              return;
            double position = m_progress_bar_song_scale
                                  .get_value(); // get current value of scale
            uint64_t song_length = m_player.get_song_length(); // get song
                                                               // length
            if (m_player.get_current_player_name() ==
                "Local") { // if local player
              m_lock_pos_changing = true;
              m_current_pos_label.set_label(Helper::get_instance().format_time(
                  position * song_length)); // set label of new pos
              m_player.set_position(position * song_length); // set new pos
              m_lock_pos_changing = false;
              m_wait = false;
            } else {
              m_lock_pos_changing = true;
              m_current_pos_label.set_label(Helper::get_instance().format_time(
                  position *
                  song_length)); // if dbus player, set label with new pos
              m_player.set_position(position * song_length *
                                    1000000); // set new pos
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
  m_song_title_list.set_activate_on_single_click(false);
  m_song_title_list.set_selection_mode(Gtk::SelectionMode::NONE);
  m_song_title_label.set_can_target(false);
  m_song_artist_label.set_can_target(false);
  m_song_title_list.set_can_target(false);
  m_song_title_label.set_focusable(false);
  m_song_artist_label.set_focusable(false);
  m_song_title_list.set_can_focus(false);

  // connect out signals for buttons
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
  m_loop_button.signal_clicked().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_loop_clicked));

  // attach all element to main grid
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

  // remove parents from popover when they closed
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
  m_playlist_scrolled_window->set_valign(Gtk::Align::FILL);
  m_playlist_scrolled_window->set_halign(Gtk::Align::FILL);
  m_playlist_scrolled_window->set_policy(Gtk::PolicyType::NEVER,
                                         Gtk::PolicyType::AUTOMATIC);
  m_playlist_scrolled_window->set_vexpand();
  m_playlist_scrolled_window->set_hexpand();
  m_playlist_scrolled_window->set_margin_start(50);
  m_playlist_scrolled_window->set_child(m_playlist_listbox);
  m_playlist_listbox.set_show_separators();

#ifdef SUPPORT_AUDIO_OUTPUT
  // add signal when we choose song in playlist
  m_playlist_listbox.signal_row_activated().connect([this](
                                                        Gtk::ListBoxRow *row) {
    auto playlist_row = dynamic_cast<PlaylistRow *>(row);
    m_current_track = playlist_row->get_index();
    if (!m_player.open_audio(playlist_row->get_filename())) { // open song
      std::cout << "Can't open file as audio: " << playlist_row->get_filename()
                << std::endl;
    } else {
      if (m_activated_row != NULL) {
        m_activated_row->stop_highlight();
      }
      playlist_row->highlight();
      m_activated_row = playlist_row;
      if (m_player.get_is_playing()) { // and if song already playing
        m_player.play_audio();         // play new song
      }
    }
  });
#endif
  // add elements of playlist to main grid
  m_main_grid.attach(m_add_song_to_playlist_button, 0, 0);
  m_main_grid.attach(*m_playlist_scrolled_window, 0, 0, 3, 1);
#ifdef SUPPORT_AUDIO_OUTPUT
  m_add_song_to_playlist_button.signal_clicked().connect(
      [this] { // if we support local audio
        auto file_dialog = Gtk::FileDialog::create(); // create file dialog when
                                                      // button "add" clicked
        file_dialog->open([file_dialog, this](
                              Glib::RefPtr<Gio::AsyncResult> &result) -> void {
          try {
            auto file = file_dialog->open_finish(
                result); // get file from result of dialog

            auto opened_file =
                Mix_LoadMUS(file->get_path().c_str()); // try to open it
            if (opened_file) { // and if it was audio file
              add_song_to_playlist(
                  file->get_path().c_str()); // add it to playlist
            }

          } catch (Glib::Error err) { // if user closed dialog
          }
        });
      });
  Mix_HookMusicFinished(
      &PlayerWindow::on_music_ends); // add signal what to do when music ends

  m_drop_target = Gtk::DropTarget::create(
      Gio::File::get_type(), Gdk::DragAction::COPY); // create drop_target
  m_conn_accept = m_drop_target->signal_accept().connect(
      [&](const std::shared_ptr<Gdk::Drop> &drop) -> gboolean {
        return on_signal_accept(drop);
      },
      false);
  m_conn_drop = m_drop_target->signal_drop().connect(
      [&](const Glib::ValueBase &value, double x, double y) {
        return on_signal_drop(value, x, y);
      },
      false);
  m_conn_leave =
      m_drop_target->signal_leave().connect([&] { return on_signal_leave(); });
  add_controller(m_drop_target); // add drop_target to main window

#endif

  if (m_player.get_current_player_name() != "Local") { // if player is not local
    m_playlist_scrolled_window->hide(); // then hide button and playlist
    m_add_song_to_playlist_button.hide();
  } else {
    set_default_size(500, 300); // if local, than set biggest size
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

#ifdef HAVE_DBUS
  if (m_player.get_current_player_name() != "Local") { // if player from DBus
    m_player.start_listening_signals(); // start listening signals
    m_player.get_song_data();           // and get current song data
  }
#endif
}

void PlayerWindow::on_playpause_clicked() {
#ifdef SUPPORT_AUDIO_OUTPUT

  if (m_player.get_current_player_name() == "Local" &&
      m_activated_row) { // some song is already chosen
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
        m_playlist_scrolled_window->get_child()->get_first_child());
    if (listbox) {
      int n_children = listbox->observe_children()->get_n_items();
      if (n_children != 0) {
        auto listitem = dynamic_cast<PlaylistRow *>(
            listbox->get_row_at_index(0));     // getting first item in playlist
        if (listitem) {                        // if first item is present
          m_current_track = 0;                 // set id of current track to 0
          if (m_activated_row)                 // if current activated
            m_activated_row->stop_highlight(); // stop current highlight
          m_activated_row = listitem;          // set activated to first
          m_activated_row->highlight();        // set highlight to first
          if (m_player.open_audio(
                  listitem->get_filename())) { // open audio from first item
            m_player.play_audio();             // play audio
          } else {
            std::cout << "Can't open " << listitem->get_filename()
                      << " as audio file." << std::endl;
          }
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
  m_player.send_play_pause(); // if not local, then send signal to dbus
}

void PlayerWindow::on_prev_clicked() {
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_player.get_current_player_name() == "Local") { // if local player
    std::cout << "Prev clicked" << std::endl;
    auto listbox = dynamic_cast<Gtk::ListBox *>(
        m_playlist_scrolled_window->get_child()->get_first_child());
    if (listbox) { // if found listbox
      int n_children =
          listbox->observe_children()->get_n_items(); // get count of rows
      for (int i = 0; i < n_children; i++) {
        auto listitem = dynamic_cast<PlaylistRow *>(
            listbox->get_row_at_index(i)); // get ith row
        if (listitem == m_activated_row) { // found current row
          std::cout << "Current song: " << listitem->get_filename()
                    << std::endl;
          auto prev_list_item = dynamic_cast<PlaylistRow *>(
              listbox->get_row_at_index(i - 1)); // get previous row
          if (prev_list_item) {                  // if found
            std::cout << "Prev song: " << prev_list_item->get_filename()
                      << std::endl;
            if (m_player.open_audio(
                    prev_list_item->get_filename())) { // then open previous
              if (m_player.get_is_playing())           // if song was playing
                m_player.play_audio();                 // play previous song
              m_current_track = prev_list_item->get_index();
              if (m_activated_row) // update highlightning
                m_activated_row->stop_highlight();
              m_activated_row = prev_list_item;
              m_activated_row->highlight();
              stop_flag = false;
              return;
            } else {
              std::cout << "Can't open " << prev_list_item->get_filename()
                        << " as audio file." << std::endl;
            }
          } else { // if no previous row
            auto last_list_item = dynamic_cast<PlaylistRow *>(
                listbox->get_row_at_index(n_children - 1)); // get last
            if (last_list_item) {
              std::cout << "Prev song: " << last_list_item->get_filename()
                        << std::endl;
              if (m_player.open_audio(
                      last_list_item->get_filename())) { // open last
                if (m_player.get_is_playing())
                  m_player.play_audio(); // play last
                if (m_activated_row)
                  m_activated_row->stop_highlight();
                m_current_track = last_list_item->get_index();
                m_activated_row = last_list_item;
                m_activated_row->highlight();
                stop_flag = false;
                return;
              } else {
                std::cout << "Can't open " << last_list_item->get_filename()
                          << " as audio file." << std::endl;
              }
            }
            return;
          }
        }
      }
      std::cout << "No song picked at all." << std::endl;
    }
    return;
  }
#endif
  m_player.send_previous();
}

void PlayerWindow::on_next_clicked() {
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_player.get_current_player_name() == "Local") { // if local player
    std::cout << "Next clicked" << std::endl;
    auto listbox =
        dynamic_cast<Gtk::ListBox *>(m_playlist_scrolled_window->get_child()
                                         ->get_first_child()); // get listbox
    if (listbox) {
      int n_children =
          listbox->observe_children()->get_n_items(); // get count of rows
      if (m_player.get_shuffle()) {                   // if shuffle enabled
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, n_children - 1);
        int new_track_index = dis(gen);
        while (m_current_track == new_track_index) { // generate new song
          new_track_index = dis(gen);
        }
        m_current_track = new_track_index;
        auto listitem = dynamic_cast<PlaylistRow *>(
            listbox->get_row_at_index(m_current_track)); // get random song
        if (m_activated_row)                             // update highlightning
          m_activated_row->stop_highlight();
        m_activated_row = listitem;
        m_activated_row->highlight();
        if (m_player.open_audio(
                m_activated_row->get_filename())) { // open new song
          if (m_player.get_is_playing())
            m_player.play_audio(); // and play if played before
        } else {
          std::cout << "Can't open " << m_activated_row->get_filename()
                    << " as audio file." << std::endl;
        }
      } else { // if shuffle disabled
        for (int i = 0; i < n_children; i++) {
          auto listitem =
              dynamic_cast<PlaylistRow *>(listbox->get_row_at_index(i));
          if (listitem == m_activated_row) { // find current song row
            std::cout << "Current song: " << listitem->get_filename()
                      << std::endl;
            auto next_list_item = dynamic_cast<PlaylistRow *>(
                listbox->get_row_at_index(i + 1)); // go to next
            if (m_player.get_repeat() == 1 &&
                !next_list_item) { // repeat playlist if it's last song
              next_list_item =
                  dynamic_cast<PlaylistRow *>(listbox->get_row_at_index(0));
            }
            if (next_list_item) { // if found next song
              std::cout << "Next song: " << next_list_item->get_filename()
                        << std::endl;
              if (m_player.open_audio(
                      next_list_item->get_filename())) { // open it
                if (m_player.get_is_playing())
                  m_player.play_audio(); // play
                if (m_activated_row)     // change highlight
                  m_activated_row->stop_highlight();
                m_activated_row = next_list_item;
                m_activated_row->highlight();
                m_current_track =
                    m_activated_row->get_index(); // change m_current_track
                stop_flag = false;
                return;
              } else {
                std::cout << "Can't open " << next_list_item->get_filename()
                          << " as audio file." << std::endl;
              }
            } else { // if current song is last
              auto last_list_item = dynamic_cast<PlaylistRow *>(
                  listbox->get_row_at_index(0)); // go to the last
              if (last_list_item) {
                std::cout << "Next song: " << last_list_item->get_filename()
                          << std::endl;
                if (m_player.open_audio(
                        last_list_item->get_filename())) { // open last
                  if (m_player.get_is_playing())
                    m_player.play_audio(); // play last
                  if (m_activated_row)     // update highlightning
                    m_activated_row->stop_highlight();
                  m_activated_row = last_list_item;
                  m_activated_row->highlight();
                  m_current_track = m_activated_row->get_index();
                  stop_flag = false;
                  return;
                } else {
                  std::cout << "Can't open " << last_list_item->get_filename()
                            << " as audio file." << std::endl;
                }
                return;
              }
            }
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
  bool current_shuffle = m_player.get_shuffle(); // get current shuffle
  m_player.set_shuffle(!current_shuffle);        // set opposite
}

void PlayerWindow::on_player_choose_clicked() {
  m_player_choose_popover.set_parent(
      m_player_choose_button); // set parent for popover
  Gtk::ListBox players_list;
  players_list.set_margin_bottom(5);
  players_list.set_halign(Gtk::Align::FILL);
  std::string selected_player_name = m_player.get_current_player_name();
  auto players_vec = m_player.get_players();

  for (const auto &pl : players_vec) { // for every player
    auto player_choosing_button = Gtk::ToggleButton(pl.first); // create button
    if (pl.first ==
        selected_player_name) { // if button is button for selected player
      player_choosing_button.set_active(); // set it active
    }
    player_choosing_button.set_has_frame(false);
    player_choosing_button.set_can_focus();
    player_choosing_button.set_halign(Gtk::Align::FILL);
    Gtk::ListBoxRow row; // create ListBoxRow
    row.set_selectable(false);
    row.set_child(player_choosing_button); // add button to id
    row.set_halign(Gtk::Align::FILL);
    row.set_valign(Gtk::Align::CENTER);
    players_list.append(row); // append row to listbox
  }
  Gtk::ToggleButton *first_button = dynamic_cast<Gtk::ToggleButton *>(
      players_list.get_row_at_index(0)->get_child()); // get first button
  for (unsigned short i = 0; i < players_vec.size(); i++) {
    Gtk::ListBoxRow *row = players_list.get_row_at_index(i);
    Gtk::ToggleButton *button =
        dynamic_cast<Gtk::ToggleButton *>(row->get_child()); // get ith button
    if (button) {                                            // if button
      if (button != first_button) {       // if ith button not first
        button->set_group(*first_button); // set group with first
      }
      button->signal_clicked().connect(sigc::bind( // bind clicked
          sigc::mem_fun(*this, &PlayerWindow::on_player_choosed), i));
    }
  }

  m_player_choose_popover.set_child(players_list);
  m_player_choose_popover.popup();
}

void PlayerWindow::on_player_choosed(unsigned short player_index) {
  m_player_choose_popover.popdown();    // close popup
  m_player.select_player(player_index); // select player
  if (m_player.get_shuffle()) {         // if shuffle enabled
    m_shuffle_button.get_style_context()->add_class(
        "shuffle-enabled"); // add class
  } else {
    m_shuffle_button.get_style_context()->remove_class(
        "shuffle-enabled"); // or remove if disabled
  }
  check_buttons_features(); // check what buttons must be accessible
  bool is_playing = m_player.get_playback_status(); // get is playing
  if (!is_playing) {                                // if not playing
    m_playpause_button.set_icon_name(
        "media-playback-start"); // set icon to start
  } else {                       // if playing
    m_playpause_button.set_icon_name(
        "media-playback-pause"); // set icon to pause
  }
  if (m_player.get_is_volume_prop()) {     // if player have volume property
    double volume = m_player.get_volume(); // get volume
    m_volume_bar_scale_button.set_value(volume); // set volume
  }

  m_player.get_song_data(); // get song data, artist and so on
  if (m_player.get_current_player_name() != "Local") {
#ifdef HAVE_DBUS
    m_player.start_listening_signals(); // if player not local, start listening
                                        // DBus signals
#endif
  }

#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_player.get_current_player_name() == "Local") { // if local player
    m_add_song_to_playlist_button.show(); // show button for adding to playlist
    m_playlist_scrolled_window->show();   // show playlist
    m_main_grid.set_valign(Gtk::Align::FILL);
    set_default_size(500, 300); // set biggest size
    m_conn_accept =
        m_drop_target->signal_accept().connect( // add signal for accepting dnd
            [&](const std::shared_ptr<Gdk::Drop> &drop) -> gboolean {
              return on_signal_accept(drop);
            },
            false);

    m_conn_drop =
        m_drop_target->signal_drop().connect( // add signal from dropping dnd
            [&](const Glib::ValueBase &value, double x, double y) {
              return on_signal_drop(value, x, y);
            },
            false);
    m_conn_leave = m_drop_target->signal_leave().connect(
        [&] { return on_signal_leave(); });
    add_controller(m_drop_target); // add dnd controller to window

  } else {                                // if player not local
    m_add_song_to_playlist_button.hide(); // hide playlist add button
    m_playlist_scrolled_window->hide();   // hide playlist
    m_main_grid.set_valign(Gtk::Align::END);
    set_default_size(500, 100); // set smallest size
    if (m_conn_accept.connected()) {
      std::cout << "Disconnecting accept" << std::endl;
      m_conn_accept.disconnect(); // disconnect from signal accept
    }
    if (m_conn_drop.connected()) {
      std::cout << "Disconnecting drop" << std::endl;
      m_conn_drop.disconnect(); // disconnect from signal drop
    }
    if (m_conn_leave.connected()) {
      std::cout << "Disconnecting leave" << std::endl;
      m_conn_leave.disconnect(); // disconnect from signal leave
    }
    remove_controller(m_drop_target); // remove controller
  }
#endif
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
  m_device_choose_popover.set_parent(
      m_device_choose_button); // set parent for popover
  Gtk::ListBox devices_list;
  devices_list.set_margin_bottom(5);
  devices_list.set_halign(Gtk::Align::FILL);
  unsigned short selected_device_sink_index =
      m_player.get_current_device_sink_index();
  std::cout << "Player selected device: " << selected_device_sink_index
            << std::endl;
  auto devices_vec = m_player.get_output_devices(); // get devices vector

  for (const auto &dev : devices_vec) { // for every device
    auto device_choosing_button = Gtk::ToggleButton(dev.first); // create button
    if (dev.second == selected_device_sink_index) { // if current button is for
                                                    // current audio device
      device_choosing_button.set_active();          // set it active
    }
    device_choosing_button.set_has_frame(false);
    device_choosing_button.set_can_focus();
    device_choosing_button.set_halign(Gtk::Align::FILL);
    Gtk::ListBoxRow row; // create row
    row.set_selectable(false);
    row.set_child(device_choosing_button); // add button to row
    row.set_halign(Gtk::Align::FILL);
    row.set_valign(Gtk::Align::CENTER);
    devices_list.append(row); // append row with button to devices list
  }
  Gtk::ToggleButton *first_button = dynamic_cast<Gtk::ToggleButton *>(
      devices_list.get_row_at_index(0)->get_child());        // get first button
  for (unsigned short i = 0; i < devices_vec.size(); i++) {  // go through
                                                             // buttons
    Gtk::ListBoxRow *row = devices_list.get_row_at_index(i); // get ith row
    Gtk::ToggleButton *button =
        dynamic_cast<Gtk::ToggleButton *>(row->get_child()); // get ith button
    if (button) {                         // if ith button is present
      if (button != first_button) {       // if ith button not first
        button->set_group(*first_button); // group it with first
      }
      button->signal_clicked().connect(
          sigc::bind(sigc::mem_fun(*this, &PlayerWindow::on_device_choosed),
                     devices_vec[i].second)); // bind signal to button
    }
  }

  m_device_choose_popover.set_child(
      devices_list);               // set devices list child for popover
  m_device_choose_popover.popup(); // show popover
}

void PlayerWindow::on_device_choosed(unsigned short device_sink_index) {
  m_device_choose_popover.popdown();             // close popover
  m_player.set_output_device(device_sink_index); // change output device
}

void PlayerWindow::on_loop_clicked() {
  int current_loop_status = m_player.get_repeat(); // get current loop status
  if (current_loop_status + 1 == 3) {              // if it last status
    m_player.set_repeat(0);                        // go to 0 status
  } else {
    m_player.set_repeat(current_loop_status + 1); // go to next status
  }
}

void PlayerWindow::check_buttons_features() {
  // just set buttons sensitivitly whether button method is available in player
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
  if (m_player.get_is_volume_prop()) {
    m_volume_bar_scale_button.set_sensitive(true);
  } else {
    m_volume_bar_scale_button.set_sensitive(false);
  }
}

void PlayerWindow::update_position_thread() {
  std::cout << "Started tracking position" << std::endl;
  m_mutex.lock();
  while (!stop_flag) { // while not signal to stop
    while (m_wait) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(500)); // wait 0.5 sec
    }
    // if song end (if paused, then thread will be stopped by stop flag)
    if (!m_player.get_playback_status()) {
      g_idle_add(
          [](gpointer data) -> gboolean {
            PlayerWindow *window = static_cast<PlayerWindow *>(data);
            window->m_progress_bar_song_scale.set_value(0.0);
            window->m_current_pos_label.set_label("00:00");
            window->m_playpause_button.set_icon_name("media-play");
            return false;
          },
          this);
      break;
    }

    g_idle_add(
        [](gpointer data) -> gboolean {
          PlayerWindow *window = static_cast<PlayerWindow *>(data);
          double current_pos =
              window->m_player.get_position(); // get current pos
          std::cout << "Current pos: " << current_pos << ", "
                    << m_player.get_position_str() << std::endl;
          window->m_current_pos_label.set_label(
              window->m_player.get_position_str()); // set label of current pos
          window->m_lock_pos_changing =
              true; // lock sending signal about pos changed again
          window->m_progress_bar_song_scale.set_value(
              current_pos /
              window->m_player.get_song_length());        // set new value
          window->m_progress_bar_song_scale.queue_draw(); // redraw progress_bar
          window->m_lock_pos_changing = false;            // unlock
          return false;
        },
        this);
    while (m_wait) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(500)); // wait 0.5 sec
    }
    m_mutex.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // wait 0.5 sec
    m_mutex.lock();
  }
  m_mutex.unlock();
  std::cout << "Thread stopped" << std::endl;
  stop_flag = false;
}

void PlayerWindow::pause_position_thread() { m_wait = true; }

void PlayerWindow::resume_position_thread() {
  m_wait = false;
  if (!m_position_thread.joinable()) { // if there is not thread
    stop_flag = false;
    m_position_thread = std::thread(&PlayerWindow::update_position_thread,
                                    this); // create new
  } else {
    stop_flag = true;
    m_position_thread.join(); // wait for thread to end
    stop_flag = false;
    m_position_thread = std::thread(&PlayerWindow::update_position_thread,
                                    this); // create new
  }
}

void PlayerWindow::stop_position_thread() {
  stop_flag = true; // set stop flag
  if (m_position_thread.joinable()) {
    m_position_thread.join(); // wait for end
  }
}

#ifdef SUPPORT_AUDIO_OUTPUT

gboolean PlayerWindow::on_signal_accept(const std::shared_ptr<Gdk::Drop> &) {
  set_cursor("wait"); // on accepting file just set mouse cursor to waiting
  return true;
}

// called when dropped file or folder
gboolean PlayerWindow::on_signal_drop(const Glib::ValueBase &value, double,
                                      double) {
  GFile *file_gobj = static_cast<GFile *>(g_value_get_object(value.gobj()));
  auto file_path = g_file_get_path(file_gobj); // get path of dropped file
  if (file_path) {                             // if it exists
    auto file_info = Gio::File::create_for_path(file_path)
                         ->query_file_type( // create file info object
                             Gio::FileQueryInfoFlags::NONE);

    if (file_info == Gio::FileType::DIRECTORY) { // if dropped directory
      // Add all files recursively
      add_directory_files_to_playlist(file_path);
      set_cursor(); // set cursor to default after added files to playlist
      return true;
    } else {                           // if dropped file
      add_song_to_playlist(file_path); // just add file
      set_cursor();
      return true;
    }
  }
  set_cursor("default");
  return false;
}

void PlayerWindow::on_signal_leave() {
  set_cursor(); // change cursor to default
}

void PlayerWindow::add_directory_files_to_playlist(
    const std::string &directory_path) {
  auto file = Gio::File::create_for_path(directory_path); // get file
  auto enumerator = file->enumerate_children();           // if it have children
  while (auto info = enumerator->next_file()) { // get while not got all
    auto child_file = file->get_child(info->get_name()); // get child file
    auto child_info = child_file->query_file_type(
        Gio::FileQueryInfoFlags::NONE);           // get child info
    if (child_info == Gio::FileType::DIRECTORY) { // if child info is directory
      add_directory_files_to_playlist(
          child_file->get_path());              // call the same function
    } else {                                    // if file
      auto child_path = child_file->get_path(); // get path
      std::cout << "File: " << child_path << std::endl;
      add_song_to_playlist(child_path); // and add this to playlist
    }
  }
}

void PlayerWindow::add_song_to_playlist(const std::string &filename) {
  Mix_Music *music = Mix_LoadMUS(filename.c_str()); // load music from filename
  if (!music) {                                     // if cannot load
    std::cout << "Mix_LoadMUS failed: " << Mix_GetError() << std::endl;
    return;
  }
  std::string song_title = Mix_GetMusicTitle(music);      // get title
  std::string song_artist = Mix_GetMusicArtistTag(music); // get artist
  std::string song_length = Helper::get_instance().format_time(
      Mix_MusicDuration(music)); // get song length

  m_playlist_listbox.append(*Gtk::make_managed<PlaylistRow>(
      song_title, song_artist, song_length, filename)); // create row
}

void PlayerWindow::on_music_ends() {
  std::cout << "On music ends" << std::endl;
  if (m_current_track == -1) {
    std::cout << "WARNING: m_current_track is unitialized." << std::endl;
    m_current_track = 0;
  }

  auto listbox =
      dynamic_cast<Gtk::ListBox *>(m_playlist_scrolled_window->get_child()
                                       ->get_first_child()); // get playlist
  int n_children = 0;
  if (listbox) {
    n_children =
        listbox->observe_children()->get_n_items(); // get size of playlist
  }
  if (m_player.get_repeat() == 2) { // if repeat current song enabled
    m_player.play_audio();          // just play again
    return;
  }
  if (m_player.get_shuffle()) { // if shuffle
    // Generate random number from 0 to n_children-1
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, n_children - 1);
    int new_track_index = dis(gen);
    while (m_current_track == new_track_index) { // generate new number
      new_track_index = dis(gen);
    }
    m_current_track = new_track_index; // set index
  } else {                             // if not shufflle
    m_current_track++;                 // just go to the next
  }
  m_activated_row->stop_highlight();
  auto next_list_item =
      dynamic_cast<PlaylistRow *>(listbox->get_row_at_index(m_current_track));
  if (!next_list_item) {              // if no next song
    if (m_player.get_repeat() == 1) { // if need to repeat playlist
      m_current_track = 0;            // go to the first
      auto first_list_item = dynamic_cast<PlaylistRow *>(
          listbox->get_row_at_index(m_current_track)); // get first
      if (first_list_item) {                           // if first present
        m_activated_row = first_list_item;
        m_activated_row->highlight(); // highlight
        if (m_player.open_audio(first_list_item->get_filename())) {
          m_player.play_audio(); // and play
        } else {
          std::cout << "Can't open " << first_list_item->get_filename()
                    << " as audio file." << std::endl;
        }
      }
    } else { // if no repeat playlist disabled, just turn off music
      Mix_HaltMusic();
    }
  } else { // if there is next
    m_activated_row = next_list_item;
    m_activated_row->highlight(); // highlight
    if (m_player.open_audio(next_list_item->get_filename())) {
      m_player.play_audio(); // play
    } else {
      std::cout << "Can't open " << next_list_item->get_filename()
                << " as audio file." << std::endl;
    }
  }
}
#endif
