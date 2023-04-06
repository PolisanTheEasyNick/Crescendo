#include "player.h"

inline const char *const bool_to_string(bool b) { return b ? "true" : "false"; }

Player::Player() {

  m_song_title = "";
  m_song_artist = "";
  m_song_length_str = "";
  m_song_pos = 0;
  m_song_length = 0;
  m_is_shuffle = false;
  m_is_playing = false;
  m_song_volume = 0;
#ifdef HAVE_DBUS
  m_dbus_conn = sdbus::createSessionBusConnection();
  if (m_dbus_conn)
    std::cout << "Connected to D-Bus as \"" << m_dbus_conn->getUniqueName()
              << "\"." << std::endl;
#endif
  get_players();
  if (m_players.size() != 0) {
    if (select_player(0)) {
      if (m_players[m_selected_player_id].first == "Local") {
        std::cout << "Selected local player." << std::endl;
      } else {
        std::cout << "Selected player: "
                  << m_players[m_selected_player_id].first << " at "
                  << m_players[m_selected_player_id].second << std::endl;
        get_song_data();
      }
    };
  }

#ifdef SUPPORT_AUDIO_OUTPUT
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }
  if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096) < 0) {
    std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }
#endif
}

Player::~Player() {
#ifdef HAVE_DBUS
  m_proxy_signal.reset();
#endif
#ifdef SUPPORT_AUDIO_OUTPUT
  Mix_FreeMusic(m_current_music);
  Mix_CloseAudio();
  SDL_Quit();
#endif
}

std::vector<std::pair<std::string, std::string>> Player::get_players() {
  m_players.clear();
#ifdef SUPPORT_AUDIO_OUTPUT
  m_players.push_back(std::make_pair("Local", ""));
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't get players. Aborting."
              << std::endl;
    return {};
  }
  std::vector<std::string> result_of_call;
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(), "org.freedesktop.DBus",
                                    "/org/freedesktop/DBus");
    proxy->callMethod("ListNames")
        .onInterface("org.freedesktop.DBus")
        .storeResultsTo(result_of_call);
  } catch (const sdbus::Error &e) {
    std::cout << "Error while getting all DBus services: " << e.what()
              << std::endl;
    return {};
  }

  std::cout << "Start checking" << std::endl;
  if (result_of_call.empty()) {
    std::cout << "Error getting reply." << std::endl;
    return {};
  } else {
    std::cout << "Got reply" << std::endl;
    int num_names = 0;
    for (const auto &name : result_of_call) {
      if (strstr(name.c_str(), "org.mpris.MediaPlayer2.") == name.c_str()) {
        std::cout << "Found media player: " << name << std::endl;
        std::string identity;
        try {
          auto proxy = sdbus::createProxy(*m_dbus_conn.get(), name,
                                          "/org/mpris/MediaPlayer2");
          sdbus::Variant v_identity;
          proxy->callMethod("Get")
              .onInterface("org.freedesktop.DBus.Properties")
              .withArguments("org.mpris.MediaPlayer2", "Identity")
              .storeResultsTo(v_identity);
          identity = v_identity.get<std::string>();
        } catch (const sdbus::Error &e) {
          std::cout << "Error while getting Identity: " << e.what()
                    << std::endl;
          m_players.push_back(std::make_pair("Player", name));
          return {};
        }
        std::cout << "Identity: " << identity << std::endl;
        m_players.push_back(std::make_pair(identity, name));
      }
    }
  }
  if (m_players.empty()) {
    std::cout << "No media players found." << std::endl;
    return {};
  }
#endif
  return m_players;
}

void Player::print_players() {
  for (auto &player : m_players) {
    std::cout << player.first << ": " << player.second << std::endl;
  }
}

void Player::print_players_names() {
  for (auto &player : m_players) {
    std::cout << player.first << std::endl;
  }
}

bool Player::select_player(unsigned int new_id) {
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't get players. Aborting."
              << std::endl;
    return false;
  }
#endif
  get_players();
  if (new_id < 0 || new_id > m_players.size()) {
    std::cout << "This player does not exists!" << std::endl;
    return false;
  }

  m_selected_player_id = new_id;
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    m_play_pause_method = true;
    m_pause_method = true;
    m_play_method = true;
    m_next_method = true;
    m_previous_method = true;
    m_setpos_method = true;
    m_is_shuffle_prop = true;
    m_is_pos_prop = true;
    m_is_volume_prop = true;
    m_is_playback_status_prop = true;
    m_is_metadata_prop = true;
#ifdef HAVE_DBUS
    stop_listening_signals();
#endif
    return true;
  } else {
    pause_audio();
  }
