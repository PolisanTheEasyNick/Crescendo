#include "player.h"

Player::Player() {
  std::cout << "Start constructor Player" << std::endl;
  // Initialize D-Bus error
  dbus_error_init(&dbus_error);
  // Connect to D-Bus
  dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
  get_players();
  if (players.size() != 0) {
    if (select_player(0)) {
      std::cout << "Selected player: " << players[selected_player_id].first
                << " at " << players[selected_player_id].second << std::endl;
    };
  }
}

Player::~Player() {
  dbus_connection_unref(dbus_conn);
  dbus_error_free(&dbus_error);
}

std::vector<std::pair<std::string, std::string>> Player::get_players() {
  players.clear();
  if (dbus_error_is_set(&dbus_error)) {
    printf("DBus error: %s\n", dbus_error.message);
    return {};
  }
  std::cout << "Connected to D-Bus as \"" << dbus_bus_get_unique_name(dbus_conn)
            << "\"." << std::endl;
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call("org.freedesktop.DBus",
                                          "/org/freedesktop/DBus",
                                          "org.freedesktop.DBus", "ListNames");
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return {};
  }
  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);

  if (!dbus_reply) {
    fprintf(stderr, "Error getting reply: %s\n", dbus_error.message);
    return {};
  }

  DBusMessageIter iter;
  dbus_message_iter_init(dbus_reply, &iter);
  int num_names = 0;

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY ||
      dbus_message_iter_get_element_type(&iter) != DBUS_TYPE_STRING) {
    std::cout << "Error parsing reply: not received array of strings."
              << std::endl;
    dbus_message_unref(dbus_msg);
    dbus_message_unref(dbus_reply);
    std::string error_msg = dbus_error.message;
  } else {
    std::cout << "Parsing reply" << std::endl;
    char *name;
    DBusMessageIter sub_iter;
    dbus_message_iter_recurse(&iter, &sub_iter);
    // Iterate over the array of names
    while (dbus_message_iter_get_arg_type(&sub_iter) == DBUS_TYPE_STRING) {
      dbus_message_iter_get_basic(&sub_iter, &name);
      dbus_message_iter_next(&sub_iter);
      // Check if this service implements the MediaPlayer2 interface
      // std::cout << "Checking " << name << std::endl;
      if (strstr(name, "org.mpris.MediaPlayer2.") == name) {
        std::cout << "Found media player: " << name << std::endl;
        // trying to get identity
        const char *identity;
        // Compose remote procedure call
        dbus_msg = dbus_message_new_method_call(
            name,                              // Destination bus name
            "/org/mpris/MediaPlayer2",         // Object path
            "org.freedesktop.DBus.Properties", // Interface name
            "Get");                            // Method name

        if (!dbus_msg) {
          std::cout << "Error creating message." << std::endl;
        }
        const char *interfaceName = "org.mpris.MediaPlayer2";
        const char *propertyName = "Identity";
        dbus_message_append_args(dbus_msg, DBUS_TYPE_STRING, &interfaceName,
                                 DBUS_TYPE_STRING, &propertyName,
                                 DBUS_TYPE_INVALID);

        // Invoke remote procedure call, block for response
        dbus_reply = dbus_connection_send_with_reply_and_block(
            dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
        if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
          std::cout << "Error getting reply: " << dbus_error.message
                    << std::endl;
          dbus_message_unref(dbus_msg);
        }
        // Extract the property value from the reply
        DBusMessageIter iter;
        dbus_message_iter_init(dbus_reply, &iter);
        const char *signature = dbus_message_iter_get_signature(&iter);
        if (strcmp(signature, "v") != 0) {
          std::cout << "DBus message error: Invalid signature" << std::endl;
        }
        DBusMessageIter valueIter;
        dbus_message_iter_recurse(&iter, &valueIter);
        dbus_message_iter_get_basic(&valueIter, &identity);

        // Print the "Identity" property value
        std::cout << "Identity: " << identity << std::endl;
        players.push_back(std::make_pair(std::string(identity), name));
      }
    }
  }

  if (players.empty()) {
    std::cout << "No media players found." << std::endl;
    return {};
  }
  dbus_message_unref(dbus_msg);
  dbus_message_unref(dbus_reply);
  return players;
}

