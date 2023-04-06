#ifndef PLAYER_H
#define PLAYER_H

#include "helper.h"
#include "pugixml.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <vector>

#ifdef HAVE_PULSEAUDIO
#include <pulse/proplist.h>
#include <pulse/pulseaudio.h>
#endif

#ifdef SUPPORT_AUDIO_OUTPUT
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <mutex>
#include <sndfile.h>
#include <unistd.h>
#endif

#ifdef HAVE_DBUS
#include <sdbus-c++/sdbus-c++.h>
#endif

class PlayerObserver {
public:
  virtual void on_song_title_changed(const std::string &new_song_name) = 0;
  virtual void on_song_artist_changed(const std::string &new_song_author) = 0;
  virtual void on_song_length_changed(const std::string &new_song_length) = 0;
  virtual void on_is_shuffle_changed(const bool &new_is_shuffle) = 0;
  virtual void on_is_playing_changed(const bool &new_is_playing) = 0;
  virtual void on_song_volume_changed(const double &new_song_volume) = 0;
  virtual void on_song_position_changed(const uint64_t &new_song_pos) = 0;
};

class Player {
private:
  std::vector<std::pair<std::string, std::string>>
      m_players; // Name: dbus interface
  std::vector<std::pair<std::string, unsigned short>>
      m_devices; // Name: pulseaudio sink index
  std::list<PlayerObserver *> m_observers;
#ifdef HAVE_DBUS
  std::unique_ptr<sdbus::IConnection> m_dbus_conn;
  std::unique_ptr<sdbus::IProxy> m_proxy_signal;
#endif
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

  std::string m_song_title, m_song_artist, m_song_length_str;
  uint64_t m_song_pos, m_song_length;
  bool m_is_shuffle, m_is_playing;
  double m_song_volume;
#ifdef SUPPORT_AUDIO_OUTPUT
  Mix_Music *m_current_music = nullptr;
  // std::vector<Mix_Music *> m_playlist;
#endif

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
  std::string get_position_str();
  bool set_position(int64_t pos);
  uint64_t get_song_length();
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
  bool get_is_playing() const;
  void set_is_playing(bool new_is_playing);

#ifdef HAVE_DBUS
  void stop_listening_signals();
  void start_listening_signals();
  void on_properties_changed(sdbus::Signal &signal);
  void on_seeked(sdbus::Signal &signal);
#endif
  void add_observer(PlayerObserver *observer);
  void remove_observer(PlayerObserver *observer);

  std::string get_song_name() const;
  void set_song_name(const std::string &new_song_name);
  std::string get_song_author() const;
  void set_song_author(const std::string &new_song_author);
  std::string get_song_length_str() const;
  void set_song_length(const std::string &new_song_length);
  void get_song_data();

  void notify_observers_song_title_changed();
  void notify_observers_song_artist_changed();
  void notify_observers_song_length_changed();
  void notify_observers_is_shuffle_changed();
  void notify_observers_is_playing_changed();
  void notify_observers_song_volume_changed();
  void notify_observers_song_position_changed();

#ifdef SUPPORT_AUDIO_OUTPUT
  void open_audio(const std::string &filename);
  void play_audio();
  void stop_audio();
  void pause_audio();
  Mix_Music *get_music() const;
  std::vector<Mix_Music *> get_playlist() const;
  void add_to_playlist(Mix_Music *);
#endif
};
#endif // PLAYER_H