#endif
#ifdef HAVE_DBUS
  std::string xml_introspect;
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Introspect")
        .onInterface("org.freedesktop.DBus.Introspectable")
        .storeResultsTo(xml_introspect);
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying to get all functions for player: "
              << e.what() << std::endl;
  }

  // start parsing and checking
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(xml_introspect.c_str());
  if (!result) {
    std::cout << "Failed to parse XML: " << result.description() << std::endl;
    return false;
  }
  // Find the root node
  pugi::xml_node root = doc.document_element();
  if (!root) {
    std::cout << "Error: no root element" << std::endl;
    return 1;
  }

  pugi::xml_node iface_node =
      doc.select_node(
             (std::string(
                  "/node/interface[@name='org.mpris.MediaPlayer2.Player']")
                  .c_str()))
          .node();
  // Searching for play_pause method...
  std::cout << "Searching for PlayPause method... ";
  pugi::xml_node methodNode =
      iface_node.find_child_by_attribute("method", "name", "PlayPause");
  if (methodNode) {
    // Method found
    std::cout << "found." << std::endl;
    m_play_pause_method = true;
  } else {
    // Method not found
    std::cout << "not found." << std::endl;
    m_play_pause_method = false;
  }

  // Searching for pause method...
  std::cout << "Searching for Pause method... ";
  methodNode = iface_node.find_child_by_attribute("method", "name", "Pause");
  if (methodNode) {
    // Method found
    std::cout << "found." << std::endl;
    m_pause_method = true;
  } else {
    // Method not found
    std::cout << "not found." << std::endl;
    m_pause_method = false;
  }

  // Searching for Play method
  std::cout << "Searching for Play method... ";
  methodNode = iface_node.find_child_by_attribute("method", "name", "Play");
  if (methodNode) {
    // Method found
    std::cout << "found." << std::endl;
    m_play_method = true;
  } else {
    // Method not found
    std::cout << "not found." << std::endl;
    m_play_method = false;
  }

  // Searching for Next method
  std::cout << "Searching for Next method... ";
  methodNode = iface_node.find_child_by_attribute("method", "name", "Next");
  if (methodNode) {
    // Method found
    std::cout << "found." << std::endl;
    m_next_method = true;
  } else {
    // Method not found
    std::cout << "not found." << std::endl;
    m_next_method = false;
  }

  // Searching for Previous method
  std::cout << "Searching for Previous method... ";
  methodNode = iface_node.find_child_by_attribute("method", "name", "Previous");
  if (methodNode) {
    // Method found
    std::cout << "found." << std::endl;
    m_previous_method = true;
  } else {
    // Method not found
    std::cout << "not found." << std::endl;
    m_previous_method = false;
  }

  // Searching for SetPosition method
  std::cout << "Searching for SetPosition method... ";
  methodNode =
      iface_node.find_child_by_attribute("method", "name", "SetPosition");
  if (methodNode) {
    // Method found
    std::cout << "found." << std::endl;
    m_setpos_method = true;
  } else {
    // Method not found
    std::cout << "not found." << std::endl;
    m_setpos_method = false;
  }

  // Searching for Shuffle property
  std::cout << "Searching for Shuffle property... ";
  pugi::xml_node propertyNode =
      iface_node.find_child_by_attribute("property", "name", "Shuffle");
  if (propertyNode) {
    // Property found
    std::cout << "found." << std::endl;
    m_is_shuffle_prop = true;
  } else {
    // Property not found
    std::cout << "not found." << std::endl;
    m_is_shuffle_prop = false;
  }

  // Searching for Position property
  std::cout << "Searching for Position property... ";
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "Position");
  if (propertyNode) {
    // Property found
    std::cout << "found." << std::endl;
    m_is_pos_prop = true;
  } else {
    // Property not found
    std::cout << "not found." << std::endl;
    m_is_pos_prop = false;
  }

  // Searching for Volume property
  std::cout << "Searching for Volume property... ";
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "Volume");
  if (propertyNode) {
    // Property found
    std::cout << "found." << std::endl;
    m_is_volume_prop = true;
  } else {
    // Property not found
    std::cout << "not found." << std::endl;
    m_is_volume_prop = false;
  }

  // Searching for PlaybackStatus property
  std::cout << "Searching for PlaybackStatus property... ";
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "PlaybackStatus");
  if (propertyNode) {
    // Property found
    std::cout << "found." << std::endl;
    m_is_playback_status_prop = true;
  } else {
    // Property not found
    std::cout << "not found." << std::endl;
    m_is_playback_status_prop = false;
  }

  // Searching for Metadata property
  std::cout << "Searching for Metadata property... ";
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "Metadata");
  if (propertyNode) {
    // Property found
    std::cout << "found." << std::endl;
    m_is_metadata_prop = true;
  } else {
    // Property not found
    std::cout << "not found." << std::endl;
    m_is_metadata_prop = false;
  }
  start_listening_signals();
#endif
  return true;
}

bool Player::send_play_pause() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    if (Mix_PlayingMusic() && Mix_PausedMusic()) {
      play_audio();
    } else if (Mix_PlayingMusic() && !Mix_PausedMusic()) {
      pause_audio();
    } else {
      std::cout << "Starting playing" << std::endl;
      play_audio();
    }
    return true;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't send PlayPause. Aborting."
              << std::endl;
    return false;
  }
  if (!m_play_pause_method) {
    std::cerr << "This player does not compatible with PlayPause method!"
              << std::endl;
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("PlayPause")
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying call PlayPause method: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

bool Player::send_pause() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    pause_audio();
    return true;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't send Pause. Aborting."
              << std::endl;
    return false;
  }
  if (!m_pause_method) {
    std::cerr << "This player does not compatible with Pause method!"
              << std::endl;
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Pause")
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying call Pause method: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

bool Player::send_play() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    play_audio();
    return true;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't send Play. Aborting."
              << std::endl;
    return false;
  }
  if (!m_play_method) {
    std::cerr << "This player does not compatible with Play method!"
              << std::endl;
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Play")
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying call Play method: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

bool Player::send_next() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't send Next. Aborting."
              << std::endl;
    return false;
  }
  if (!m_next_method) {
    std::cerr << "This player does not compatible with Next method!"
              << std::endl;
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Next")
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying call Next method: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

bool Player::send_previous() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't send Previous. Aborting."
              << std::endl;
    return false;
  }
  if (!m_previous_method) {
    std::cerr << "This player does not compatible with Previous method!"
              << std::endl;
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Previous")
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying call Previous method: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

bool Player::get_shuffle() {
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return m_is_shuffle;
  }
#endif
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't get Shuffle. Aborting."
              << std::endl;
    return false;
  }
  if (!m_is_shuffle_prop) {
    std::cerr << "This player does not compatible with Shuffle property!"
              << std::endl;
    return false;
  }
  if (m_selected_player_id == -1) {
    std::cout << "get_shuffle(): Player not selected, can't get shuffle"
              << std::endl;
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant current_shuffle_v;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player", "Shuffle")
        .storeResultsTo(current_shuffle_v);
    bool current_shuffle = current_shuffle_v.get<bool>();
    return current_shuffle;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying to get Shuffle property: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

bool Player::set_shuffle(bool isShuffle) {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    m_is_shuffle = isShuffle;
    notify_observers_is_shuffle_changed();
    return true;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't set Shuffle. Aborting."
              << std::endl;
    return false;
  }
  if (!m_is_shuffle_prop) {
    std::cerr << "This player does not compatible with Shuffle property!"
              << std::endl;
    return false;
  }
  bool current_shuffle = get_shuffle();
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Set")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player", "Shuffle",
                       sdbus::Variant(!current_shuffle))
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying to set Shuffle property: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