void Player::print_players() {
  for (auto &player : players) {
    std::cout << player.first << ": " << player.second << std::endl;
  }
}

void Player::print_players_names() {
  for (auto &player : players) {
    DBusMessage *dbus_msg, *dbus_reply;
    // Compose remote procedure call
    dbus_msg = dbus_message_new_method_call(
        player.second.c_str(),             // Destination bus name
        "/org/mpris/MediaPlayer2",         // Object path
        "org.freedesktop.DBus.Properties", // Interface name
        "Get");                            // Method name

    if (!dbus_msg) {
      std::cout << "Error creating message." << std::endl;
      return;
    }
    const char *interfaceName = "org.mpris.MediaPlayer2";
    const char *propertyName = "Identity";
    dbus_message_append_args(dbus_msg, DBUS_TYPE_STRING, &interfaceName,
                             DBUS_TYPE_STRING, &propertyName,
                             DBUS_TYPE_INVALID);

    // Invoke remote procedure call, block for response
    dbus_reply = dbus_connection_send_with_reply_and_block(
        dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
    if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
      std::cout << "Error getting reply: " << dbus_error.message << std::endl;
      dbus_message_unref(dbus_msg);
      return;
    }
    // Extract the property value from the reply
    DBusMessageIter iter;
    dbus_message_iter_init(dbus_reply, &iter);
    const char *signature = dbus_message_iter_get_signature(&iter);
    if (strcmp(signature, "v") != 0) {
      std::cout << "DBus message error: Invalid signature" << std::endl;
      return;
    }
    DBusMessageIter valueIter;
    dbus_message_iter_recurse(&iter, &valueIter);
    const char *identity;
    dbus_message_iter_get_basic(&valueIter, &identity);

    // Print the "Identity" property value
    std::cout << "Identity: " << identity << std::endl;
  }
}

