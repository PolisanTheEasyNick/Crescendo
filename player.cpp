#include "player.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <thread>
/**
 * Converts bool to const char*
 *
 *@param b (type: bool)
 *@return "true" if b is true or "false" otherwise
 */
inline const char *const bool_to_string(bool b) { return b ? "true" : "false"; }

#ifdef HAVE_DBUS
void Player::update_position_thread() {
  if (get_is_playing()) {
    double current_pos = get_position(); // get current pos
    Helper::get_instance().log("Current pos: " + std::to_string(current_pos) +
                               ", " + get_position_str());
  }
  send_info_to_clients();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // wait 1 sec
}
#endif

void Player::server_thread() {
  // Create a socket
  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket == -1) {
    Helper::get_instance().log("SOCKET: Failed to create socket");
    return;
  }

  // Set the socket to non-blocking mode
  if (fcntl(serverSocket, F_SETFL, O_NONBLOCK) == -1) {
    Helper::get_instance().log(
        "SOCKET: Failed to set socket to non-blocking mode");
    close(serverSocket);
    return;
  }

  // Bind the socket to a specific IP address and port
  sockaddr_in serverAddress{};
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr =
      INADDR_ANY;                       // Listen on all network interfaces
  serverAddress.sin_port = htons(4308); // Port number

  bool isBinded = false;
  while (!isBinded) {
    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress),
             sizeof(serverAddress)) == -1) {
      Helper::get_instance().log(
          "SOCKET: Failed to bind socket, trying again after 10 seconds...");
      // close(serverSocket);
      std::this_thread::sleep_for(
          std::chrono::seconds(10)); // Wait for 10 seconds
    } else {
      isBinded = true; // Socket is successfully bound, exit the loop
    }
  }

  // Listen for incoming connections
  if (listen(serverSocket, 10) == -1) {
    Helper::get_instance().log("SOCKET: Failed to listen on socket");
    close(serverSocket);
    return;
  }

  Helper::get_instance().log(
      "SOCKET: Server started. Listening for connections...");

  // Accept incoming connections and handle them
  while (serverRunning) {
    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);
    // Accept a client connection
    int clientSocket =
        accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddress),
               &clientAddressLength);
    if (clientSocket == -1) {
      // Check if the error is due to non-blocking socket and no connection is
      // pending
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        // Sleep for a short duration to avoid busy looping
        usleep(1000); // 1 millisecond
        continue;
      } else {
        Helper::get_instance().log(
            "SOCKET: Failed to accept client connection");
        close(serverSocket);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket),
                      clients.end());
        return;
      }
    }
    // Handle the client request

    clients.push_back(clientSocket);
    while (serverRunning) {
      // Set up the timeout for recv()
      struct timeval timeout;
      timeout.tv_sec = 2; // Set the timeout value in seconds
      timeout.tv_usec = 0;

      fd_set readSet;
      FD_ZERO(&readSet);
      FD_SET(clientSocket, &readSet);

      int selectResult =
          select(clientSocket + 1, &readSet, nullptr, nullptr, &timeout);
      if (selectResult == -1) {
        Helper::get_instance().log("SOCKET: Select failed");
        close(clientSocket);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket),
                      clients.end());
        break;
      } else if (selectResult == 0) {
        Helper::get_instance().log(
            "SOCKET: Timeout occurred. Closing the client connection.");
        close(clientSocket);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket),
                      clients.end());
        break;
      }

      char received[2048] = {0};
      std::string receivedStr;
      ssize_t bytesRead =
          recv(clientSocket, &received, sizeof(received) - 1, 0);
      received[bytesRead] = '\0';
      receivedStr = std::string(received);
      memset(&(received[0]), 0, 2048);
      if (bytesRead == -1) {
        Helper::get_instance().log("SOCKET: Failed to read from client socket");
        close(clientSocket);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket),
                      clients.end());
        break;
      } else if (bytesRead == 0) {
        // Client disconnected
        Helper::get_instance().log("SOCKET: Client disconnected");
        close(clientSocket);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket),
                      clients.end());
        break;
      } else {
        // Process the received byte
        // int received = ntohl(buffer);
        // received = ntohl(received);
        int operation_code = Helper::get_instance().getOPCode(receivedStr);
        if (operation_code == 400 || operation_code == 40)
          operation_code =
              4; // At startup in some reason receives "400" instead of "4"
        if (operation_code != 0)
          Helper::get_instance().log("Received: " + receivedStr);
        switch (operation_code) {
        case 0: {
          // std::cout << "Received byte: 0 (Testing connection)" <<
          // std::endl;
          break;
        }
        case 1: {
          Helper::get_instance().log("SOCKET: Received byte: 1 (Previous)");
          send_previous();
          break;
        }
        case 2: {
          Helper::get_instance().log("SOCKET: Received byte: 2 (PlayPause)");
          send_play_pause();
          break;
        }
        case 3: {
          Helper::get_instance().log("SOCKET: Received byte: 3 (Next)");
          send_next();
          break;
        }
        case 4: {
          Helper::get_instance().log("SOCKET: Received byte: 4 (Get)");
          send_info_to_clients();
          break;
        }
        case 5: {
          Helper::get_instance().log(
              "SOCKET: Received byte: 5 (Toggle Shuffle)");
          set_shuffle(!get_shuffle());
          break;
        }
        case 6: {
          Helper::get_instance().log(
              "SOCKET: Received byte: 6 (Toggle Repeat)");
          int current_loop_status = get_repeat(); // get current loop status
          if (current_loop_status + 1 == 3) {     // if it last status
            set_repeat(0);                        // go to 0 status
          } else {
            set_repeat(current_loop_status + 1); // go to next status
          }
          break;
        }
        case 7: {
          Helper::get_instance().log("SOCKET: Received byte: 7 (Set position)");
          std::string digits = receivedStr.substr(3);
          int newPos;
          try {
            newPos = std::stoi(digits);
          } catch (std::invalid_argument) {
            Helper::get_instance().log(
                "Error while setting position! Can't cast \"" + digits +
                "\" to int.");
          }

          Helper::get_instance().log("Fetched position " + digits);
          set_position(newPos);
          break;
        }
        case 8: { // Get players. Need to send status 8, count of players and
                  // player:id pairs.
          Helper::get_instance().log("SOCKET: Received byte: 8 (Get players)");

          auto players = get_players();
          uint64_t selected = get_current_player_index();
          std::string result = "8||" + std::to_string(selected);
          for (const auto &player : players) {
            if (player.first == "Local")
              result += "||" + player.first + "||Local";
            else
              result += "||" + player.first + "||" + player.second;
          }
          for (int client : clients) {
            ssize_t bytesSent = send(client, result.c_str(), result.size(), 0);

            if (bytesSent == -1) {
              Helper::get_instance().log(
                  "Failed to send message to the client " +
                  std::to_string(client));
            } else {
              Helper::get_instance().log("Sent " + std::to_string(bytesSent) +
                                         " bytes to the client " +
                                         std::to_string(client));
            }
          }
          break;
        }
        case 9: { // change player. Desired input format: "9||playerIndex"
          Helper::get_instance().log("SOCKET: Received byte: 9 (Set player)");
          // Find the position of "9||" in the input string
          std::string playerID = receivedStr.substr(3);
          uint64_t index;
          try {
            index = std::stoi(playerID);
          } catch (std::invalid_argument) {
            Helper::get_instance().log(
                "Error while setting player! Can't cast \"" + playerID +
                "\" to int.");
          }
          select_player(index);
          if (m_players[index].first == "Local") {
            notify_observers_player_choosed(true);
          } else
            notify_observers_player_choosed(false);
          break;
        }
        case 10: {
          // get list of output devices: devicename||sinkid
          Helper::get_instance().log(
              "SOCKET: Received byte: 10 (Get output devices)");
          auto devices = get_output_devices();
          uint64_t selected = get_current_device_sink_index();
          std::string result = "9||" + std::to_string(selected);
          for (const auto &device : devices) {
            result +=
                "||" + device.first + "||" + std::to_string(device.second);
          }
          for (int client : clients) {
            ssize_t bytesSent = send(client, result.c_str(), result.size(), 0);

            if (bytesSent == -1) {
              Helper::get_instance().log(
                  "Failed to send message to the client " +
                  std::to_string(client));
            } else {
              Helper::get_instance().log("Sent " + std::to_string(bytesSent) +
                                         " bytes to the client " +
                                         std::to_string(client));
            }
          }
          break;
        }
        case 11: { // change output device. Desired input format:
                   // "11||sinkIndex"
          Helper::get_instance().log(
              "SOCKET: Received byte: 11 (Set output device)");
          std::string deviceID = receivedStr.substr(4);
          uint64_t index;
          try {
            index = std::stoi(deviceID);
          } catch (std::invalid_argument) {
            Helper::get_instance().log(
                "Error while setting output device! Can't cast \"" + deviceID +
                "\" to int.");
          }
          set_output_device(index);
          break;
        }
        case 12: { // change volume. Desired input format: "12||newVolume"
          Helper::get_instance().log("SOCKET: Received byte: 12 (Set volume)");
          std::string volume = receivedStr.substr(4);
          double newVolume;
          try {
            newVolume = std::stod(volume);
          } catch (std::invalid_argument) {
            Helper::get_instance().log(
                "Error while setting volume! Can't cast \"" + volume +
                "\" to double.");
          }
          set_volume(newVolume);
          break;
        }
        default: {
          Helper::get_instance().log("SOCKET: Received unknown byte: " +
                                     std::to_string(operation_code));
          break;
        }
        }
      }
    }
    close(clientSocket);
    clients.erase(std::remove(clients.begin(), clients.end(), clientSocket),
                  clients.end());
  }

  // Close the server socket
  close(serverSocket);
}