int64_t Player::get_position() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return 0;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return Mix_GetMusicPosition(m_current_music);
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't set Position. Aborting."
              << std::endl;
    return 0;
  }
  if (!m_is_pos_prop) {
    std::cerr << "This player does not compatible with Position property!"
              << std::endl;
    return 0;
  }

  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant position_v;
    int64_t position;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player", "Position")
        .storeResultsTo(position_v);
    position = position_v.get<int64_t>();
    return position / 1000000;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying to get Position property: " << e.what()
              << std::endl;
    return 0;
  }
#endif
  return 0;
}

std::string Player::get_position_str() {
  return Helper::get_instance().format_time(get_position());
}

bool Player::set_position(int64_t pos) {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    int res = Mix_SetMusicPosition(pos);
    return res == 0;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't set Position. Aborting."
              << std::endl;
    return false;
  }
  if (!m_setpos_method) {
    std::cerr << "This player does not compatible with SetPosition method!"
              << std::endl;
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    auto meta = get_metadata();
    sdbus::ObjectPath trackid;
    for (auto &info : meta) {
      if (info.first == "mpris:trackid") {
        trackid = info.second;
        break;
      }
    }
    proxy->callMethod("SetPosition")
        .onInterface("org.mpris.MediaPlayer2.Player")
        .withArguments(trackid, pos);
    return true;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying to set Position: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

uint64_t Player::get_song_length() {
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return m_song_length;
  }
#endif
  auto metadata = get_metadata();
  for (auto &info : metadata) {
    if (info.first == "mpris:length") {
      int64_t length = stoi(info.second);
#ifdef SUPPORT_AUDIO_OUTPUT
      if (m_players[m_selected_player_id].first != "Local") {
        length /= 1000000;
      }
#endif
      return length;
    }
  }

  return 0;
}

double Player::get_volume() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return 0;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return Mix_GetMusicVolume(m_current_music) / 128;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't set Volume. Aborting."
              << std::endl;
    return 0;
  }
  if (!m_is_volume_prop) {
    std::cerr << "This player does not compatible with Volume property!"
              << std::endl;
    return 0;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant volume_v;
    double volume;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player", "Volume")
        .storeResultsTo(volume_v);
    volume = volume_v.get<double>();
    return volume;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying to get Position property: " << e.what()
              << std::endl;
    return 0;
  }
#endif
  return 0;
}

bool Player::set_volume(double volume) {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    int newVolume = 128 * volume;
    Mix_VolumeMusic(newVolume);
    return true;
  }
#endif
  if (!m_is_volume_prop) {
    std::cerr << "This player does not compatible with Volume property!"
              << std::endl;
    return false;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't set Volume. Aborting."
              << std::endl;
    return false;
  }
  if (!m_is_shuffle_prop) {
    std::cerr << "This player does not compatible with Volume property!"
              << std::endl;
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Set")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player", "Volume",
                       sdbus::Variant(volume))
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying to set Volume property: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return true;
}

bool Player::get_playback_status() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return Mix_PlayingMusic() && !Mix_PausedMusic();
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't get PlayBack Status. Aborting."
              << std::endl;
    return false;
  }
  if (!m_is_playback_status_prop) {
    std::cerr << "This player does not compatible with PlayBack property!"
              << std::endl;
    return false;
  }

  std::string playback_str;
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant playback_v;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player", "PlaybackStatus")
        .storeResultsTo(playback_v);
    playback_str = playback_v.get<std::string>();
    return "Playing" == playback_str;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while trying to set PlayBack property: " << e.what()
              << std::endl;
    return false;
  }
#endif
  return false;
}

std::string Player::get_current_player_name() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return "";
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return "Local";
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't get Identity. Aborting."
              << std::endl;
    return "";
  }
  try {
    auto proxy = sdbus::createProxy(
        *m_dbus_conn.get(), m_players[m_selected_player_id].second.c_str(),
        "/org/mpris/MediaPlayer2");
    sdbus::Variant v_identity;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2", "Identity")
        .storeResultsTo(v_identity);
    std::string identity = v_identity.get<std::string>();
    return identity;
  } catch (const sdbus::Error &e) {
    std::cout << "Error while getting current player name: " << e.what()
              << std::endl;
    return "";
  }
#endif
  return "";
}

unsigned short Player::get_current_device_sink_index() {
#ifdef HAVE_PULSEAUDIO
  // Code thatuse PulseAudio
  std::cout << "PulseAudio installed" << std::endl;
#else
  // Code that doesn't uses PulseAudio
  std::cout << "PulseAudio not installed, can't continue." << std::endl;
  return -1;
#endif
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return false;
  }
  static uint32_t proc_id = 0;
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    proc_id = getpid();
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout
        << "Not connected to DBus, can't get player's process id. Aborting."
        << std::endl;
    return -1;
  }
  if (m_players[m_selected_player_id].first != "Local") {
    try {
      auto proxy = sdbus::createProxy(
          *m_dbus_conn.get(), "org.freedesktop.DBus", "/org/freedesktop/DBus");
      proxy->callMethod("GetConnectionUnixProcessID")
          .onInterface("org.freedesktop.DBus")
          .withArguments(m_players[m_selected_player_id].second)
          .storeResultsTo(proc_id);
    } catch (const sdbus::Error &e) {
      std::cout << "Error while getting current device process id: " << e.what()
                << std::endl;
      return -1;
    }
  }