bool Player::select_player(unsigned int new_id) {
  get_players();
  if (new_id < 0 || new_id > players.size()) {
    std::cout << "This player does not exists!" << std::endl;
    return false;
  }
  selected_player_id = new_id;
  // send Interspect
  if (dbus_error_is_set(&dbus_error)) {
    printf("DBus error: %s\n", dbus_error.message);
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), "/org/mpris/MediaPlayer2",
      "org.freedesktop.DBus.Introspectable", "Introspect");
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);

  if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    return false;
  }

  std::cout << "Parsing reply" << std::endl;
  char *xml_introspect;
  dbus_message_get_args(dbus_reply, &dbus_error, DBUS_TYPE_STRING,
                        &xml_introspect, DBUS_TYPE_INVALID);
  if (!dbus_error_is_set(&dbus_error)) {
    dbus_message_unref(dbus_msg);
    dbus_message_unref(dbus_reply);
    // start parsing and checking
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml_introspect);
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
      play_pause_method = true;
    } else {
      // Method not found
      std::cout << "not found." << std::endl;
      play_pause_method = false;
    }

    // Searching for pause method...
    std::cout << "Searching for Pause method... ";
    methodNode = iface_node.find_child_by_attribute("method", "name", "Pause");
    if (methodNode) {
      // Method found
      std::cout << "found." << std::endl;
      pause_method = true;
    } else {
      // Method not found
      std::cout << "not found." << std::endl;
      pause_method = false;
    }

    // Searching for Play method
    std::cout << "Searching for Play method... ";
    methodNode = iface_node.find_child_by_attribute("method", "name", "Play");
    if (methodNode) {
      // Method found
      std::cout << "found." << std::endl;
      play_method = true;
    } else {
      // Method not found
      std::cout << "not found." << std::endl;
      play_method = false;
    }

    // Searching for Next method
    std::cout << "Searching for Next method... ";
    methodNode = iface_node.find_child_by_attribute("method", "name", "Next");
    if (methodNode) {
      // Method found
      std::cout << "found." << std::endl;
      next_method = true;
    } else {
      // Method not found
      std::cout << "not found." << std::endl;
      next_method = false;
    }

    // Searching for Previous method
    std::cout << "Searching for Previous method... ";
    methodNode =
        iface_node.find_child_by_attribute("method", "name", "Previous");
    if (methodNode) {
      // Method found
      std::cout << "found." << std::endl;
      previous_method = true;
    } else {
      // Method not found
      std::cout << "not found." << std::endl;
      previous_method = false;
    }

    // Searching for SetPosition method
    std::cout << "Searching for SetPosition method... ";
    methodNode =
        iface_node.find_child_by_attribute("method", "name", "SetPosition");
    if (methodNode) {
      // Method found
      std::cout << "found." << std::endl;
      setpos_method = true;
    } else {
      // Method not found
      std::cout << "not found." << std::endl;
      setpos_method = false;
    }

    // Searching for Shuffle property
    std::cout << "Searching for Shuffle property... ";
    pugi::xml_node propertyNode =
        iface_node.find_child_by_attribute("property", "name", "Shuffle");
    if (propertyNode) {
      // Property found
      std::cout << "found." << std::endl;
      is_shuffle_prop = true;
    } else {
      // Property not found
      std::cout << "not found." << std::endl;
      is_shuffle_prop = false;
    }

    // Searching for Position property
    std::cout << "Searching for Position property... ";
    propertyNode =
        iface_node.find_child_by_attribute("property", "name", "Position");
    if (propertyNode) {
      // Property found
      std::cout << "found." << std::endl;
      is_pos_prop = true;
    } else {
      // Property not found
      std::cout << "not found." << std::endl;
      is_pos_prop = false;
    }

    // Searching for Volume property
    std::cout << "Searching for Volume property... ";
    propertyNode =
        iface_node.find_child_by_attribute("property", "name", "Volume");
    if (propertyNode) {
      // Property found
      std::cout << "found." << std::endl;
      is_volume_prop = true;
    } else {
      // Property not found
      std::cout << "not found." << std::endl;
      is_volume_prop = false;
    }

    // Searching for PlaybackStatus property
    std::cout << "Searching for PlaybackStatus property... ";
    propertyNode = iface_node.find_child_by_attribute("property", "name",
                                                      "PlaybackStatus");
    if (propertyNode) {
      // Property found
      std::cout << "found." << std::endl;
      is_playback_status_prop = true;
    } else {
      // Property not found
      std::cout << "not found." << std::endl;
      is_playback_status_prop = false;
    }

    // Searching for Metadata property
    std::cout << "Searching for Metadata property... ";
    propertyNode =
        iface_node.find_child_by_attribute("property", "name", "Metadata");
    if (propertyNode) {
      // Property found
      std::cout << "found." << std::endl;
      is_metadata_prop = true;
    } else {
      // Property not found
      std::cout << "not found." << std::endl;
      is_metadata_prop = false;
    }

    return true;
  } else {
    std::cout << "Error while trying to parse reply: " << dbus_error.message
              << std::endl;
    dbus_message_unref(dbus_msg);
    dbus_message_unref(dbus_reply);
    return false;
  }
}

bool Player::send_play_pause() {
  if (!play_pause_method) {
    std::cerr << "This player does not compatible with PlayPause method!"
              << std::endl;
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "PlayPause");
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  dbus_uint32_t serial = 0;
  if (!dbus_connection_send(dbus_conn, dbus_msg, &serial)) {
    return false;
  }
  return true;
}

bool Player::send_pause() {
  if (!pause_method) {
    std::cerr << "This player does not compatible with Pause method!"
              << std::endl;
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "Pause");
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  dbus_uint32_t serial = 0;
  if (!dbus_connection_send(dbus_conn, dbus_msg, &serial)) {
    return false;
  }
  return true;
}

bool Player::send_play() {
  if (!play_method) {
    std::cerr << "This player does not compatible with Play method!"
              << std::endl;
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "Play");
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  dbus_uint32_t serial = 0;
  if (!dbus_connection_send(dbus_conn, dbus_msg, &serial)) {
    return false;
  }
  return true;
}

