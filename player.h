#ifndef PLAYER_H
#define PLAYER_H
#include "pugixml.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <pulse/proplist.h>
#include <pulse/pulseaudio.h>
#include <sdbus-c++/sdbus-c++.h>
#include <sstream>
#include <vector>

class Player {
private:
  std::vector<std::pair<std::string, std::string>>
      m_players; // Name: dbus interface
  std::vector<std::pair<std::string, unsigned short>>
      m_devices; // Name: pulseaudio sink index
  std::unique_ptr<sdbus::IConnection> m_dbus_conn;
  unsigned int m_selected_player_id = -1;
  bool m_play_pause_method = false;
  bool m_pause_method = false;
  bool m_play_method = false;
  bool m_next_method = false;
  bool m_previous_method = false;
  bool m_setpos_method = false;
  bool m_is_shuffle_prop = false;
  bool m_is_pos_prop = false;

  bool m_is_volume_prop = false;
  bool m_is_playback_status_prop = false;
  bool m_is_metadata_prop = false;

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
  bool get_playback_status();
  std::string get_current_player_name();
  unsigned short get_current_device_sink_index();
  std::vector<std::pair<std::string, std::string>> get_metadata();
  std::vector<std::pair<std::string, unsigned short>> get_output_devices();
  void set_output_device(unsigned short);

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
