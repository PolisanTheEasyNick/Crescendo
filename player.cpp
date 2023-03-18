#include "player.h"

Player::Player() {
  // Initialize D-Bus error
  dbus_error_init(&dbus_error);
  // Connect to D-Bus
  dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
  get_players();
}

Player::~Player() {
  dbus_connection_unref(dbus_conn);
  dbus_error_free(&dbus_error);
}

bool Player::get_players() {
  if (dbus_error_is_set(&dbus_error)) {
    printf("DBus error: %s\n", dbus_error.message);
    return false;
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
    return false;
  }
  // Invoke remote procedure call, block for response
  dbus_reply = dbus_connection_send_with_reply_and_block(
      dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);

  if (!dbus_reply) {
    fprintf(stderr, "Error getting reply: %s\n", dbus_error.message);
    return false;
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
        printf("Found media player: %s\n", name);
        players.push_back(name);
      }
    }
  }

  if (players.empty()) {
    std::cout << "No media players found." << std::endl;
    return false;
  }
  dbus_message_unref(dbus_msg);
  dbus_message_unref(dbus_reply);
  return true;
}

void Player::print_players() {
  for (auto &player : players) {
    std::cout << player << std::endl;
  }
}

void Player::print_players_names() {
  for (auto &player : players) {
    DBusMessage *dbus_msg, *dbus_reply;
    // Compose remote procedure call
    dbus_msg = dbus_message_new_method_call(
        player.c_str(),                    // Destination bus name
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
  if (new_id < 0 || new_id > players.size()) {
    std::cout << "This player does not exists!" << std::endl;
    return false;
  }
  selected_player_id = new_id;
  return true;
}

bool Player::send_play_pause() {
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), "/org/mpris/MediaPlayer2",
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
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), "/org/mpris/MediaPlayer2",
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
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), "/org/mpris/MediaPlayer2",
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
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), "/org/mpris/MediaPlayer2",
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
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), "/org/mpris/MediaPlayer2",
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
  DBusMessage *dbus_msg, *dbus_reply;
  if (selected_player_id == -1) {
    std::cout << "get_metadata(): Player not selected, can't get metadata"
              << std::endl;
    return false;
  }
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",           // Object path
      "org.freedesktop.DBus.Properties",   // Interface name
      "Get");                              // Method name

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
  if (dbus_error_is_set(&dbus_error)) {
    printf("DBus error: %s\n", dbus_error.message);
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",           // Object path
      "org.freedesktop.DBus.Properties",   // Interface name
      "Set");                              // Method name
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
  dbus_message_set_destination(dbus_msg, players[selected_player_id].c_str());

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
  DBusMessage *dbus_msg, *dbus_reply;
  if (selected_player_id == -1) {
    std::cout << "get_position(): Player not selected, can't get metadata"
              << std::endl;
    return 0;
  }
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",           // Object path
      "org.freedesktop.DBus.Properties",   // Interface name
      "Get");                              // Method name

  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    dbus_connection_unref(dbus_conn);
    std::string error_msg = dbus_error.message;
    dbus_error_free(&dbus_error);
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
    dbus_connection_unref(dbus_conn);
    std::string error_msg = dbus_error.message;
    dbus_error_free(&dbus_error);
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
  if (dbus_error_is_set(&dbus_error)) {
    printf("DBus error: %s\n", dbus_error.message);
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  if (players[selected_player_id] == "org.mpris.MediaPlayer2.spotify") {
    dbus_msg = dbus_message_new_method_call(
        players[selected_player_id].c_str(), // Destination bus name
        "/org/mpris/MediaPlayer2",           // Object path
        "org.mpris.MediaPlayer2.Player",     // Interface name
        "SetPosition");                      // Method name
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
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH,
                                        &trackid) ||
        !dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT64, &pos)) {
      std::cout << "Error appending arguments to message: "
                << dbus_error.message << std::endl;
      dbus_message_unref(dbus_msg);
      return false;
    }
    // Set destination bus name
    dbus_message_set_destination(dbus_msg, players[selected_player_id].c_str());

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
  } else {
    return false;
  }
}

double Player::get_volume() {
  DBusMessage *dbus_msg, *dbus_reply;
  if (selected_player_id == -1) {
    std::cout << "get_volume(): Player not selected, can't get volume"
              << std::endl;
    return 0;
  }
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",           // Object path
      "org.freedesktop.DBus.Properties",   // Interface name
      "Get");                              // Method name

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
  if (dbus_error_is_set(&dbus_error)) {
    printf("DBus error: %s\n", dbus_error.message);
    return false;
  }
  DBusMessage *dbus_msg, *dbus_reply;
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",           // Object path
      "org.freedesktop.DBus.Properties",   // Interface name
      "Set");                              // Method name
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
  dbus_message_set_destination(dbus_msg, players[selected_player_id].c_str());

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

std::vector<std::pair<std::string, std::string>> Player::get_metadata() {
  DBusMessage *dbus_msg, *dbus_reply;
  if (selected_player_id == -1) {
    std::cout << "get_metadata(): Player not selected, can't get metadata"
              << std::endl;
    return {};
  }
  // Compose remote procedure call
  dbus_msg = dbus_message_new_method_call(
      players[selected_player_id].c_str(), // Destination bus name
      "/org/mpris/MediaPlayer2",           // Object path
      "org.freedesktop.DBus.Properties",   // Interface name
      "Get");                              // Method name

  if (!dbus_msg) {
    std::cout << "Error creating message." << std::endl;
    dbus_connection_unref(dbus_conn);
    std::string error_msg = dbus_error.message;
    dbus_error_free(&dbus_error);
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
    dbus_message_unref(dbus_msg);
    dbus_connection_unref(dbus_conn);
    std::string error_msg = dbus_error.message;
    dbus_error_free(&dbus_error);
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
      if (variant_type == DBUS_TYPE_STRING) {
        char *value_str;
        dbus_message_iter_get_basic(&dict_entry_iter, &value_str);
        metadata.push_back(
            std::make_pair(std::string(key_str), std::string(value_str)));
      }
      // Advance to next dictionary entry
      dbus_message_iter_next(&iter);
    }
  } else {
    std::cerr << "Metadata property is not a variant" << std::endl;
  }
  return metadata;
}