bool Player::send_next() {
  if (!next_method) {
    std::cerr << "This player does not compatible with Next method!"
              << std::endl;
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "Next");
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  dbus_uint32_t serial = 0;
  if (!dbus_connection_send(dbus_conn, dbus_msg, &serial)) {
    return false;
  }
  return true;
}

bool Player::send_previous() {
  if (!previous_method) {
    std::cerr << "This player does not compatible with Previous method!"
              << std::endl;
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "Previous");
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  dbus_uint32_t serial = 0;
  if (!dbus_connection_send(dbus_conn, dbus_msg, &serial)) {
    return false;
  }
  return true;
}

bool Player::get_shuffle() {
  if (!is_shuffle_prop) {
    std::cerr << "This player does not compatible with Shuffle property!"
              << std::endl;
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  if (selected_player_id == -1) {
    std::cout << "get_shuffle(): Player not selected, can't get shuffle"
              << std::endl;
    return false;
  }
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",                  // Object path
      "org.freedesktop.DBus.Properties",          // Interface name
      "Get");                                     // Method name

  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  const char *interfaceName = "org.mpris.MediaPlayer2.Player";
  const char *propertyName = "Shuffle";
  dbus_message_append_args(dbus_msg, DBUS_TYPE_STRING, &interfaceName,
                           DBUS_TYPE_STRING, &propertyName, DBUS_TYPE_INVALID);
  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
  if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    return false;
  }

  DBusMessageIter iter;
  if (dbus_message_iter_init(dbus_reply, &iter) &&
      dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
    bool isShuffle;

    // Extract variant value
    dbus_message_iter_recurse(&iter, &iter);
    dbus_message_iter_get_basic(&iter, &isShuffle);
    return isShuffle;
  }
  return false;
}

bool Player::set_shuffle(bool isShuffle) {
  if (!is_shuffle_prop) {
    std::cerr << "This player does not compatible with Shuffle property!"
              << std::endl;
    return false;
  }
  if (dbus_error_is_set(&dbus_error)) {
    printf("DBus error: %s\n", dbus_error.message);
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",                  // Object path
      "org.freedesktop.DBus.Properties",          // Interface name
      "Set");                                     // Method name
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  const char *interfaceName = "org.mpris.MediaPlayer2.Player";
  const char *propertyName = "Shuffle";
  DBusMessageIter iter, sub_iter;
  dbus_message_iter_init_append(dbus_msg, &iter);
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING,
                                      &interfaceName) ||
      !dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &propertyName)) {
    std::cout << "Error appending arguments to message: " << dbus_error.message
              << std::endl;
    dbus_message_unref(dbus_msg);
    return false;
  }
  dbus_bool_t set_shuffle = isShuffle;
  dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "b", &sub_iter);
  if (!dbus_message_iter_append_basic(&sub_iter, DBUS_TYPE_BOOLEAN,
                                      &set_shuffle) ||
      !dbus_message_iter_close_container(&iter, &sub_iter)) {
    std::cout << "Error appending arguments to message: " << dbus_error.message
              << std::endl;
    dbus_message_unref(dbus_msg);
    return false;
  }

  // Set destination bus name
  dbus_message_set_destination(dbus_msg,
                               players[selected_player_id].second.c_str());

  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);

  // Check for errors
  if (!dbus_reply) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    dbus_message_unref(dbus_msg);
    return false;
  }

  // Free reply message
  dbus_message_unref(dbus_reply);

  // Free message
  dbus_message_unref(dbus_msg);
  return true;
}

int64_t Player::get_position() {
  if (!is_pos_prop) {
    std::cerr << "This player does not compatible with Position property!"
              << std::endl;
    return 0;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",                  // Object path
      "org.freedesktop.DBus.Properties",          // Interface name
      "Get");                                     // Method name

  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return {};
  }
  const char *interfaceName = "org.mpris.MediaPlayer2.Player";
  const char *propertyName = "Position";
  dbus_message_append_args(dbus_msg, DBUS_TYPE_STRING, &interfaceName,
                           DBUS_TYPE_STRING, &propertyName, DBUS_TYPE_INVALID);

  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
  if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    dbus_message_unref(dbus_msg);
    return 0;
  }
  DBusMessageIter iter;
  if (dbus_message_iter_init(dbus_reply, &iter) &&
      dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
    int64_t seeked;
    dbus_message_iter_recurse(&iter, &iter);
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INT64) {
      dbus_message_iter_get_basic(&iter, &seeked);
      return seeked / 1000000;
    } else {
      std::cout << "Error decrypting reply." << std::endl;
      std::cout << "Iter arg_type: "
                << (char)dbus_message_iter_get_arg_type(&iter)
                << " when INT64 required." << std::endl;
      return 0;
    }
  } else {
    std::cout << "Error decrypting reply." << std::endl;
    std::cout << "Iter arg_type: "
              << (char)dbus_message_iter_get_arg_type(&iter)
              << " when VARIANT required." << std::endl;
    return 0;
  }
}