#endif
  static int player_sink_id = -1;
#ifdef HAVE_PULSEAUDIO
  // Create a main loop object
  pa_mainloop *mainloop = pa_mainloop_new();

  // Create a new PulseAudio context
  pa_context *context =
      pa_context_new(pa_mainloop_get_api(mainloop), "pulse_get_sink_index");

  // Connect to the PulseAudio server
  if (pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
    std::cerr << "pa_context_connect() failed: "
              << pa_strerror(pa_context_errno(context)) << std::endl;
    pa_context_unref(context);
    pa_mainloop_free(mainloop);
    return -1;
  }

  // Wait for the context to be ready
  while (pa_context_get_state(context) != PA_CONTEXT_READY) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }

  // Get the list of sink devices
  static std::string player_name = m_players[m_selected_player_id].first;
  pa_operation *operation = pa_context_get_sink_input_info_list(
      context,
      [](pa_context *context, const pa_sink_input_info *info, int eol,
         void *userdata) {
        if (eol == 0) {
          std::cout << "Sink input #" << info->index << ":" << std::endl;
          std::cout << "    Name: " << info->name << std::endl;
          std::cout << "    Application name: "
                    << pa_proplist_gets(info->proplist, "application.name")
                    << std::endl;
          std::cout << "    Client index: " << info->client << std::endl;
          std::cout << "    Sink index: " << info->sink << std::endl;
          std::cout << "    App Binary: "
                    << pa_proplist_gets(info->proplist,
                                        "application.process.binary")
                    << std::endl;
          std::cout << "    Process id: "
                    << pa_proplist_gets(info->proplist,
                                        "application.process.id")
                    << std::endl;
          std::cout << "comparing \""
                    << pa_proplist_gets(info->proplist, "application.name")
                    << "\" and \"" << player_name << "\"" << std::endl;
          char player_proc_id_str[10];
          snprintf(player_proc_id_str, sizeof(player_proc_id_str), "%u",
                   proc_id);
          std::cout << "procid: " << player_proc_id_str << std::endl;
          if (strcmp(pa_proplist_gets(info->proplist, "application.process.id"),
                     player_proc_id_str) == 0 ||
              strcmp(pa_proplist_gets(info->proplist, "application.name"),
                     player_name.c_str()) == 0) {
            std::cout << "Found current player output sink: " << player_name
                      << ". #" << info->sink << std::endl;
            int *sink_id = static_cast<int *>(userdata);
            *sink_id = info->sink;
            return;
          }
        }
      },
      &player_sink_id);
  if (!operation) {
    std::cerr << "Failed to get player sink." << std::endl;
    pa_context_disconnect(context);
    pa_mainloop_free(mainloop);
    return -1;
  }

  // Wait for the operation to complete
  while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }
#endif
  if (player_sink_id == -1) {
    std::cout << "Not found player sink. Can't continue." << std::endl;
    return -1;
  }
  return player_sink_id;
}

std::vector<std::pair<std::string, std::string>> Player::get_metadata() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return {};
  }
  std::vector<std::pair<std::string, std::string>> metadata;
#ifdef SUPPORT_AUDIO_OUTPUT
  if (get_current_player_name() == "Local") {
    std::cout << "Music filename: " << Mix_GetMusicTitle(m_current_music)
              << std::endl;
    metadata.push_back(
        std::make_pair("xesam:title", Mix_GetMusicTitle(m_current_music)));
    metadata.push_back(
        std::make_pair("xesam:artist", Mix_GetMusicArtistTag(m_current_music)));
    metadata.push_back(std::make_pair(
        "mpris:length", std::to_string(Mix_MusicDuration(m_current_music))));
    return metadata;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout << "Not connected to DBus, can't get metadata. Aborting."
              << std::endl;
    return {};
  }
  if (!m_is_metadata_prop) {
    std::cerr << "This player does not compatible with Metadata property!"
              << std::endl;
    return {};
  }
  if (m_selected_player_id == -1) {
    std::cout << "Player not selected, can't get metadata" << std::endl;
    return {};
  }

  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant metadata_v;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player", "Metadata")
        .storeResultsTo(metadata_v);
    auto meta = metadata_v.get<std::map<std::string, sdbus::Variant>>();
    std::map<std::string, int> type_map = {{"n", 1},    // int16
                                           {"q", 2},    // uint16
                                           {"i", 3},    // int32
                                           {"u", 4},    // uint32
                                           {"x", 5},    // int64
                                           {"t", 6},    // uint64
                                           {"d", 7},    // double
                                           {"s", 8},    // string
                                           {"o", 9},    // object path
                                           {"b", 10},   // boolean
                                           {"as", 11}}; // array of strings

    for (auto &data : meta) {
      std::string type = data.second.peekValueType();
      switch (type_map[type]) {
      case 0: {
        std::cout << "Warning: not implemented parsing for type \"" << type
                  << "\", skipping " << data.first << std::endl;
      }
      case 1: { // int16
        try {
          int16_t num = data.second.get<int16_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch int16: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"n\" expected."
                    << std::endl;
          break;
        }

        break;
      }
      case 2: { // uint16
        try {
          uint16_t num = data.second.get<uint16_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch uint16: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"q\" expected."
                    << std::endl;
          break;
        }

        break;
      }
      case 3: { // int32
        try {
          int32_t num = data.second.get<int32_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch int32: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"i\" expected."
                    << std::endl;
          break;
        }

        break;
      }
      case 4: { // uint32
        try {
          uint32_t num = data.second.get<uint32_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch uint32: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"i\" expected."
                    << std::endl;
          break;
        }

        break;
      }
      case 5: { // int64
        try {
          int64_t num = data.second.get<int64_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch int64: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"x\" expected."
                    << std::endl;
          break;
        }

        break;
      }
      case 6: { // uint64
        try {
          uint64_t num = data.second.get<uint64_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch uint64: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"d\" expected."
                    << std::endl;
          break;
        }

        break;
      }
      case 7: { // double
        try {
          double num = data.second.get<double>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch double: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"t\" expected."
                    << std::endl;
          break;
        }

        break;
      }
      case 8: { // string
        try {
          std::string str = data.second.get<std::string>();
          metadata.push_back(std::make_pair(data.first, str));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch string: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"s\" expected."
                    << std::endl;
          break;
        }

        break;
      }
      case 9: { // object path
        try {
          std::string path = data.second.get<sdbus::ObjectPath>();
          metadata.push_back(std::make_pair(data.first, path));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch object path: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"o\" expected."
                    << std::endl;
          break;
        }
        break;
      }
      case 10: { // boolean
        try {
          bool boolean = data.second.get<bool>();
          metadata.push_back(
              std::make_pair(data.first, bool_to_string(boolean)));
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch boolean: " << e.what()
                    << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"b\" expected."
                    << std::endl;
          break;
        }
        break;
      }
      case 11: { // array of strings
        try {
          std::vector<std::string> arr =
              data.second.get<std::vector<std::string>>();
          for (auto &entry : arr) {
            metadata.push_back(std::make_pair(data.first, entry));
          }
        } catch (const sdbus::Error &e) {
          std::cout << "Error while trying to fetch array of strings: "
                    << e.what() << std::endl;
          std::cout << "Received type: \"" << type << "\" while \"b\" expected."
                    << std::endl;
          break;
        }
        break;
      }
      default: {
        std::cout << "Got not implemented data type: " << type << ", skipping "
                  << data.first << std::endl;
        break;
      }
      }
    }
  } catch (const sdbus::Error &e) {
    std::cout << "Error while getting metadata: " << e.what() << std::endl;
    return {};
  }