void Player::send_info_to_clients() {
  if (!clients.empty()) {
    std::string current_info = "";
    auto metadata = get_metadata();
    for (const auto &data : metadata) {
      if (data.first == "mpris:artUrl") {
        if (data.second != "")
          current_info += "art||" + data.second + "||";
        else
          current_info += "art||-||";
      } else if (data.first == "xesam:artist") {
        if (data.second != "")
          current_info += "artist||" + data.second + "||";
        else
          current_info += "artist||-||";
      } else if (data.first == "xesam:title") {
        if (data.second != "")
          current_info += "title||" + data.second + "||";
        else
          current_info += "title||-||";
      }
    }
    std::string length = get_song_length_str();
    if (length == "")
      length = "0:00";
    std::string position = get_position_str();
    if (position == "")
      position = "0:00";

    current_info += "length||" + length + "||";
    current_info += "pos||" + position + "||";
    current_info += "playing||" + std::to_string(get_is_playing()) + "||";
    current_info += "shuffle||" + std::to_string(get_shuffle()) + "||";
    current_info += "repeat||" + std::to_string(get_repeat()) + "||";
    current_info += "volume||" + std::to_string(get_volume()) + "||";
    for (int client : clients) {
      ssize_t bytesSent =
          send(client, current_info.c_str(), current_info.size(), 0);

      if (bytesSent == -1) {
        Helper::get_instance().log("Failed to send message to the client " +
                                   std::to_string(client));
      } else {
        Helper::get_instance().log("Sent " + std::to_string(bytesSent) +
                                   " bytes to the client " +
                                   std::to_string(client));
      }
    }
  }
}

Player::Player(bool with_gui) {
  // init neccessary variables
  m_with_gui = with_gui;
  m_song_title = "";
  m_song_artist = "";
  m_song_length_str = "";
  m_song_pos = 0;
  m_song_length = 0;
  m_is_shuffle = false;
  m_is_playing = false;
  m_song_volume = 0;
#ifdef HAVE_DBUS
  // create dbus connection
  m_dbus_conn = sdbus::createSessionBusConnection();
  if (m_dbus_conn) // check connection and print info about connection on
                   // success
    Helper::get_instance().log("Connected to D-Bus as \"" +
                               m_dbus_conn->getUniqueName() + "\".");
#endif

#ifdef HAVE_DBUS
  start_server();
#endif

  // get current players
  get_players();
  // if players size is not null
  if (m_players.size() != 0) {
    // then select first accessible player
    if (select_player(0)) {
      if (m_players[m_selected_player_id].first == "Local") {
        Helper::get_instance().log("Selected local player.");
      } else {
        Helper::get_instance().log(
            "Selected player: " + m_players[m_selected_player_id].first +
            " at " + m_players[m_selected_player_id].second);
        std::this_thread::sleep_for(
            std::chrono::milliseconds(500)); // wait 0.5 sec
        get_song_data(); // get song data from dbus if player is not local
      }
    };
  } else {
    //    while(m_players.size() <= 0 || !serverRunning) {
    //    //no players found
    //    std::this_thread::sleep_for(
    //        std::chrono::milliseconds(5000));  // wait 5 sec
    //      get_players();
    //    }
    if (m_players.size() > 0 && select_player(0)) {
      if (m_players[m_selected_player_id].first == "Local") {
        Helper::get_instance().log("Selected local player.");
      } else {
        Helper::get_instance().log(
            "Selected player: " + m_players[m_selected_player_id].first +
            " at " + m_players[m_selected_player_id].second);
        std::this_thread::sleep_for(
            std::chrono::milliseconds(500)); // wait 0.5 sec
        get_song_data(); // get song data from dbus if player is not local
      }
    }
  }

#ifdef SUPPORT_AUDIO_OUTPUT
  // if we can use audio output then initialize SDL
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
  // release proxy
  m_proxy_signal.reset();
#endif
#ifdef SUPPORT_AUDIO_OUTPUT
  // free music from mix
  Mix_FreeMusic(m_current_music);
  // close audio output device
  Mix_CloseAudio();
  // quit from SDL
  SDL_Quit();
#endif
}

std::vector<std::pair<std::string, std::string>> Player::get_players() {
  m_players.clear(); // clear m_players vector
#ifdef SUPPORT_AUDIO_OUTPUT
  // if we can play local audio, then add local player
  if (m_with_gui)
    m_players.push_back(std::make_pair("Local", ""));
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get players. Aborting.");
    // if we cant get list of players via DBus then just return local player if
    // added
    return m_players;
  }
  std::vector<std::string> result_of_call; // vector of all ListNames of Dbus
  try {
    // creating proxy for getting ListNames
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(), "org.freedesktop.DBus",
                                    "/org/freedesktop/DBus");
    proxy->callMethod("ListNames")
        .onInterface("org.freedesktop.DBus")
        .storeResultsTo(result_of_call); // and storing inside vector
  } catch (const sdbus::Error &e) {
    // if error - print it
    Helper::get_instance().log(
        std::string("Error while getting all DBus services: ") + e.what());
    return m_players;
  }

  Helper::get_instance().log("Started checking for players");
  if (result_of_call.empty()) { // if vector empty - stop parsing
    Helper::get_instance().log("Error getting reply.");
    return m_players;
  } else {
    Helper::get_instance().log("Got reply from DBus");
    int num_names = 0;
    for (const auto &name : result_of_call) { // check every name
      if (strstr(name.c_str(), "org.mpris.MediaPlayer2.") ==
          name.c_str()) { // if it implements org.mpris.MediaPlayer2
        Helper::get_instance().log("Found media player: " +
                                   name); // print that we found media player
        std::string identity; // string for saving name of this player
        try {
          auto proxy =
              sdbus::createProxy(*m_dbus_conn.get(), name,
                                 "/org/mpris/MediaPlayer2"); // create new proxy
          sdbus::Variant v_identity;
          proxy->callMethod("Get")
              .onInterface("org.freedesktop.DBus.Properties")
              .withArguments("org.mpris.MediaPlayer2",
                             "Identity") // get identity of new player
              .storeResultsTo(
                  v_identity); // save it into identity variant variable
          identity =
              v_identity.get<std::string>(); // parse std::string from variable
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while getting Identity: ") +
              e.what()); // if error while getting player name
          m_players.push_back(std::make_pair(
              "Player", name)); // then just add interface with "Player" name
          return m_players;
        }
        Helper::get_instance().log("Identity: " +
                                   identity); // print identity of player
        m_players.push_back(std::make_pair(
            identity, name)); // add name and interface to m_players vector
      }
    }
  }
  // if no players found and local not accessible
  if (m_players.empty()) {
    Helper::get_instance().log("No media players found.");
    return {};
  }
#endif
  return m_players;
}

void Player::print_players() {
  for (auto &player : m_players) {
    Helper::get_instance().log(player.first + ": " + player.second);
  }
}

void Player::print_players_names() {
  for (auto &player : m_players) {
    Helper::get_instance().log(player.first);
  }
}

bool Player::select_player(unsigned int new_id) {
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get players. Aborting.");
    return false;
  }
#endif
  get_players(); // get list of currently accessible players
  if (new_id < 0 || new_id > m_players.size()) { // if new_id out of bounds
    Helper::get_instance().log("This player does not exists!");
    return false; // cancel operation
  }

  m_selected_player_id = new_id; // set new player
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") { // if it is local
    // then just say that all methods and properties are supported
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
    m_is_repeat_prop = true;
#ifdef HAVE_DBUS
    // if local player, we must stop listening signals from dbus
    stop_listening_signals();
#endif
    return true;
  } else {
    // if we switched from local player to DBus, we need to pause audio on local
    // player
    pause_audio();
  }
