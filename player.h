#ifndef PLAYER_H
#define PLAYER_H
#include "pugixml.hpp"
#include <cstring>
#include <dbus/dbus.h>
#include <iomanip>
#include <iostream>
#include <pulse/proplist.h>
#include <pulse/pulseaudio.h>
#include <sstream>
#include <vector>

class Player {
private:
  std::vector<std::pair<std::string, std::string>> players, devices;
  DBusConnection *dbus_conn = nullptr;
  DBusError dbus_error;
  unsigned int selected_player_id = -1;
  bool play_pause_method = false;
  bool pause_method = false;
  bool play_method = false;
  bool next_method = false;
  bool previous_method = false;
  bool setpos_method = false;
  bool is_shuffle_prop = false;
  bool is_pos_prop = false;
  bool is_volume_prop = false;
  bool is_playback_status_prop = false;
  bool is_metadata_prop = false;

public:
  Player();
  ~Player();
  std::vector<std::pair<std::string, std::string>> get_players();
  void print_players();
  void print_players_names();
  bool select_player(unsigned int id); // select player id
  bool send_play_pause();
  bool send_pause();
  bool send_play();
  bool send_next();
  bool send_previous();
  bool get_shuffle();
  bool set_shuffle(bool isShuffle);
  int64_t get_position();
  bool set_position(int64_t pos);
  double get_volume();
  bool set_volume(double volume);
  std::string get_current_player_name();
  std::vector<std::pair<std::string, std::string>> get_metadata();
  std::vector<std::pair<std::string, std::string>> get_output_devices();
  void set_output_device(std::string sink_name);

  bool get_play_pause_method() const;
  bool get_pause_method() const;
  bool get_play_method() const;
  bool get_next_method() const;
  bool get_previous_method() const;
  bool get_setpos_method() const;
  bool get_is_shuffle_prop() const;
  bool get_is_pos_prop() const;
  bool get_is_volume_prop() const;
  bool get_is_playback_status_prop() const;
  bool get_is_metadata_prop() const;
  unsigned int get_count_of_players() const;
};

#endif // PLAYER_H