#endif
  return metadata;
}

std::vector<std::pair<std::string, unsigned short>>
Player::get_output_devices() {
#ifdef HAVE_PULSEAUDIO
  // Code thatuse PulseAudio
  std::cout << "PulseAudio installed" << std::endl;
#else
  // Code that doesn't uses PulseAudio
  std::cout << "PulseAudio not installed, can't continue." << std::endl;
  return {};
#endif
  m_devices.clear();
#ifdef HAVE_PULSEAUDIO
  // Create a main loop and a new PulseAudio context
  pa_mainloop *mainloop = pa_mainloop_new();
  pa_context *context =
      pa_context_new(pa_mainloop_get_api(mainloop), "check_sinks");

  // Connect to the PulseAudio server
  pa_context_connect(context, NULL, PA_CONTEXT_NOFAIL, NULL);

  // Wait for the context to be ready
  while (pa_context_get_state(context) != PA_CONTEXT_READY) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }

  // Get the list of sink devices
  pa_operation *operation = pa_context_get_sink_info_list(
      context,
      [](pa_context *context, const pa_sink_info *info, int eol,
         void *userdata) {
        if (eol < 0) {
          // Error
          std::cerr << "Failed to get sink info list: "
                    << pa_strerror(pa_context_errno(context)) << std::endl;
          pa_context_disconnect(context);
        } else if (eol == 0) {
          // Sink device
          auto devices = static_cast<
              std::vector<std::pair<std::string, unsigned short>> *>(userdata);
          devices->emplace_back(info->description, info->index);
        }
      },
      &m_devices);
  if (!operation) {
    std::cerr << "Failed to get sink info list." << std::endl;
    pa_context_disconnect(context);
    pa_mainloop_free(mainloop);
    return {};
  }

  // Wait for the operation to complete
  while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }

  for (auto &device : m_devices) {
    std::cout << "Sink name: " << device.first << std::endl;
    std::cout << "Sink index: " << device.second << std::endl;
  }

  pa_context_disconnect(context);
  pa_context_unref(context);
  pa_mainloop_free(mainloop);
#endif
  return m_devices;
}

void Player::set_output_device(unsigned short output_sink_index) {
#ifdef HAVE_PULSEAUDIO
  std::cout << "PulseAudio installed" << std::endl;
#else
  std::cout << "PulseAudio not installed, can't continue." << std::endl;
  return;
#endif
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    std::cout << "Player not selected, can't continue." << std::endl;
    return;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    std::cout
        << "Not connected to DBus, can't get player's process id. Aborting."
        << std::endl;
    return;
  }
#endif
  static uint32_t proc_id = 0;
#ifdef SUPPORT_AUDIO_OUTPUT
  if (get_current_player_name() == "Local")
    proc_id = getpid();
#endif
#ifdef HAVE_DBUS
  if (get_current_player_name() != "Local") {
    try {
      auto proxy = sdbus::createProxy(
          *m_dbus_conn.get(), "org.freedesktop.DBus", "/org/freedesktop/DBus");
      proxy->callMethod("GetConnectionUnixProcessID")
          .onInterface("org.freedesktop.DBus")
          .withArguments(m_players[m_selected_player_id].second)
          .storeResultsTo(proc_id);
    } catch (const sdbus::Error &e) {
      std::cout << "Error while getting current device process id: " << e.what()
                << std::endl;
      return;
    }
  }
#endif
  if (proc_id == 0) {
    std::cout << "Error while getting player's process id, can't continue."
              << std::endl;
    return;
  }