#endif
#ifdef HAVE_DBUS
  std::string
      xml_introspect; // string, which will contain xml of all interfaces,
                      // properties and methods of out player
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy
        ->callMethod("Introspect") // call introspect method
        .onInterface("org.freedesktop.DBus.Introspectable")
        .storeResultsTo(xml_introspect); // and store info in xml_introspect
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to get all functions for player: ") +
        e.what());
  }

  // start parsing and checking
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(xml_introspect.c_str());
  if (!result) {
    Helper::get_instance().log(std::string("Failed to parse XML: ") +
                               result.description());
    return false;
  }
  // Find the root node
  pugi::xml_node root = doc.document_element();
  if (!root) {
    Helper::get_instance().log("Error: no root element");
    return 1;
  }

  pugi::xml_node iface_node =
      doc.select_node(
             (std::string(
                  "/node/interface[@name='org.mpris.MediaPlayer2.Player']")
                  .c_str()))
          .node();
  // Searching for play_pause method...
  Helper::get_instance().log("Searching for PlayPause method... ", false);
  pugi::xml_node methodNode =
      iface_node.find_child_by_attribute("method", "name", "PlayPause");
  if (methodNode) {
    // Method found
    Helper::get_instance().log("found.", true, false);
    m_play_pause_method = true;
  } else {
    // Method not found
    Helper::get_instance().log("not found.", true, false);
    m_play_pause_method = false;
  }

  // Searching for pause method...
  Helper::get_instance().log("Searching for Pause method... ", false);
  methodNode = iface_node.find_child_by_attribute("method", "name", "Pause");
  if (methodNode) {
    // Method found
    Helper::get_instance().log("found.", true, false);
    m_pause_method = true;
  } else {
    // Method not found
    Helper::get_instance().log("not found.", true, false);
    m_pause_method = false;
  }

  // Searching for Play method
  Helper::get_instance().log("Searching for Play method... ", false);
  methodNode = iface_node.find_child_by_attribute("method", "name", "Play");
  if (methodNode) {
    // Method found
    Helper::get_instance().log("found.", true, false);
    m_play_method = true;
  } else {
    // Method not found
    Helper::get_instance().log("not found.", true, false);
    m_play_method = false;
  }

  // Searching for Next method
  Helper::get_instance().log("Searching for Next method... ", false);
  methodNode = iface_node.find_child_by_attribute("method", "name", "Next");
  if (methodNode) {
    // Method found
    Helper::get_instance().log("found.", true, false);
    m_next_method = true;
  } else {
    // Method not found
    Helper::get_instance().log("not found.", true, false);
    m_next_method = false;
  }

  // Searching for Previous method
  Helper::get_instance().log("Searching for Previous method... ", false);
  methodNode = iface_node.find_child_by_attribute("method", "name", "Previous");
  if (methodNode) {
    // Method found
    Helper::get_instance().log("found.", true, false);
    m_previous_method = true;
  } else {
    // Method not found
    Helper::get_instance().log("not found.", true, false);
    m_previous_method = false;
  }

  // Searching for SetPosition method
  Helper::get_instance().log("Searching for SetPosition method... ", false);
  methodNode =
      iface_node.find_child_by_attribute("method", "name", "SetPosition");
  if (methodNode) {
    // Method found
    Helper::get_instance().log("found.", true, false);
    m_setpos_method = true;
  } else {
    // Method not found
    Helper::get_instance().log("not found.", true, false);
    m_setpos_method = false;
  }

  // Searching for Shuffle property
  Helper::get_instance().log("Searching for Shuffle property... ", false);
  pugi::xml_node propertyNode =
      iface_node.find_child_by_attribute("property", "name", "Shuffle");
  if (propertyNode) {
    // Property found
    Helper::get_instance().log("found.", true, false);
    m_is_shuffle_prop = true;
  } else {
    // Property not found
    Helper::get_instance().log("not found.", true, false);
    m_is_shuffle_prop = false;
  }

  // Searching for Position property
  Helper::get_instance().log("Searching for Position property... ", false);
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "Position");
  if (propertyNode) {
    // Property found
    Helper::get_instance().log("found.", true, false);
    m_is_pos_prop = true;
  } else {
    // Property not found
    Helper::get_instance().log("not found.", true, false);
    m_is_pos_prop = false;
  }

  // Searching for Volume property
  Helper::get_instance().log("Searching for Volume property... ", false);
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "Volume");
  if (propertyNode) {
    // Property found
    Helper::get_instance().log("found.", true, false);
    m_is_volume_prop = true;
  } else {
    // Property not found
    Helper::get_instance().log("not found.", true, false);
    m_is_volume_prop = false;
  }

  // Searching for PlaybackStatus property
  Helper::get_instance().log("Searching for PlaybackStatus property... ",
                             false);
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "PlaybackStatus");
  if (propertyNode) {
    // Property found
    Helper::get_instance().log("found.", true, false);
    m_is_playback_status_prop = true;
  } else {
    // Property not found
    Helper::get_instance().log("not found.", true, false);
    m_is_playback_status_prop = false;
  }

  // Searching for Metadata property
  Helper::get_instance().log("Searching for Metadata property... ", false);
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "Metadata");
  if (propertyNode) {
    // Property found
    Helper::get_instance().log("found.", true, false);
    m_is_metadata_prop = true;
  } else {
    // Property not found
    Helper::get_instance().log("not found.", true, false);
    m_is_metadata_prop = false;
  }

  // Searching for LoopStatus property
  Helper::get_instance().log("Searching for LoopStatus property... ", false);
  propertyNode =
      iface_node.find_child_by_attribute("property", "name", "LoopStatus");
  if (propertyNode) {
    // Property found
    Helper::get_instance().log("found.", true, false);
    m_is_repeat_prop = true;
  } else {
    // Property not found
    Helper::get_instance().log("not found.", true, false);
    m_is_repeat_prop = false;
  }
  get_song_data();
  start_listening_signals();
#endif
  return true;
}

bool Player::send_play_pause() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") { // if local player
    if (Mix_PlayingMusic() && Mix_PausedMusic()) { // if music opened and paused
      play_audio();                                // just play
    } else if (Mix_PlayingMusic() &&
               !Mix_PausedMusic()) { // if opened and playing
      pause_audio();                 // just pause
    } else {                         // in any other variant
      Helper::get_instance().log("Starting playing");
      play_audio(); // just play
    }
    return true; // success
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't send PlayPause. Aborting.");
    return false;
  }
  if (!m_play_pause_method) {
    Helper::get_instance().log(
        "This player does not compatible with PlayPause method!");
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy
        ->callMethod("PlayPause") // call PlayPause method
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(std::string(
        std::string("Error while trying call PlayPause method: ") + e.what()));
    return false;
  }
#endif
  return false;
}

bool Player::send_pause() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    pause_audio(); // if local player then just pause audio
    return true;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't send Pause. Aborting.");
    return false;
  }
  if (!m_pause_method) {
    Helper::get_instance().log(
        "This player does not compatible with Pause method!");
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy
        ->callMethod("Pause") // call Pause method to DBus
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log("Error while trying call Pause method: ");
    return false;
  }
#endif
  return false;
}

bool Player::send_play() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    play_audio(); // if local player then just play
    return true;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't send Play. Aborting.");
    return false;
  }
  if (!m_play_method) {
    Helper::get_instance().log(
        "This player does not compatible with Play method!");
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy
        ->callMethod("Play") // call Play method to DBus
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying call Play method: ") + e.what());
    return false;
  }
#endif
  return false;
}

bool Player::send_next() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't send Next. Aborting.");
    return false;
  }
  if (!m_next_method) {
    Helper::get_instance().log(
        "This player does not compatible with Next method!");
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy
        ->callMethod("Next") // call Next method to DBus
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying call Next method: ") + e.what());
    return false;
  }
#endif
  return false;
}

bool Player::send_previous() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't send Previous. Aborting.");
    return false;
  }
  if (!m_previous_method) {
    Helper::get_instance().log(
        "This player does not compatible with Previous method!");
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy
        ->callMethod("Previous") // call Previous method to DBus
        .onInterface("org.mpris.MediaPlayer2.Player")
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying call Previous method: ") + e.what());
    return false;
  }
#endif
  return false;
}

bool Player::get_shuffle() {
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return m_is_shuffle; // if local player then just return variable
  }
#endif
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get Shuffle. Aborting.");
    return false;
  }
  if (!m_is_shuffle_prop) {
    Helper::get_instance().log(
        "This player does not compatible with Shuffle property!");
    return false;
  }
  if (m_selected_player_id == -1) {
    Helper::get_instance().log(
        "get_shuffle(): Player not selected, can't get shuffle");
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant current_shuffle_v;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player",
                       "Shuffle")           // get Shuffle property from DBus
        .storeResultsTo(current_shuffle_v); // save it
    bool current_shuffle =
        current_shuffle_v.get<bool>(); // parse variant into bool
    return current_shuffle;            // return Shuffle property
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to get Shuffle property: ") + e.what());
    return false;
  }
#endif
  return false;
}

bool Player::set_shuffle(bool isShuffle) {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    if (m_is_shuffle != isShuffle) {
      m_is_shuffle = isShuffle; // if local player then just set variable
      notify_observers_is_shuffle_changed(); // and notify that shuffle changed
    }
    return true;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't set Shuffle. Aborting.");
    return false;
  }
  if (!m_is_shuffle_prop) {
    std::cerr << "This player does not compatible with Shuffle property!"
              << std::endl;
    return false;
  }
  bool current_shuffle = get_shuffle(); // for DBus get current shuffle status
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Set")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments(
            "org.mpris.MediaPlayer2.Player", "Shuffle",
            sdbus::Variant(!current_shuffle)) // and set opposite from Shuffle
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to set Shuffle property: ") + e.what());
    return false;
  }
#endif
  return false;
}

int Player::get_repeat() {
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return m_repeat; // if local then just return variable
  }
