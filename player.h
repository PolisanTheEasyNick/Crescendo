#ifndef PLAYER_H
#define PLAYER_H
#include <cstring>
#include <dbus/dbus.h>
#include <iostream>

#include <vector>

class Player {
private:
  std::vector<std::string> players;
  DBusConnection *dbus_conn = nullptr;
  DBusError dbus_error;
  unsigned int selected_player_id = -1;

public:
  Player();
  ~Player();
  bool get_players(); // get players and write it into players vector
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
  std::vector<std::pair<std::string, std::string>> get_metadata();
  // TODO
  /* 1. Check for abilities for checked player. Many bools with writed abilities
   * paths?
   * 2. GUI........
   * 3. Ability to find sound devices and switch between them
   *
   */
};

#endif // PLAYER_H