bool Player::set_position(int64_t pos) {
  if (!is_pos_prop) {
    std::cerr << "This player does not compatible with Position property!"
              << std::endl;
    return 0;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",                  // Object path
      "org.mpris.MediaPlayer2.Player",            // Interface name
      "SetPosition");                             // Method name
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }

  auto meta = get_metadata();
  std::string trackid;
  for (auto &info : meta) {
    if (info.first == "mpris:trackid") {
      trackid = info.second;
      break;
    }
  }
  DBusMessageIter iter;
  dbus_message_iter_init_append(dbus_msg, &iter);
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &trackid) ||
      !dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT64, &pos)) {
    std::cout << "Error appending arguments to message: " << dbus_error.message
              << std::endl;
    dbus_message_unref(dbus_msg);
    return false;
  }
  // Set destination bus name
  dbus_message_set_destination(dbus_msg,
                               players[selected_player_id].second.c_str());

  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);

  // Check for errors
  if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    dbus_message_unref(dbus_msg);
    return false;
  }

  // Free reply message
  dbus_message_unref(dbus_reply);

  // Free message
  dbus_message_unref(dbus_msg);
  return true;
}

double Player::get_volume() {
  if (!is_volume_prop) {
    std::cerr << "This player does not compatible with Volume property!"
              << std::endl;
    return 0;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",                  // Object path
      "org.freedesktop.DBus.Properties",          // Interface name
      "Get");                                     // Method name

  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return {};
  }
  const char *interfaceName = "org.mpris.MediaPlayer2.Player";
  const char *propertyName = "Volume";
  dbus_message_append_args(dbus_msg, DBUS_TYPE_STRING, &interfaceName,
                           DBUS_TYPE_STRING, &propertyName, DBUS_TYPE_INVALID);

  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
  if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    dbus_message_unref(dbus_msg);
    return 0;
  }
  DBusMessageIter iter;
  if (dbus_message_iter_init(dbus_reply, &iter) &&
      dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
    double volume;
    dbus_message_iter_recurse(&iter, &iter);
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_DOUBLE) {
      dbus_message_iter_get_basic(&iter, &volume);
      return volume;
    } else {
      std::cout << "Error decrypting reply." << std::endl;
      std::cout << "Iter arg_type: "
                << (char)dbus_message_iter_get_arg_type(&iter)
                << " when DOUBLE required." << std::endl;
      return 0;
    }
  } else {
    std::cout << "Error decrypting reply." << std::endl;
    std::cout << "Iter arg_type: "
              << (char)dbus_message_iter_get_arg_type(&iter)
              << " when VARIANT required." << std::endl;
    return 0;
  }
}