#endif
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return 0;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get Repeat status. Aborting.");
    return 0;
  }
  if (!m_is_repeat_prop) {
    Helper::get_instance().log(
        "This player does not compatible with Repeat status!");
    return 0;
  }
  if (m_selected_player_id == -1) {
    Helper::get_instance().log(
        "get_shuffle(): Player not selected, can't get Repeat status");
    return 0;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant current_loop_v;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player",
                       "LoopStatus") // if DBus player then get LoopStatus
                                     // property of current player
        .storeResultsTo(current_loop_v);
    std::string current_shuffle = current_loop_v.get<std::string>();
    int res = -1;
    if (current_shuffle == "None") { // translate it into int
      res = 0;
    } else if (current_shuffle == "Playlist") {
      res = 1;
    } else if (current_shuffle == "Track") {
      res = 2;
    }
    return res;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to get LoopStatus property: ") +
        e.what());
    return 0;
  }
#endif
  return 0;
}

bool Player::set_repeat(int new_repeat = -1) {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    if (m_repeat != new_repeat) {
      m_repeat = new_repeat; // if local player then just set variable
      notify_observers_loop_status_changed(); // and notify that variable
                                              // changed
    }
    return true;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't set LoopStatus. Aborting.");
    return false;
  }
  if (!m_is_repeat_prop) {
    std::cerr << "This player does not compatible with LoopStatus property!"
              << std::endl;
    return false;
  }
  try { // if dbus player selected
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2"); // create proxy
    std::string loop_to_set; // parse from int status to DBus's string status
    if (new_repeat == -1 || new_repeat == 0) {
      loop_to_set = "None";
    } else if (new_repeat == 1) {
      loop_to_set = "Playlist";
    } else if (new_repeat == 2) {
      loop_to_set = "Track";
    } else {
      loop_to_set = "None";
    }
    proxy->callMethod("Set")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments(
            "org.mpris.MediaPlayer2.Player", "LoopStatus",
            sdbus::Variant(loop_to_set)) // set new LoopStatus property
        .dontExpectReply();
    if (m_repeat != new_repeat) {
      m_repeat = new_repeat;                  // set variable
      notify_observers_loop_status_changed(); // and notify that variable
                                              // changed
    }
    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to set LoopStatus property: ") +
        e.what());
    return false;
  }
#endif
  return false;
}

int64_t Player::get_position() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return 0;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    // if local player then get current position from SDL
    int64_t pos = Mix_GetMusicPosition(m_current_music);
    if (pos > 0)
      return pos;
    else
      return 0;
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't set Position. Aborting.");
    return 0;
  }
  if (!m_is_pos_prop) {
    Helper::get_instance().log(
        "This player does not compatible with Position property!");
    return 0;
  }

  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant position_v;
    int64_t position;
    proxy
        ->callMethod("Get") // if dbus player then
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player",
                       "Position") // get Position property
        .storeResultsTo(position_v);
    position = position_v.get<int64_t>();
    if (position < 0)
      return 0;
    return position / 1000000; // convert it into seconds and return
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to get Position property: ") +
        e.what());
    return 0;
  }
#endif
  return 0;
}

std::string Player::get_position_str() {
  return Helper::get_instance().format_time(
      get_position()); // return formatted current position
}

bool Player::set_position(int64_t pos) {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    int res =
        Mix_SetMusicPosition(pos); // if local player then set position via SDL
    return res == 0;
  }
#endif
#ifdef HAVE_DBUS
  if (get_current_player_name() != "Local") { // if not local player
    pos *= 1000000;
  }

  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't set Position. Aborting.");
    return false;
  }
  if (!m_setpos_method) {
    Helper::get_instance().log(
        "This player does not compatible with SetPosition method!");
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2"); // create proxy
    auto meta = get_metadata();                                 // get metadata
    sdbus::ObjectPath trackid;
    for (auto &info : meta) {
      if (info.first == "mpris:trackid") { // parse current trackid
        trackid = info.second;
        break;
      }
    }
    auto call =
        proxy->createMethodCall("org.mpris.MediaPlayer2.Player",
                                "SetPosition"); // call method SetPosition
    call << trackid << pos; // for current trackid and position
    call.dontExpectReply();
    call.send(100);

    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to set Position: ") + e.what());
    return false;
  }
#endif
  return false;
}

uint64_t Player::get_song_length() {
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return m_song_length; // if local player then just get song length variable
  }
#endif
  auto metadata = get_metadata(); // get current metadata
  for (auto &info : metadata) {
    if (info.first == "mpris:length") { // find length
      int64_t length = stol(info.second);
      if (m_players[m_selected_player_id].first !=
          "Local") {       // if player is not local
        length /= 1000000; // then convert to seconds
      }
      if (length > 0)
        return length; // and return length
      else
        return 0;
    }
  }

  return 0;
}

double Player::get_volume() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return 0;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") { // if local player
    return Mix_GetMusicVolume(m_current_music) /
           128; // then get current volume and divide by 128 because max volume
                // in SDL is 128 but we work with 0-1 values
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't set Volume. Aborting.");
    return 0;
  }
  if (!m_is_volume_prop) {
    Helper::get_instance().log(
        "This player does not compatible with Volume property!");
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
        .withArguments("org.mpris.MediaPlayer2.Player",
                       "Volume") // if dbus player, then get Volume property
        .storeResultsTo(volume_v);
    volume = volume_v.get<double>(); // parse from variant
    return volume;                   // and return volume
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to get Volume property: ") + e.what());
    return 0;
  }
#endif
  return 0;
}

bool Player::set_volume(double volume) {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    int newVolume = 128 * volume; // if local player then convert from 0-1 to
                                  // SDL's 0-128 volume format
    Mix_VolumeMusic(newVolume);   // set new volume via SDL
    return true;
  }
#endif
  if (!m_is_volume_prop) {
    Helper::get_instance().log(
        "This player does not compatible with Volume property!");
    return false;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't set Volume. Aborting.");
    return false;
  }
  if (!m_is_shuffle_prop) {
    Helper::get_instance().log(
        "This player does not compatible with Volume property!");
    return false;
  }
  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");

    proxy->callMethod("Set")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments(
            "org.mpris.MediaPlayer2.Player", "Volume",
            sdbus::Variant(
                volume)) // if Dbus player, then just set Volume property
        .dontExpectReply();
    return true;
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while trying to set Volume property: ") + e.what());
    return false;
  }
#endif
  return true;
}

bool Player::get_playback_status() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return false;
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    return Mix_PlayingMusic() && // if local player return whether song opened
                                 // and
           !Mix_PausedMusic();   // not paused
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get PlayBack Status. Aborting.");
    return false;
  }
  if (!m_is_playback_status_prop) {
    std::cerr << "This player does not compatible with PlayBack property!"
              << std::endl;
    return false;
  }

  std::string playback_str;
  try { // if dbus player
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant playback_v;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player",
                       "PlaybackStatus") // get current PlaybackStatus property
        .storeResultsTo(playback_v);     // save it
    playback_str = playback_v.get<std::string>(); // parse it
    return "Playing" == playback_str; // and return whether it is "Playing"
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log("Error while trying to set PlayBack property: ");
    return false;
  }
#endif
  return false;
}

uint64_t Player::get_current_player_index() {
  std::string current_player_name = get_current_player_name();
  for (int i = 0; i < m_players.size(); i++) {
    if (m_players[i].first == current_player_name) {
      return i;
    }
  }
  return -1;
}

std::string Player::get_current_player_name() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return "";
  }
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first ==
      "Local") {    // if current player is local
    return "Local"; // then return Local
  }
#endif
#ifdef HAVE_DBUS // if dbus player
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get Identity. Aborting.");
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
        .storeResultsTo(v_identity); // get identity again
    std::string identity = v_identity.get<std::string>();
    return identity; // and return it
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(
        std::string("Error while getting current player name: ") + e.what());
    return "";
  }
#endif
  return "";
}

unsigned short Player::get_current_device_sink_index() {
#ifdef HAVE_PULSEAUDIO
  // Code thatuse PulseAudio
  Helper::get_instance().log("PulseAudio installed");
#elif HAVE_PIPEWIRE
  Helper::get_instance().log("PipeWire installed");
#else
  // Code that doesn't uses PulseAudio
  Helper::get_instance().log(
      "PulseAudio or PipeWire not installed, can't continue.");
  return -1;
#endif
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return -1;
  }
  static uint32_t proc_id = 0;
#ifdef SUPPORT_AUDIO_OUTPUT
  if (m_players[m_selected_player_id].first == "Local") {
    proc_id = getpid(); // if local player then just get current process pid
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get player's process id. Aborting.");
    return -1;
  }
  if (m_players[m_selected_player_id].first != "Local") {
    try {
      auto proxy = sdbus::createProxy(
          *m_dbus_conn.get(), "org.freedesktop.DBus", "/org/freedesktop/DBus");
      proxy
          ->callMethod("GetConnectionUnixProcessID") // get process id
          .onInterface("org.freedesktop.DBus")
          .withArguments(m_players[m_selected_player_id]
                             .second) // for our player interface
          .storeResultsTo(proc_id);   // and save it
    } catch (const sdbus::Error &e) {
      Helper::get_instance().log(
          std::string("Error while getting current device process id: ") +
          e.what());
      return -1;
    }
  }
#endif
  static int player_sink_id = -1; //-1 means not found