#ifdef HAVE_PULSEAUDIO
  // Create a main loop object
  pa_mainloop *mainloop = pa_mainloop_new();

  // Create a new PulseAudio context
  pa_context *context =
      pa_context_new(pa_mainloop_get_api(mainloop), "example");

  // Connect to the PulseAudio server
  if (pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
    std::cerr << "pa_context_connect() failed: "
              << pa_strerror(pa_context_errno(context)) << std::endl;
    pa_context_unref(context);
    pa_mainloop_free(mainloop);
    return;
  }

  // Wait for the context to be ready
  while (pa_context_get_state(context) != PA_CONTEXT_READY) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }
  static int player_sink_id = -1;
  // Get the list of sink devices
  static std::string player_name = m_players[m_selected_player_id].first;
  pa_operation *operation = pa_context_get_sink_input_info_list(
      context,
      [](pa_context *context, const pa_sink_input_info *info, int eol,
         void *userdata) {
        if (eol == 0) {
          std::cout << "Sink input #" << info->index << ":" << std::endl;
          std::cout << "    Name: " << info->name << std::endl;
          std::cout << "    Application name: "
                    << pa_proplist_gets(info->proplist, "application.name")
                    << std::endl;
          std::cout << "    Client index: " << info->client << std::endl;
          std::cout << "    Sink index: " << info->sink << std::endl;
          std::cout << "    App Binary: "
                    << pa_proplist_gets(info->proplist,
                                        "application.process.binary")
                    << std::endl;
          std::cout << "    Process id: "
                    << pa_proplist_gets(info->proplist,
                                        "application.process.id")
                    << std::endl;
          std::cout << "comparing \""
                    << pa_proplist_gets(info->proplist, "application.name")
                    << "\" and \"" << player_name << "\"" << std::endl;
          char player_proc_id_str[10];
          snprintf(player_proc_id_str, sizeof(player_proc_id_str), "%u",
                   proc_id);
          std::cout << "procid: " << player_proc_id_str << std::endl;
          if (strcmp(pa_proplist_gets(info->proplist, "application.process.id"),
                     player_proc_id_str) == 0 ||
              strcmp(pa_proplist_gets(info->proplist, "application.name"),
                     player_name.c_str()) == 0) {
            std::cout << "Found current player: " << player_name << ". #"
                      << info->index << std::endl;
            int *sink_id = static_cast<int *>(userdata);
            *sink_id = info->index;
            // info->client
            return;
          }
        }
      },
      &player_sink_id);
  if (!operation) {
    std::cerr << "Failed to get player sink." << std::endl;
    pa_context_disconnect(context);
    pa_mainloop_free(mainloop);
    return;
  }

  // Wait for the operation to complete
  while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }

  if (player_sink_id == -1) {
    std::cout << "Not found player sink. Can't continue." << std::endl;
    return;
  }

  std::cout << "Trying to change output device for sink " << player_sink_id
            << " to " << output_sink_index << std::endl;
  static const unsigned short output_sink_path_copy = output_sink_index;

  // Get the list of sink devices
  pa_operation *operation2 = pa_context_move_sink_input_by_index(
      context, player_sink_id, output_sink_index, NULL, NULL);
  if (!operation2) {
    std::cout << "Failed to set sink output device." << std::endl;
    pa_context_disconnect(context);
    pa_mainloop_free(mainloop);
    return;
  }
  // Wait for the operation to complete
  while (pa_operation_get_state(operation2) == PA_OPERATION_RUNNING) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }
  pa_operation_unref(operation);
  pa_operation_unref(operation2);
  pa_context_disconnect(context);
  pa_context_unref(context);
  pa_mainloop_free(mainloop);
#endif
}

bool Player::get_play_pause_method() const { return m_play_pause_method; }

bool Player::get_pause_method() const { return m_pause_method; }

bool Player::get_play_method() const { return m_play_method; }

bool Player::get_next_method() const { return m_next_method; }

bool Player::get_previous_method() const { return m_previous_method; }

bool Player::get_setpos_method() const { return m_setpos_method; }

bool Player::get_is_shuffle_prop() const { return m_is_shuffle_prop; }

bool Player::get_is_pos_prop() const { return m_is_pos_prop; }

bool Player::get_is_volume_prop() const { return m_is_volume_prop; }

bool Player::get_is_playback_status_prop() const {
  return m_is_playback_status_prop;
}

bool Player::get_is_metadata_prop() const { return m_is_metadata_prop; }

unsigned int Player::get_count_of_players() const { return m_players.size(); }

bool Player::get_is_playing() const { return m_is_playing; }

void Player::set_is_playing(bool new_is_playing) {
  m_is_playing = new_is_playing;
  notify_observers_is_playing_changed();
}

#ifdef HAVE_DBUS
void Player::stop_listening_signals() { m_proxy_signal.reset(); }

void Player::start_listening_signals() {
  stop_listening_signals();
  m_proxy_signal = sdbus::createProxy(*m_dbus_conn.get(),
                                      m_players[m_selected_player_id].second,
                                      "/org/mpris/MediaPlayer2");
  m_proxy_signal->registerSignalHandler(
      "org.freedesktop.DBus.Properties", "PropertiesChanged",
      [this](sdbus::Signal &sig) { on_properties_changed(sig); });
  m_proxy_signal->registerSignalHandler(
      "org.mpris.MediaPlayer2.Player", "Seeked",
      [this](sdbus::Signal &sig) { on_seeked(sig); });
  m_proxy_signal->finishRegistration();

  // Start the event loop in a new thread
  m_dbus_conn->enterEventLoopAsync();
  std::cout << "Event loop started" << std::endl;
}