bool Player::set_volume(double volume) {
  if (!is_volume_prop) {
    std::cerr << "This player does not compatible with Volume property!"
              << std::endl;
    return 0;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",                  // Object path
      "org.freedesktop.DBus.Properties",          // Interface name
      "Set");                                     // Method name
  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return false;
  }
  const char *interfaceName = "org.mpris.MediaPlayer2.Player";
  const char *propertyName = "Volume";
  DBusMessageIter iter, sub_iter;
  dbus_message_iter_init_append(dbus_msg, &iter);
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING,
                                      &interfaceName) ||
      !dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &propertyName)) {
    std::cout << "Error appending arguments to message: " << dbus_error.message
              << std::endl;
    dbus_message_unref(dbus_msg);
    return false;
  }
  dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "d", &sub_iter);
  if (!dbus_message_iter_append_basic(&sub_iter, DBUS_TYPE_DOUBLE, &volume) ||
      !dbus_message_iter_close_container(&iter, &sub_iter)) {
    std::cout << "Error appending arguments to message: " << dbus_error.message
              << std::endl;
    dbus_message_unref(dbus_msg);
    return false;
  }

  // Set destination bus name
  dbus_message_set_destination(dbus_msg,
                               players[selected_player_id].second.c_str());

  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);

  // Check for errors
  if (!dbus_reply) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    dbus_message_unref(dbus_msg);
    return false;
  }

  // Free reply message
  dbus_message_unref(dbus_reply);

  // Free message
  dbus_message_unref(dbus_msg);
  return true;
}

std::string Player::get_current_player_name() {
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",                  // Object path
      "org.freedesktop.DBus.Properties",          // Interface name
      "Get");                                     // Method name

  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return "";
  }
  const char *interfaceName = "org.mpris.MediaPlayer2";
  const char *propertyName = "Identity";
  dbus_message_append_args(dbus_msg, DBUS_TYPE_STRING, &interfaceName,
                           DBUS_TYPE_STRING, &propertyName, DBUS_TYPE_INVALID);

  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
  if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    dbus_message_unref(dbus_msg);
    return "";
  }
  // Extract the property value from the reply
  DBusMessageIter iter;
  dbus_message_iter_init(dbus_reply, &iter);
  const char *signature = dbus_message_iter_get_signature(&iter);
  if (strcmp(signature, "v") != 0) {
    std::cout << "DBus message error: Invalid signature" << std::endl;
    return "";
  }
  DBusMessageIter valueIter;
  dbus_message_iter_recurse(&iter, &valueIter);
  const char *identity;
  dbus_message_iter_get_basic(&valueIter, &identity);

  return identity;
}

inline const char *const bool_to_string(bool b) { return b ? "true" : "false"; }