#ifdef HAVE_PULSEAUDIO
  // Create a main loop object
  pa_mainloop *mainloop = pa_mainloop_new();

  // Create a new PulseAudio context
  pa_context *context =
      pa_context_new(pa_mainloop_get_api(mainloop), "pulse_get_sink_index");

  // Connect to the PulseAudio server
  if (pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
    Helper::get_instance().log(std::string("pa_context_connect() failed: ") +
                               pa_strerror(pa_context_errno(context)));
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
          Helper::get_instance().log("Sink input #" +
                                     std::to_string(info->index) + ":");
          Helper::get_instance().log(std::string("    Name: ") + info->name);
          Helper::get_instance().log(
              std::string("    Application name: ") +
              pa_proplist_gets(info->proplist, "application.name"));
          Helper::get_instance().log("    Client index: " +
                                     std::to_string(info->client));
          Helper::get_instance().log("    Sink index: " +
                                     std::to_string(info->sink));
          Helper::get_instance().log(
              "    App Binary: " +
              std::string(pa_proplist_gets(info->proplist,
                                           "application.process.binary")));
          Helper::get_instance().log(
              "    Process id: " +
              std::string(
                  pa_proplist_gets(info->proplist, "application.process.id")));
          Helper::get_instance().log("comparing \"" +
                                     std::string(pa_proplist_gets(
                                         info->proplist, "application.name")) +
                                     "\" and \"" + player_name + "\"");
          char player_proc_id_str[10];
          snprintf(player_proc_id_str, sizeof(player_proc_id_str), "%u",
                   proc_id);
          Helper::get_instance().log(std::string("procid: ") +
                                     player_proc_id_str);
          if (strcmp(pa_proplist_gets(
                         info->proplist,
                         "application.process.id"), // if found same process id
                     player_proc_id_str) == 0 ||
              strcmp(pa_proplist_gets(info->proplist, "application.name"),
                     player_name.c_str()) == 0) // or same binary name
          {
            Helper::get_instance().log(
                "Found current player output sink: " + player_name + ". #" +
                std::to_string(info->sink));
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

#if HAVE_PIPEWIRE
  auto main_loop = pipewire::main_loop::create();
  auto context = pipewire::context::create(main_loop);
  auto core = context->core();
  auto reg = core->registry();
  auto reg_listener = reg->listen();
  struct node_my {
    uint32_t node_id;
    uint32_t client_id;
    uint32_t proc_id;
    std::string media_class;
  };

  struct link_my {
    uint32_t link_id;
    uint32_t client_id;
    uint32_t input_node;
    uint32_t output_node;
  };

  std::vector<node_my> nodes;
  std::vector<link_my> links;

  reg_listener.on<pipewire::registry_event::global>(
      [&](const pipewire::global &global) {
        if (global.type == pipewire::node::type) {
          auto node = reg->bind<pipewire::node>(global.id).get();
          auto info = node->info();
          node_my node_to_add;
          node_to_add.node_id = info.id;
          for (const auto &prop : info.props) {
            if (prop.first != "pipewire.sec.label")
              if (prop.first == "client.id")
                node_to_add.client_id = std::stoi(prop.second);
            if (prop.first == "application.process.id")
              node_to_add.proc_id = std::stoi(prop.second);
            if (prop.first == "media.class")
              node_to_add.media_class = prop.second;
          }
          nodes.push_back(node_to_add);
        }
        if (global.type == pipewire::link::type) {
          auto props = global.props;
          link_my link;
          link.link_id = global.id;
          for (const auto &prop : props) {
            if (prop.first == "client.id")
              link.client_id = std::stoi(prop.second);
            if (prop.first == "link.input.node")
              link.input_node = std::stoi(prop.second);
            if (prop.first == "link.output.node")
              link.output_node = std::stoi(prop.second);
          }
          links.push_back(link);
        }
      });
  core->update();
  uint32_t current_node_id = -1;
  for (const auto &node : nodes) {
    if (node.proc_id == proc_id && node.media_class == "Stream/Output/Audio") {
      current_node_id = node.node_id;
      break;
    };
  }
  // searching to which node client linked
  for (const auto &link : links) {
    if (link.output_node == current_node_id) {
      player_sink_id = link.input_node;
      break;
    }
  }

#endif

  if (player_sink_id == -1) {
    Helper::get_instance().log("Not found player sink. Can't continue.");
    return -1;
  }
  return player_sink_id; // return sink id
}

std::vector<std::pair<std::string, std::string>> Player::get_metadata() {
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return {};
  }
  std::vector<std::pair<std::string, std::string>> metadata;
#ifdef SUPPORT_AUDIO_OUTPUT
  if (get_current_player_name() == "Local") { // if local
    metadata.push_back(std::make_pair(
        "xesam:title", Mix_GetMusicTitle(m_current_music))); // set title
    metadata.push_back(std::make_pair(
        "xesam:artist", Mix_GetMusicArtistTag(m_current_music))); // set artist
    metadata.push_back(std::make_pair(
        "mpris:length",
        std::to_string(Mix_MusicDuration(m_current_music)))); // set length
    return metadata;                                          // and return
  }
#endif
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get metadata. Aborting.");
    return {};
  }
  if (!m_is_metadata_prop) {
    Helper::get_instance().log(
        "This player does not compatible with Metadata property!");
    return {};
  }
  if (m_selected_player_id == -1) {
    Helper::get_instance().log("Player not selected, can't get metadata");
    return {};
  }

  try {
    auto proxy = sdbus::createProxy(*m_dbus_conn.get(),
                                    m_players[m_selected_player_id].second,
                                    "/org/mpris/MediaPlayer2");
    sdbus::Variant metadata_v;
    proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments(
            "org.mpris.MediaPlayer2.Player",
            "Metadata") // get metadata from dbus and write info variant
        .storeResultsTo(metadata_v);
    auto meta = metadata_v.get<
        std::map<std::string, sdbus::Variant>>();    // convert into std::map of
                                                     // string and Variant
    std::map<std::string, int> type_map = {{"n", 1}, // int16
                                           {"q", 2}, // uint16
                                           {"i", 3}, // int32
                                           {"u", 4}, // uint32
                                           {"x", 5}, // int64
                                           {"t", 6}, // uint64
                                           {"d", 7}, // double
                                           {"s", 8}, // string
                                           {"o", 9}, // object path
                                           {"b", 10},   // boolean
                                           {"as", 11}}; // array of strings

    for (auto &data : meta) { // start parsing
      std::string type = data.second.peekValueType();
      switch (type_map[type]) {
      case 0: {
        Helper::get_instance().log(
            "Warning: not implemented parsing for type \"" + type +
            "\", skipping " + data.first);
      }
      case 1: { // int16
        try {
          int16_t num = data.second.get<int16_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch int16: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"n\" expected.");
          break;
        }

        break;
      }
      case 2: { // uint16
        try {
          uint16_t num = data.second.get<uint16_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch uint16: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"q\" expected.");
          break;
        }

        break;
      }
      case 3: { // int32
        try {
          int32_t num = data.second.get<int32_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch int32: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"i\" expected.");
          break;
        }

        break;
      }
      case 4: { // uint32
        try {
          uint32_t num = data.second.get<uint32_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch uint32: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"i\" expected.");
          break;
        }

        break;
      }
      case 5: { // int64
        try {
          int64_t num = data.second.get<int64_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch int64: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"x\" expected.");
          break;
        }

        break;
      }
      case 6: { // uint64
        try {
          uint64_t num = data.second.get<uint64_t>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch uint64: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"d\" expected.");
          break;
        }

        break;
      }
      case 7: { // double
        try {
          double num = data.second.get<double>();
          metadata.push_back(std::make_pair(data.first, std::to_string(num)));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch double: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"t\" expected.");
          break;
        }

        break;
      }
      case 8: { // string
        try {
          std::string str = data.second.get<std::string>();
          metadata.push_back(std::make_pair(data.first, str));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch string: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"s\" expected.");
          break;
        }

        break;
      }
      case 9: { // object path
        try {
          std::string path = data.second.get<sdbus::ObjectPath>();
          metadata.push_back(std::make_pair(data.first, path));
        } catch (const sdbus::Error &e) {
          Helper::get_instance().log(
              std::string("Error while trying to fetch object path: ") +
              e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"o\" expected.");
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
          Helper::get_instance().log(
              std::string("Error while trying to fetch boolean: ") + e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"b\" expected.");
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
          Helper::get_instance().log(
              std::string("Error while trying to fetch array of strings: ") +
              e.what());
          Helper::get_instance().log("Received type: \"" + type +
                                     "\" while \"b\" expected.");
          break;
        }
        break;
      }
      default: {
        Helper::get_instance().log("Got not implemented data type: " + type +
                                   ", skipping " + data.first);
        break;
      }
      }
    }
  } catch (const sdbus::Error &e) {
    Helper::get_instance().log(std::string("Error while getting metadata: ") +
                               e.what());
    return {};
  }
#endif
  return metadata;
}