void Player::on_properties_changed(sdbus::Signal &signal) {
  std::map<std::string, int> property_map = {
      {"Shuffle", 1},         // Shuffle
      {"Metadata", 2},        // Changed song
      {"Volume", 3},          // changed Volume
      {"PlaybackStatus", 4}}; // paused or played

  // Handle the PropertiesChanged signal
  std::cout << "Prop changed" << std::endl;
  std::string string_arg;
  std::map<std::string, sdbus::Variant> properties;
  std::vector<std::string> array_of_strings;
  signal >> string_arg;
  signal >> properties;
  for (auto &prop : properties) {
    std::cout << prop.first << std::endl;
    switch (property_map[prop.first]) {
    case 0: { // not mapped
      std::cout << "Property \"" << prop.first << "\" not supported."
                << std::endl;
      return;
    }
    case 1: { // shuffle
      std::cout << "Shuffle property changed, new value: "
                << prop.second.get<bool>() << std::endl;
      m_is_shuffle = prop.second.get<bool>();
      notify_observers_is_shuffle_changed();
      return;
    }
    case 2: { // metadata
      std::cout << "Metadata property changed." << std::endl;
      auto meta_v = prop.second.get<std::map<std::string, sdbus::Variant>>();
      std::map<std::string, int> type_map = {{"n", 1},    // int16
                                             {"q", 2},    // uint16
                                             {"i", 3},    // int32
                                             {"u", 4},    // uint32
                                             {"x", 5},    // int64
                                             {"t", 6},    // uint64
                                             {"d", 7},    // double
                                             {"s", 8},    // string
                                             {"o", 9},    // object path
                                             {"b", 10},   // boolean
                                             {"as", 11}}; // array of strings
      std::vector<std::pair<std::string, std::string>> metadata;

      for (auto &data : meta_v) {
        std::string type = data.second.peekValueType();
        switch (type_map[type]) {
        case 0: {
          std::cout << "Warning: not implemented parsing for type \"" << type
                    << "\", skipping " << data.first << std::endl;
        }
        case 1: { // int16
          try {
            int16_t num = data.second.get<int16_t>();
            std::cout << data.first << ": " << num << std::endl;
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch int16: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"n\" expected." << std::endl;
            break;
          }

          break;
        }
        case 2: { // uint16
          try {
            uint16_t num = data.second.get<uint16_t>();
            std::cout << data.first << ": " << num << std::endl;
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch uint16: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"q\" expected." << std::endl;
            break;
          }

          break;
        }
        case 3: { // int32
          try {
            int32_t num = data.second.get<int32_t>();
            std::cout << data.first << ": " << num << std::endl;
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch int32: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"i\" expected." << std::endl;
            break;
          }

          break;
        }
        case 4: { // uint32
          try {
            uint32_t num = data.second.get<uint32_t>();
            std::cout << data.first << ": " << num << std::endl;
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch uint32: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"i\" expected." << std::endl;
            break;
          }

          break;
        }
        case 5: { // int64
          try {
            int64_t num = data.second.get<int64_t>();
            std::cout << data.first << ": " << num << std::endl;
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch int64: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"x\" expected." << std::endl;
            break;
          }

          break;
        }
        case 6: { // uint64
          try {
            uint64_t num = data.second.get<uint64_t>();
            std::cout << data.first << ": " << num << std::endl;
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch uint64: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"d\" expected." << std::endl;
            break;
          }

          break;
        }
        case 7: { // double
          try {
            double num = data.second.get<double>();
            std::cout << data.first << ": " << num << std::endl;
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch double: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"t\" expected." << std::endl;
            break;
          }

          break;
        }
        case 8: { // string
          try {
            std::string str = data.second.get<std::string>();
            std::cout << data.first << ": " << str << std::endl;
            metadata.push_back(std::make_pair(data.first, str));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch string: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"s\" expected." << std::endl;
            break;
          }

          break;
        }
        case 9: { // object path
          try {
            std::string path = data.second.get<sdbus::ObjectPath>();
            std::cout << data.first << ": " << path << std::endl;
            metadata.push_back(std::make_pair(data.first, path));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch object path: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"o\" expected." << std::endl;
            break;
          }
          break;
        }
        case 10: { // boolean
          try {
            bool boolean = data.second.get<bool>();
            std::cout << data.first << ": " << bool_to_string(boolean)
                      << std::endl;
            metadata.push_back(
                std::make_pair(data.first, bool_to_string(boolean)));
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch boolean: " << e.what()
                      << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"b\" expected." << std::endl;
            break;
          }
          break;
        }
        case 11: { // array of strings
          try {
            std::vector<std::string> arr =
                data.second.get<std::vector<std::string>>();
            for (auto &entry : arr) {
              std::cout << data.first << ": " << entry << std::endl;
              metadata.push_back(std::make_pair(data.first, entry));
            }
          } catch (const sdbus::Error &e) {
            std::cout << "Error while trying to fetch array of strings: "
                      << e.what() << std::endl;
            std::cout << "Received type: \"" << type
                      << "\" while \"b\" expected." << std::endl;
            break;
          }
          break;
        }
        default: {
          std::cout << "Got not implemented data type: " << type
                    << ", skipping " << data.first << std::endl;
          break;
        }
        }
      }

      // set new song, new length, new author....
      for (auto &info : metadata) {
        if (info.first == "mpris:length") {
          int64_t length = stoi(info.second);
          length /= 1000000;
          m_song_length_str = Helper::get_instance().format_time(length);
          notify_observers_song_length_changed();
          std::cout << "Song length: " << m_song_length_str << std::endl;
        } else if (info.first == "xesam:artist") {
          m_song_artist = info.second;
          notify_observers_song_artist_changed();
        } else if (info.first == "xesam:title") {
          m_song_title = info.second;
          notify_observers_song_title_changed();
        }
      }
      return;
    }
    case 3: { // Volume property
      std::cout << "Volume property changed, new value: "
                << prop.second.get<double>() << std::endl;
      m_song_volume = prop.second.get<double>();
      notify_observers_song_volume_changed();
      return;
    }
    case 4: { // PlaybackStatus
      std::cout << "PlaybackStatus property changed, new value: "
                << prop.second.get<std::string>() << std::endl;
      m_is_playing = prop.second.get<std::string>() == "Playing";
      notify_observers_is_playing_changed();
      return;
    }
    }
  }
}

void Player::on_seeked(sdbus::Signal &signal) {
  m_song_pos = get_position();
  notify_observers_song_position_changed();
}

#endif