std::vector<std::pair<std::string, std::string>> Player::get_metadata() {
  DBusMessage *dbus_msg, *dbus_reply;
  if (!is_metadata_prop) {
    std::cerr << "This player does not compatible with Metadata property!"
              << std::endl;
    return {};
  }
  if (selected_player_id == -1) {
    std::cout << "get_metadata(): Player not selected, can't get metadata"
              << std::endl;
    return {};
  }
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].second.c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",                  // Object path
      "org.freedesktop.DBus.Properties",          // Interface name
      "Get");                                     // Method name

  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    return {};
  }
  const char *interfaceName = "org.mpris.MediaPlayer2.Player";
  const char *propertyName = "Metadata";
  dbus_message_append_args(dbus_msg, DBUS_TYPE_STRING, &interfaceName,
                           DBUS_TYPE_STRING, &propertyName, DBUS_TYPE_INVALID);

  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
  if (!dbus_reply || dbus_error_is_set(&dbus_error)) {
    std::cout << "Error getting reply: " << dbus_error.message << std::endl;
    return {};
  }
  std::vector<std::pair<std::string, std::string>> metadata;
  DBusMessageIter iter, dict_iter;
  if (dbus_message_iter_init(dbus_reply, &iter) &&
      dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
    // Extract variant value
    dbus_message_iter_recurse(&iter, &dict_iter);
    // Check that the type is DBUS_TYPE_ARRAY
    if (dbus_message_iter_get_arg_type(&dict_iter) != DBUS_TYPE_ARRAY) {
      std::cerr << "Metadata property is not an array" << std::endl;
      return metadata;
    }
    // Iterate over array
    dbus_message_iter_recurse(&dict_iter, &iter);
    while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_DICT_ENTRY) {
      // Iterate over dictionary entry
      DBusMessageIter dict_entry_iter;
      dbus_message_iter_recurse(&iter, &dict_entry_iter);
      // Get key as a string
      char *key_str;
      dbus_message_iter_get_basic(&dict_entry_iter, &key_str);
      // Advance iterator to value
      dbus_message_iter_next(&dict_entry_iter);
      // Get value as a variant
      dbus_message_iter_recurse(&dict_entry_iter, &dict_entry_iter);
      // Get variant type
      int variant_type = dbus_message_iter_get_arg_type(&dict_entry_iter);
      // Add key-value pair to output vector if value is a string
      if (variant_type != DBUS_TYPE_INVALID &&
          variant_type != DBUS_TYPE_SIGNATURE &&
          variant_type != DBUS_TYPE_ARRAY) {
        switch (variant_type) {
        case DBUS_TYPE_INT16: {
          short num;
          dbus_message_iter_get_basic(&dict_entry_iter, &num);
          std::string value_string = std::to_string(num);
          metadata.push_back(
              std::make_pair(std::string(key_str), value_string));
          break;
        }
        case DBUS_TYPE_UINT16: {
          unsigned short num;
          dbus_message_iter_get_basic(&dict_entry_iter, &num);
          std::string value_string = std::to_string(num);
          metadata.push_back(
              std::make_pair(std::string(key_str), value_string));
          break;
        }
        case DBUS_TYPE_INT32: {
          int num;
          dbus_message_iter_get_basic(&dict_entry_iter, &num);
          std::string value_string = std::to_string(num);
          metadata.push_back(
              std::make_pair(std::string(key_str), value_string));
          break;
        }
        case DBUS_TYPE_UINT32: {
          unsigned int num;
          dbus_message_iter_get_basic(&dict_entry_iter, &num);
          std::string value_string = std::to_string(num);
          metadata.push_back(
              std::make_pair(std::string(key_str), value_string));
          break;
        }
        case DBUS_TYPE_INT64: {
          int64_t num;
          dbus_message_iter_get_basic(&dict_entry_iter, &num);
          std::string value_string = std::to_string(num);
          metadata.push_back(
              std::make_pair(std::string(key_str), value_string));
          break;
        }
        case DBUS_TYPE_UINT64: {
          uint64_t num;
          dbus_message_iter_get_basic(&dict_entry_iter, &num);
          std::string value_string = std::to_string(num);
          metadata.push_back(
              std::make_pair(std::string(key_str), value_string));
          break;
        }
        case DBUS_TYPE_DOUBLE: {
          double num;
          dbus_message_iter_get_basic(&dict_entry_iter, &num);
          std::string value_string = std::to_string(num);
          metadata.push_back(
              std::make_pair(std::string(key_str), value_string));
          break;
        }
        case DBUS_TYPE_BYTE: {
          std::byte num;
          dbus_message_iter_get_basic(&dict_entry_iter, &num);
          // create a char* pointer and assign it the address of the byte
          char *char_ptr = reinterpret_cast<char *>(&num);
          // create a stringstream and insert the byte as a hex value
          std::stringstream ss;
          ss << std::hex << std::setfill('0') << std::setw(2)
             << static_cast<int>(*char_ptr);
          metadata.push_back(std::make_pair(std::string(key_str), ss.str()));
          break;
        }
        case DBUS_TYPE_STRING: {
          char *value_str;
          dbus_message_iter_get_basic(&dict_entry_iter, &value_str);

          metadata.push_back(
              std::make_pair(std::string(key_str), std::string(value_str)));
          break;
        }
        case DBUS_TYPE_OBJECT_PATH: {
          char *value_str;
          dbus_message_iter_get_basic(&dict_entry_iter, &value_str);

          metadata.push_back(
              std::make_pair(std::string(key_str), std::string(value_str)));
          break;
        }
        case DBUS_TYPE_BOOLEAN: {
          bool value;
          dbus_message_iter_get_basic(&dict_entry_iter, &value);

          metadata.push_back(std::make_pair(
              std::string(key_str), std::string(bool_to_string(value))));
          break;
        }
        default: {
          std::cout << "Warning: Metadata Unknown type: " << (char)variant_type
                    << std::endl;
        }
        }
      }
      // Advance to next dictionary entry
      dbus_message_iter_next(&iter);
    }
  } else {
    std::cerr << "Metadata property is not a variant" << std::endl;
  }
  return metadata;
}