std::vector<std::pair<std::string, unsigned short>>
Player::get_output_devices() {
#ifdef HAVE_PULSEAUDIO
  // Code thatuse PulseAudio
  Helper::get_instance().log("PulseAudio installed");
#elif HAVE_PIPEWIRE
  Helper::get_instance().log("PipeWire installed");
#else
  // Code that doesn't uses PulseAudio
  Helper::get_instance().log("PulseAudio not installed, can't continue.");
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
          Helper::get_instance().log(
              std::string("Failed to get sink info list: ") +
              pa_strerror(pa_context_errno(context)));
          pa_context_disconnect(context);
        } else if (eol == 0) {
          // Sink device
          auto devices = static_cast<
              std::vector<std::pair<std::string, unsigned short>> *>(userdata);
          devices->emplace_back(info->description,
                                info->index); // add device info devices vector
        }
      },
      &m_devices);
  if (!operation) {
    Helper::get_instance().log("Failed to get sink info list.");
    pa_context_disconnect(context);
    pa_mainloop_free(mainloop);
    return {};
  }

  // Wait for the operation to complete
  while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }

  for (auto &device : m_devices) { // print all devices
    Helper::get_instance().log("Sink name: " + device.first);
    Helper::get_instance().log("Sink index: " + std::to_string(device.second));
  }

  pa_context_disconnect(context); // free pulseaudio elements
  pa_context_unref(context);
  pa_mainloop_free(mainloop);
#endif

#ifdef HAVE_PIPEWIRE
  auto main_loop = pipewire::main_loop::create();
  auto context = pipewire::context::create(main_loop);
  auto core = context->core();
  auto reg = core->registry();

  auto reg_listener = reg->listen<pipewire::registry_listener>();

  struct output_device {
    uint32_t node_id;
    std::string node_desc;
    std::string media_class;
  };

  std::vector<output_device> devices;

  reg_listener.on<pipewire::registry_event::global>(
      [&](const pipewire::global &global) {
        if (global.type == pipewire::node::type) {
          auto node = reg->bind<pipewire::node>(global.id).get();
          auto info = node->info();
          output_device device;
          device.node_id = info.id;
          for (const auto &prop : info.props) {
            if (prop.first == "media.class")
              device.media_class = prop.second;
            if (prop.first == "node.description")
              device.node_desc = prop.second;
          }
          devices.push_back(device);
        }
      });

  core->update();

  for (const auto &device : devices) {
    if (device.media_class == "Audio/Sink" &&
        device.node_desc != "Loopback Analog Stereo")
      m_devices.emplace_back(device.node_desc, device.node_id);
  }

#endif

  return m_devices; // return devices
}