void Player::add_observer(PlayerObserver *observer) {
  m_observers.push_back(observer);
}

void Player::remove_observer(PlayerObserver *observer) {
  m_observers.erase(
      std::remove(m_observers.begin(), m_observers.end(), observer),
      m_observers.end());
}

std::string Player::get_song_name() const { return m_song_title; }

void Player::set_song_name(const std::string &new_song_name) {
  m_song_title = new_song_name;
  notify_observers_song_title_changed();
}

std::string Player::get_song_author() const { return m_song_artist; }

void Player::set_song_author(const std::string &new_song_author) {
  m_song_artist = new_song_author;
  notify_observers_song_artist_changed();
}

std::string Player::get_song_length_str() const { return m_song_length_str; }

void Player::set_song_length(const std::string &new_song_length) {
  m_song_length_str = new_song_length;
  notify_observers_song_length_changed();
}

void Player::get_song_data() {
  auto metadata = get_metadata();
  for (auto &info : metadata) {
    if (info.first == "mpris:length") {
      std::cout << "Got metadata length: " << info.second << std::endl;
      int64_t length = stoi(info.second);
      if (length == -1)
        length = 0;
      std::cout << "Stoi: " << length << std::endl;
      if (get_current_player_name() != "Local")
        length /= 1000000;
      m_song_length_str = Helper::get_instance().format_time(length);
      notify_observers_song_length_changed();
      std::cout << "Song length: " << m_song_length_str << std::endl;
    } else if (info.first == "xesam:artist") {
      m_song_artist = info.second;
      notify_observers_song_artist_changed();
    } else if (info.first == "xesam:title") {
      m_song_title = info.second;
      notify_observers_song_title_changed();
    }
  }
  m_is_shuffle = get_shuffle();
  notify_observers_is_shuffle_changed();
  m_is_playing = get_playback_status();
  notify_observers_is_playing_changed();
  m_song_volume = get_volume();
  notify_observers_song_volume_changed();
  m_song_pos = get_position();
  if (m_song_pos == -1)
    m_song_pos = 0;
  notify_observers_song_position_changed();
}

void Player::notify_observers_song_title_changed() {
  for (auto observer : m_observers) {
    observer->on_song_title_changed(m_song_title);
  }
}

void Player::notify_observers_song_artist_changed() {
  for (auto observer : m_observers) {
    observer->on_song_artist_changed(m_song_artist);
  }
}

void Player::notify_observers_song_length_changed() {
  for (auto observer : m_observers) {
    observer->on_song_length_changed(m_song_length_str);
  }
}

void Player::notify_observers_is_shuffle_changed() {
  for (auto observer : m_observers) {
    observer->on_is_shuffle_changed(m_is_shuffle);
  }
}

void Player::notify_observers_is_playing_changed() {
  for (auto observer : m_observers) {
    observer->on_is_playing_changed(m_is_playing);
  }
}

void Player::notify_observers_song_volume_changed() {
  for (auto observer : m_observers) {
    observer->on_song_volume_changed(m_song_volume);
  }
}

void Player::notify_observers_song_position_changed() {
  for (auto observer : m_observers) {
    observer->on_song_position_changed(m_song_pos);
  }
}

#ifdef SUPPORT_AUDIO_OUTPUT

void Player::open_audio(const std::string &filename) {
  Mix_FreeMusic(m_current_music);
  SF_INFO info = {0};
  SNDFILE *sndfile = sf_open(filename.c_str(), SFM_READ, &info);
  if (!sndfile) {
    std::cout << "Error opening file " << filename.c_str() << ": "
              << sf_strerror(sndfile) << std::endl;
    return;
  }

  std::cout << "Audio file: " << filename.c_str() << std::endl;
  std::cout << "Sample rate: " << info.samplerate << std::endl;
  std::cout << "Channels: " << info.channels << std::endl;

  sf_close(sndfile);
  int audio_rate = info.samplerate;
  Uint16 audio_format = AUDIO_S16SYS;
  int audio_channels = info.channels;
  int audio_buffers = 4096;

  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) <
      0) {
    std::cout << "Failed to open audio: " << Mix_GetError() << std::endl;
    return;
  }
  m_current_music = Mix_LoadMUS(filename.c_str());
  if (!m_current_music) {
    std::cout << "Mix_LoadMUS failed: " << Mix_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }
  m_song_title = Mix_GetMusicTitle(m_current_music);
  m_song_artist = Mix_GetMusicArtistTag(m_current_music);
  m_song_length = Mix_MusicDuration(m_current_music);
  m_song_length_str = Helper::get_instance().format_time(m_song_length);
  std::cout << "Title: " << m_song_title << std::endl;
  notify_observers_song_title_changed();
  std::cout << "Artist: " << m_song_artist << std::endl;
  notify_observers_song_artist_changed();
  std::cout << "Length (seconds): " << m_song_length << std::endl;
  notify_observers_song_length_changed();
  m_song_pos = 0;
  notify_observers_song_position_changed();
}

void Player::play_audio() {
  if (!Mix_PlayingMusic()) {
    // Start playing the audio
    if (Mix_PlayMusic(m_current_music, 0) == -1) {
      std::cout << "Mix_PlayMusic failed: " << Mix_GetError() << std::endl;
      return;
    }

    m_is_playing = true;
    notify_observers_is_playing_changed();
    return;
  } else if (!Mix_PlayingMusic() || Mix_PausedMusic()) {
    Mix_ResumeMusic();
    m_is_playing = true;
    notify_observers_is_playing_changed();
    return;
  }
}

void Player::stop_audio() {
  Mix_HaltMusic();
  m_is_playing = false;
  notify_observers_is_playing_changed();
}

void Player::pause_audio() {
  Mix_PauseMusic();
  m_is_playing = false;
  notify_observers_is_playing_changed();
}

Mix_Music *Player::get_music() const { return m_current_music; }

#endif