std::vector<std::pair<std::string, std::string>> Player::get_output_devices() {
#ifdef HAVE_PULSEAUDIO
  // Code thatuse PulseAudio
  std::cout << "PulseAudio installed" << std::endl;
#else
  // Code that doesn't uses PulseAudio
  std::cout << "PulseAudio not installed, can't continue." << std::endl;
  return false;
#endif
  devices.clear();
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
          auto devices =
              static_cast<std::vector<std::pair<std::string, std::string>> *>(
                  userdata);
          devices->emplace_back(info->description, info->name);
        }
      },
      &devices);
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

  for (auto &device : devices) {
    std::cout << "Sink name: " << device.first << std::endl;
    std::cout << "Sink description: " << device.second << std::endl;
  }

  pa_context_disconnect(context);
  pa_context_unref(context);
  pa_mainloop_free(mainloop);

  return devices;
}

void Player::set_output_device(std::string output_sink_path) {
#ifdef HAVE_PULSEAUDIO
  // Code thatuse PulseAudio
  std::cout << "PulseAudio installed" << std::endl;
#else
  // Code that doesn't uses PulseAudio
  std::cout << "PulseAudio not installed, can't continue." << std::endl;
  return false;
#endif
  DBusMessage *dbus_msg, *dbus_reply;
  static uint32_t *proc_id = 0;
  dbus_msg = dbus_message_new_method_call(
      "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
      "GetConnectionUnixProcessID");
  const char *interface_name = players[selected_player_id].second.c_str();
  if (!dbus_message_append_args(dbus_msg, DBUS_TYPE_STRING, &interface_name,
                                DBUS_TYPE_INVALID)) {
    std::cout << "Error while appending to message" << std::endl;
    proc_id = 0;
  }

  dbus_reply = dbus_connection_send_with_reply_and_block(dbus_conn, dbus_msg,
                                                         -1, &dbus_error);

  // Get the ProcessID for player from the reply message
  DBusMessageIter iter;

  if (dbus_message_iter_init(dbus_reply, &iter) &&
      dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32) {
    dbus_message_iter_get_basic(&iter, &proc_id);
  }

  std::cout << "proc_id: " << proc_id << std::endl;

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
  static int sink_id = -1;
  // Get the list of sink devices
  static std::string player_name = players[selected_player_id].first;
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
          std::memset(player_proc_id_str, 0, sizeof(player_proc_id_str));
          std::cout << "procid: " << player_proc_id_str << std::endl;
          if (strcmp(pa_proplist_gets(info->proplist, "application.process.id"),
                     player_proc_id_str) == 0 ||
              strcmp(pa_proplist_gets(info->proplist, "application.name"),
                     player_name.c_str()) == 0) {
            std::cout << "Found current player: " << player_name << ". #"
                      << info->sink << std::endl;
            int *sink_id = static_cast<int *>(userdata);
            *sink_id = info->index;
            // info->client
            return;
          }
        }
      },
      &sink_id);
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

  if (sink_id == -1) {
    std::cout << "Not found player sink. Can't continue." << std::endl;
    return;
  }

  std::cout << "Trying to change output device for sink " << sink_id << " to "
            << output_sink_path << std::endl;
  static const std::string output_sink_path_copy = output_sink_path;

  // Get the list of sink devices
  pa_operation *operation2 = pa_context_move_sink_input_by_name(
      context, sink_id, output_sink_path_copy.c_str(), NULL, NULL);
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
}

bool Player::get_play_pause_method() const { return play_pause_method; }

bool Player::get_pause_method() const { return pause_method; }

bool Player::get_play_method() const { return play_method; }

bool Player::get_next_method() const { return next_method; }

bool Player::get_previous_method() const { return previous_method; }

bool Player::get_setpos_method() const { return setpos_method; }

bool Player::get_is_shuffle_prop() const { return is_shuffle_prop; }

bool Player::get_is_pos_prop() const { return is_pos_prop; }

bool Player::get_is_volume_prop() const { return is_volume_prop; }

bool Player::get_is_playback_status_prop() const {
  return is_playback_status_prop;
}

bool Player::get_is_metadata_prop() const { return is_metadata_prop; }

unsigned int Player::get_count_of_players() const { return players.size(); }