void Player::set_output_device(unsigned short output_sink_index) {
#ifdef HAVE_PULSEAUDIO
  // Code thatuse PulseAudio
  Helper::get_instance().log("PulseAudio installed");
#elif HAVE_PIPEWIRE
  Helper::get_instance().log("PipeWire installed");
#else
  // Code that doesn't uses PulseAudio
  Helper::get_instance().log(
      "PulseAudio or PipeWire not installed, can't continue.");
  return;
#endif
  if (m_selected_player_id < 0 || m_selected_player_id > m_players.size()) {
    Helper::get_instance().log("Player not selected, can't continue.");
    return;
  }
#ifdef HAVE_DBUS
  if (!m_dbus_conn) {
    Helper::get_instance().log(
        "Not connected to DBus, can't get player's process id. Aborting.");
    return;
  }
#endif
  static uint32_t proc_id = 0;
#ifdef SUPPORT_AUDIO_OUTPUT
  if (get_current_player_name() == "Local")
    proc_id = getpid(); // if local player then get process id of current
                        // process
#endif
#ifdef HAVE_DBUS
  if (get_current_player_name() != "Local") { // if dbus player
    try {
      auto proxy = sdbus::createProxy(
          *m_dbus_conn.get(), "org.freedesktop.DBus", "/org/freedesktop/DBus");
      proxy->callMethod("GetConnectionUnixProcessID")
          .onInterface("org.freedesktop.DBus")
          .withArguments(m_players[m_selected_player_id].second)
          .storeResultsTo(proc_id); // get proc id from dbus
    } catch (const sdbus::Error &e) {
      Helper::get_instance().log(
          std::string("Error while getting current device process id: ") +
          e.what());
      return;
    }
  }
#endif
  if (proc_id == 0) {
    Helper::get_instance().log(
        "Error while getting player's process id, can't continue.");
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
    Helper::get_instance().log(std::string("pa_context_connect() failed: ") +
                               pa_strerror(pa_context_errno(context)));
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
          Helper::get_instance().log("Sink input #" +
                                     std::to_string(info->index) + ":");
          Helper::get_instance().log("    Name: " + std::string(info->name));
          Helper::get_instance().log("    Application name: " +
                                     std::string(pa_proplist_gets(
                                         info->proplist, "application.name")));
          Helper::get_instance().log("    Client index: " +
                                     std::to_string(info->client));
          Helper::get_instance().log("    Sink index: " +
                                     std::to_string(info->sink));
          Helper::get_instance().log(
              "    App Binary: " +
              std::string(pa_proplist_gets(info->proplist,
                                           "application.process.binary")));
          Helper::get_instance().log(
              "    Process id: " +
              std::string(
                  pa_proplist_gets(info->proplist, "application.process.id")));
          Helper::get_instance().log("comparing \"" +
                                     std::string(pa_proplist_gets(
                                         info->proplist, "application.name")) +
                                     "\" and \"" + player_name + "\"");
          char player_proc_id_str[10];
          snprintf(player_proc_id_str, sizeof(player_proc_id_str), "%u",
                   proc_id); // convert proc_id to char[10] for converting
                             // between char[]
          Helper::get_instance().log(std::string("procid: ") +
                                     player_proc_id_str);
          if (strcmp(pa_proplist_gets(info->proplist, "application.process.id"),
                     player_proc_id_str) == 0 ||
              strcmp(pa_proplist_gets(info->proplist, "application.name"),
                     player_name.c_str()) == 0) {
            Helper::get_instance().log("Found current player: " + player_name +
                                       ". #" + std::to_string(info->index));
            int *sink_id = static_cast<int *>(userdata);
            *sink_id = info->index;
            return;
          }
        }
      },
      &player_sink_id);
  if (!operation) {
    Helper::get_instance().log("Failed to get player sink.");
    pa_context_disconnect(context);
    pa_mainloop_free(mainloop);
    return;
  }

  // Wait for the operation to complete
  while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
    pa_mainloop_iterate(mainloop, true, NULL);
  }

  if (player_sink_id == -1) {
    Helper::get_instance().log("Not found player sink. Can't continue.");
    return;
  }

  Helper::get_instance().log("Trying to change output device for sink " +
                             std::to_string(player_sink_id) + " to " +
                             std::to_string(output_sink_index));
  static const unsigned short output_sink_path_copy = output_sink_index;

  // Get the list of sink devices
  pa_operation *operation2 = pa_context_move_sink_input_by_index(
      context, player_sink_id, output_sink_index, NULL, NULL);
  if (!operation2) {
    Helper::get_instance().log("Failed to set sink output device.");
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

#ifdef HAVE_PIPEWIRE
  auto main_loop = pipewire::main_loop::create();
  auto context = pipewire::context::create(main_loop);
  auto core = context->core();
  auto reg = core->registry();
  auto reg_listener = reg->listen<pipewire::registry_listener>();
  struct node_my {
    uint32_t node_id;
    uint32_t client_id;
    uint32_t proc_id;
    std::string media_class;
  };

  struct link_my {
    pw_link *link;
    uint32_t link_id;
    uint32_t client_id;
    uint32_t input_node;
    uint32_t output_node;
  };

  std::vector<node_my> nodes;
  std::vector<link_my> links;
  pw_metadata *metadata = nullptr;
  reg_listener.on<pipewire::registry_event::global>(
      [&](const pipewire::global &global) {
        if (global.type == pipewire::node::type) {
          auto node = reg->bind<pipewire::node>(global.id).get();
          auto info = node->info();
          node_my node_to_add;
          node_to_add.node_id = info.id;
          for (const auto &prop : info.props) {
            if (prop.first != "pipewire.sec.label")
              if (prop.first == "client.id")
                node_to_add.client_id = std::stoi(prop.second);
            if (prop.first == "application.process.id")
              node_to_add.proc_id = std::stoi(prop.second);
            if (prop.first == "media.class")
              node_to_add.media_class = prop.second;
          }
          nodes.push_back(node_to_add);
        }
        if (global.type == pipewire::link::type) {
          auto props = global.props;
          link_my link;
          link.link_id = global.id;
          for (const auto &prop : props) {
            if (prop.first == "client.id")
              link.client_id = std::stoi(prop.second);
            if (prop.first == "link.input.node")
              link.input_node = std::stoi(prop.second);
            if (prop.first == "link.output.node")
              link.output_node = std::stoi(prop.second);
          }
          link.link = static_cast<struct pw_link *>(
              pw_registry_bind(reg.get()->get(), global.id, PW_TYPE_INTERFACE_Link,
                               PW_VERSION_LINK, 0));
          links.push_back(link);
        }
        if (!metadata && global.type == PW_TYPE_INTERFACE_Metadata) {
          auto info = global.props;
          for (const auto &prop : info) {
            if (prop.first == "metadata.name" && prop.second == "default") {
              metadata = static_cast<pw_metadata *>(pw_registry_bind(
                  reg.get()->get(), global.id, PW_TYPE_INTERFACE_Metadata,
                  PW_VERSION_METADATA, 0));
              break;
            }
          }
        }
      });
  core->update();
  uint32_t current_player_id = -1;
  for (const auto &node : nodes) {
    if (node.proc_id == proc_id && node.media_class == "Stream/Output/Audio") {
      current_player_id = node.node_id;
    };
  }

  // searching to which node client linked
  for (const auto &link : links) {
    if (link.output_node == current_player_id) {
      if (metadata) {
        pw_metadata_set_property(metadata, current_player_id, "target.node",
                                 "Spa:Id",
                                 std::to_string(output_sink_index).c_str());
      } else {
        Helper::get_instance().log(
            "Error! Metadata not found, so can't change output device.");
      }

      core->update();
      break;
    }
  }

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

bool Player::get_is_repeat_prop() const { return m_is_repeat_prop; }

unsigned int Player::get_count_of_players() const { return m_players.size(); }

bool Player::get_is_playing() const { return m_is_playing; }

void Player::set_is_playing(bool new_is_playing) {
  if (m_is_playing != new_is_playing) {
    m_is_playing = new_is_playing;
    notify_observers_is_playing_changed();
  }
}

#ifdef HAVE_DBUS
void Player::stop_listening_signals() { m_proxy_signal.reset(); }

void Player::start_listening_signals() {
  stop_listening_signals(); // stop listening previous signals
  m_proxy_signal = sdbus::createProxy(*m_dbus_conn.get(),
                                      m_players[m_selected_player_id].second,
                                      "/org/mpris/MediaPlayer2");
  m_proxy_signal->registerSignalHandler( // subscribe to Dbus Signals
      "org.freedesktop.DBus.Properties", "PropertiesChanged",
      [this](sdbus::Signal &sig) {
        on_properties_changed(sig);
      }); // call corresponding functions
  m_proxy_signal->registerSignalHandler(
      "org.mpris.MediaPlayer2.Player", "Seeked",
      [this](sdbus::Signal &sig) { on_seeked(sig); });
  m_proxy_signal->finishRegistration();

  // Start the event loop in a new thread
  m_dbus_conn->enterEventLoopAsync();
  Helper::get_instance().log("Event loop started");
}

void Player::on_properties_changed(sdbus::Signal &signal) {
  std::map<std::string, int> property_map = {
      {"Shuffle", 1},        // Shuffle
      {"Metadata", 2},       // Changed song
      {"Volume", 3},         // changed Volume
      {"PlaybackStatus", 4}, // paused or played
      {"LoopStatus", 5}};    // Loop button status

  // Handle the PropertiesChanged signal
  Helper::get_instance().log("Prop changed");
  std::string string_arg;
  std::map<std::string, sdbus::Variant> properties;
  std::vector<std::string> array_of_strings;
  signal >> string_arg;
  signal >> properties;
  for (auto &prop : properties) { // start parsing properties
    Helper::get_instance().log(prop.first);
    switch (property_map[prop.first]) {
    case 0: { // not mapped
      Helper::get_instance().log("Property \"" + prop.first +
                                 "\" not supported.");
      return;
    }
    case 1: { // shuffle
      Helper::get_instance().log("Shuffle property changed, new value: " +
                                 std::to_string(prop.second.get<bool>()));
      bool new_is_shuffle = prop.second.get<bool>();
      if (m_is_shuffle != new_is_shuffle) {
        m_is_shuffle = new_is_shuffle;
        notify_observers_is_shuffle_changed();
      }
      return;
    }
    case 2: { // metadata
      Helper::get_instance().log("Metadata property changed.");
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
          Helper::get_instance().log(
              "Warning: not implemented parsing for type \"" + type +
              "\", skipping " + data.first);
        }
        case 1: { // int16
          try {
            int16_t num = data.second.get<int16_t>();
            Helper::get_instance().log(data.first + ": " + std::to_string(num));
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch int16: ") + e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"n\" expected.");
            break;
          }

          break;
        }
        case 2: { // uint16
          try {
            uint16_t num = data.second.get<uint16_t>();
            Helper::get_instance().log(data.first + ": " + std::to_string(num));
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch uint16: ") + e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"q\" expected.");
            break;
          }

          break;
        }
        case 3: { // int32
          try {
            int32_t num = data.second.get<int32_t>();
            Helper::get_instance().log(data.first + ": " + std::to_string(num));
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch int32: ") + e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"i\" expected.");
            break;
          }

          break;
        }
        case 4: { // uint32
          try {
            uint32_t num = data.second.get<uint32_t>();
            Helper::get_instance().log(data.first + ": " + std::to_string(num));
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch uint32: ") + e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"i\" expected.");
            break;
          }

          break;
        }
        case 5: { // int64
          try {
            int64_t num = data.second.get<int64_t>();
            Helper::get_instance().log(data.first + ": " + std::to_string(num));
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch int64: ") + e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"x\" expected.");
            break;
          }

          break;
        }
        case 6: { // uint64
          try {
            uint64_t num = data.second.get<uint64_t>();
            Helper::get_instance().log(data.first + ": " + std::to_string(num));
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch uint64: ") + e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"d\" expected.");
            break;
          }

          break;
        }
        case 7: { // double
          try {
            double num = data.second.get<double>();
            Helper::get_instance().log(data.first + ": " + std::to_string(num));
            metadata.push_back(std::make_pair(data.first, std::to_string(num)));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch double: ") + e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"t\" expected.");
            break;
          }

          break;
        }
        case 8: { // string
          try {
            std::string str = data.second.get<std::string>();
            Helper::get_instance().log(data.first + ": " + str);
            metadata.push_back(std::make_pair(data.first, str));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch string: ") + e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"s\" expected.");
            break;
          }

          break;
        }
        case 9: { // object path
          try {
            std::string path = data.second.get<sdbus::ObjectPath>();
            Helper::get_instance().log(data.first + ": " + path);
            metadata.push_back(std::make_pair(data.first, path));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch object path: ") +
                e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"o\" expected.");
            break;
          }
          break;
        }
        case 10: { // boolean
          try {
            bool boolean = data.second.get<bool>();
            Helper::get_instance().log(data.first + ": " +
                                       bool_to_string(boolean));
            metadata.push_back(
                std::make_pair(data.first, bool_to_string(boolean)));
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch boolean: ") +
                e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"b\" expected.");
            break;
          }
          break;
        }
        case 11: { // array of strings
          try {
            std::vector<std::string> arr =
                data.second.get<std::vector<std::string>>();
            for (auto &entry : arr) {
              Helper::get_instance().log(data.first + ": " + entry);
              metadata.push_back(std::make_pair(data.first, entry));
            }
          } catch (const sdbus::Error &e) {
            Helper::get_instance().log(
                std::string("Error while trying to fetch array of strings: ") +
                e.what());
            Helper::get_instance().log("Received type: \"" + type +
                                       "\" while \"b\" expected.");
            break;
          }
          break;
        }
        default: {
          Helper::get_instance().log("Got not implemented data type: " + type +
                                     ", skipping " + data.first);
          break;
        }
        }
      }

      for (auto &info : metadata) {
        if (info.first == "mpris:length") { // get new length
          int64_t length = stol(info.second);
          length /= 1000000;
          if (m_song_length != length) {
            m_song_length = length;
            m_song_length_str = Helper::get_instance().format_time(length);
            notify_observers_song_length_changed(); // notify that song
                                                    // length changed
          }
          Helper::get_instance().log("Song length: " + m_song_length_str);
        } else if (info.first == "xesam:artist") { // get new artist
          std::string new_artist = info.second;
          if (m_song_artist != new_artist) {
            m_song_artist = info.second;
            notify_observers_song_artist_changed(); // notify that artist
                                                    // changed
          }
        } else if (info.first == "xesam:title") { // get new title
          std::string new_title = info.second;
          if (m_song_title != new_title) {
            m_song_title = info.second;
            notify_observers_song_title_changed(); // notify that title
                                                   // changed
          }
        }
      }
      return;
    }
    case 3: { // Volume property
      Helper::get_instance().log("Volume property changed, new value: " +
                                 std::to_string(prop.second.get<double>()));
      double new_volume = prop.second.get<double>();
      if (m_song_volume != new_volume) {
        m_song_volume = new_volume;             // write new volume
        notify_observers_song_volume_changed(); // notify that volume changed
      }
      return;
    }
    case 4: { // PlaybackStatus
      Helper::get_instance().log(
          "PlaybackStatus property changed, new value: " +
          prop.second.get<std::string>());
      bool new_is_playing = prop.second.get<std::string>() ==
                            "Playing"; // write new playback status
      if (m_is_playing != new_is_playing) {
        m_is_playing = new_is_playing;
        notify_observers_is_playing_changed(); // notify that playback status
                                               // changed
      }
      return;
    }
    case 5: {                                            // LoopStatus
      std::string loop = prop.second.get<std::string>(); // get new loop status
      Helper::get_instance().log("LoopStatus property changed, new value: " +
                                 loop);
      int new_repeat = -1;
      if (loop == "None") // parse it into string
        new_repeat = 0;
      else if (loop == "Playlist")
        new_repeat = 1;
      else if (loop == "Track")
        new_repeat = 2;
      else
        new_repeat = -1;
      if (m_repeat != new_repeat) {
        m_repeat = new_repeat;
        notify_observers_loop_status_changed(); // notify that loop status
                                                // changed
      }
    }
    }
  }
}
void Player::on_seeked(sdbus::Signal &signal) {
  double new_pos = get_position(); // get new position
  if (m_song_pos != new_pos) {
    m_song_pos = get_position();              // set new position
    notify_observers_song_position_changed(); // notify that position changed
  }
}

#endif

void Player::add_observer(PlayerObserver *observer) {
  m_observers.push_back(observer); // add observer to m_observers vector
}

void Player::remove_observer(PlayerObserver *observer) {
  m_observers.erase(
      std::remove(m_observers.begin(), m_observers.end(), observer),
      m_observers.end()); // find and remove observer from m_observers vector
}

std::string Player::get_song_name() const { return m_song_title; }

void Player::set_song_name(const std::string &new_song_name) {
  m_song_title = new_song_name;          // set new song name
  notify_observers_song_title_changed(); // notify that song name changed
}

std::string Player::get_song_author() const { return m_song_artist; }

void Player::set_song_author(const std::string &new_song_author) {
  m_song_artist = new_song_author;        // set new artist
  notify_observers_song_artist_changed(); // notify that artist changed
}

std::string Player::get_song_length_str() const { return m_song_length_str; }

void Player::set_song_length(const std::string &new_song_length) {
  m_song_length_str = new_song_length;    // set new song length
  notify_observers_song_length_changed(); // notify that length changed
}

void Player::get_song_data() {
  auto metadata = get_metadata();
  for (auto &info : metadata) {         // start parsing metadata
    if (info.first == "mpris:length") { // find length
      Helper::get_instance().log("Got metadata length: " + info.second);
      int64_t length = stol(info.second);
      if (length == -1)
        length = 0;
      Helper::get_instance().log("Stol: " + std::to_string(length));
      if (get_current_player_name() != "Local")
        length /= 1000000;
      if (m_song_length != length) {
        m_song_length = length;
        m_song_length_str = Helper::get_instance().format_time(length);
        notify_observers_song_length_changed(); // notify that length changed
      }
      Helper::get_instance().log("Song length: " + m_song_length_str);
    } else if (info.first == "xesam:artist") { // find artist
      if (m_song_artist != info.second) {
        m_song_artist = info.second;
        notify_observers_song_artist_changed(); // notify that artist changed
      }
    } else if (info.first == "xesam:title") {
      if (m_song_title != info.second) {
        m_song_title = info.second;            // find title
        notify_observers_song_title_changed(); // notify that title changed
      }
    }
  }
  auto new_shuffle = get_shuffle();
  if (m_is_shuffle != new_shuffle) {
    m_is_shuffle = new_shuffle;            // get new shuffle
    notify_observers_is_shuffle_changed(); // notify that shuffle changed
  }
  auto new_playing = get_playback_status();
  if (m_is_playing != new_playing) {
    m_is_playing = new_playing;
    notify_observers_is_playing_changed();
  }
  auto new_volume = get_volume();
  if (m_song_volume != new_volume) {
    m_song_volume = new_volume;
    notify_observers_song_volume_changed();
  }
  auto new_pos = get_position();
  if (m_song_pos != new_pos) {
    m_song_pos = new_pos;
    if (m_song_pos == -1)
      m_song_pos = 0;
    notify_observers_song_position_changed();
  }
  auto new_repeat = get_repeat();
  if (m_repeat != new_repeat) {
    m_repeat = new_repeat;
    notify_observers_loop_status_changed();
  }
}

void Player::notify_observers_song_title_changed() {
  for (auto observer : m_observers) {
    observer->on_song_title_changed(m_song_title);
  }
  send_info_to_clients();
}

void Player::notify_observers_song_artist_changed() {
  for (auto observer : m_observers) {
    observer->on_song_artist_changed(m_song_artist);
  }
  send_info_to_clients();
}

void Player::notify_observers_song_length_changed() {
  for (auto observer : m_observers) {
    observer->on_song_length_changed(m_song_length_str);
  }
  send_info_to_clients();
}

void Player::notify_observers_is_shuffle_changed() {
  for (auto observer : m_observers) {
    observer->on_is_shuffle_changed(m_is_shuffle);
  }
  send_info_to_clients();
}

void Player::notify_observers_is_playing_changed() {
  for (auto observer : m_observers) {
    observer->on_is_playing_changed(m_is_playing);
  }
  send_info_to_clients();
}

void Player::notify_observers_song_volume_changed() {
  for (auto observer : m_observers) {
    observer->on_song_volume_changed(m_song_volume);
  }
  send_info_to_clients();
}

void Player::notify_observers_song_position_changed() {
  for (auto observer : m_observers) {
    observer->on_song_position_changed(m_song_pos);
  }
  send_info_to_clients();
}

void Player::notify_observers_loop_status_changed() {
  for (auto observer : m_observers) {
    observer->on_loop_status_changed(m_repeat);
  }
  send_info_to_clients();
}

void Player::notify_observers_player_choosed(const bool toLocal) {
  for (auto observer : m_observers) {
    observer->on_player_toggled(toLocal);
  }
}

#ifdef SUPPORT_AUDIO_OUTPUT

bool Player::open_audio(const std::string &filename) {
  Mix_FreeMusic(m_current_music); // free previous opened music
  SF_INFO info = {0};
  SNDFILE *sndfile = sf_open(filename.c_str(), SFM_READ, &info);
  if (!sndfile) {
    Helper::get_instance().log("Error opening file " + filename + ": " +
                               sf_strerror(sndfile));
    return false;
  }

  Helper::get_instance().log("Audio file: " + filename); // take info from file
  Helper::get_instance().log("Sample rate: " + std::to_string(info.samplerate));
  Helper::get_instance().log("Channels: " + std::to_string(info.channels));

  sf_close(sndfile);
  int audio_rate = info.samplerate;
  Uint16 audio_format = AUDIO_S16SYS;
  int audio_channels = info.channels;
  int audio_buffers = 4096;

  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) <
      0) { // if file cannot be opened
    Helper::get_instance().log("Failed to open audio: " +
                               std::string(Mix_GetError()));
    return false;
  }
  m_current_music = Mix_LoadMUS(filename.c_str()); // open file
  if (!m_current_music) {                          // if not opened
    Helper::get_instance().log("Mix_LoadMUS failed: " +
                               std::string(Mix_GetError()));
    return false;
  }
  Mix_PlayMusic(m_current_music, 0);
  Mix_PauseMusic();
  m_song_title = Mix_GetMusicTitle(m_current_music);      // get title
  m_song_artist = Mix_GetMusicArtistTag(m_current_music); // get artist
  if (m_song_title == "" && m_song_artist == "") {
    m_song_title = filename; // if no title and artist in audio file metadata,
                             // then just make title named as filename
  }
  m_song_length = Mix_MusicDuration(m_current_music); // get duration
  m_song_length_str = Helper::get_instance().format_time(m_song_length);
  Helper::get_instance().log("Title: " + m_song_title);
  notify_observers_song_title_changed(); // notify that title changed
  Helper::get_instance().log("Artist: " + m_song_artist);
  notify_observers_song_artist_changed(); // notify that artist changed
  Helper::get_instance().log("Length (seconds): " +
                             std::to_string(m_song_length));
  notify_observers_song_length_changed();
  m_song_pos = 0;                           // position from start
  notify_observers_song_position_changed(); // notify that pos changed
  return true;
}

void Player::play_audio() {
  if (!Mix_PlayingMusic()) { // if not playing
    // Start playing the audio
    if (Mix_PlayMusic(m_current_music, 0) == -1) {
      Helper::get_instance().log("Mix_PlayMusic failed: " +
                                 std::string(Mix_GetError()));
      return;
    }
    if (!m_is_playing) {
      m_is_playing = true;
      notify_observers_is_playing_changed(); // notify that music playing now
    }
    return;
  } else if (!Mix_PlayingMusic() || Mix_PausedMusic()) { // if paused
    Mix_ResumeMusic();                                   // then just resume
    if (!m_is_playing) {
      m_is_playing = true;
      notify_observers_is_playing_changed(); // notify that music playing now
    }
    return;
  }
}

void Player::stop_audio() {
  Mix_HaltMusic(); // stop playing
  if (m_is_playing) {
    m_is_playing = false;
    notify_observers_is_playing_changed();
  }
}

void Player::pause_audio() {
  Mix_PauseMusic(); // pause playing
  if (m_is_playing) {
    m_is_playing = false;
    notify_observers_is_playing_changed();
  }
}

Mix_Music *Player::get_music() const { return m_current_music; }

void Player::start_server() {
  // start server
  m_server_thread = std::thread(&Player::server_thread, this);
  m_server_thread.detach();
}

void Player::stop_server() {
  serverRunning = false;
  if (m_server_thread.joinable())
    m_server_thread.join();
}

#endif
