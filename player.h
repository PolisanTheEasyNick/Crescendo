#ifndef PLAYER_H
#define PLAYER_H

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <vector>

#include "helper.h"
#include "pugixml.hpp"

#ifdef HAVE_PULSEAUDIO
#include <pulse/proplist.h>
#include <pulse/pulseaudio.h>
#endif

#ifdef SUPPORT_AUDIO_OUTPUT
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <sndfile.h>
#include <unistd.h>

#include <mutex>
#endif

#ifdef HAVE_DBUS
#include <sdbus-c++/sdbus-c++.h>
#endif

class PlayerObserver {
 public:
  /**
   * Function, that will be called when song title changed
   *
   * @param new_song_name New song title (type: const std::string&)
   */
  virtual void on_song_title_changed(const std::string &new_song_name) = 0;
  /**
   * Function, that will be called when song author changed
   *
   * @param new_song_author New song author (type: const std::string&)
   */
  virtual void on_song_artist_changed(const std::string &new_song_author) = 0;
  /**
   * Function, that will be called when song length changed
   *
   * @param new_song_length New song length (type: const std::string&)
   */
  virtual void on_song_length_changed(const std::string &new_song_length) = 0;
  /**
   * Function, that will be called when player's shuffle property changed
   *
   * @param new_is_shuffle New property status (type: const bool&)
   */
  virtual void on_is_shuffle_changed(const bool &new_is_shuffle) = 0;
  /**
   * Function, that will be called when player's is_playing property changed
   *
   * @param new_is_playing New property status (type: const bool&)
   */
  virtual void on_is_playing_changed(const bool &new_is_playing) = 0;
  /**
   * Function, that will be called when player's volume changed
   *
   * @param new_song_volume New volume (type: const double&)
   */
  virtual void on_song_volume_changed(const double &new_song_volume) = 0;
  /**
   * Function, that will be called when player's position changed (by seeking)
   *
   * @param new_song_pos New volume (type: const uint64_t&)
   */
  virtual void on_song_position_changed(const uint64_t &new_song_pos) = 0;
  /**
   * Function, that will be called when player's loop status changed
   *
   * @param new_loop_status New volume (type: const int&)
   */
  virtual void on_loop_status_changed(const int &new_loop_status) = 0;
};

class Player {
 private:
  /**
   * Vector of DBus accessible players
   * This vector contains pairs of std::string-std::string
   * First string contain Name of the player
   * Second string contain DBus interface of player
   */
  std::vector<std::pair<std::string, std::string>>
      m_players;  // Name: dbus interface
  /**
   * Vector of PulseAudio accessible output devices
   * This vector contains pairs of std::string-unsigned short
   * String contain Name of the output device
   * unsighed short contain PulseAudio sink index
   */
  std::vector<std::pair<std::string, unsigned short>>
      m_devices;  // Name: pulseaudio sink index
  /**
   * List of observers, which need to be notified
   * when some property of player is changed
   */
  std::list<PlayerObserver *> m_observers;

#ifdef HAVE_DBUS
  /**
   * DBus Connection pointer
   */
  std::unique_ptr<sdbus::IConnection> m_dbus_conn;
  /**
   * DBus Proxy pointer
   */
  std::unique_ptr<sdbus::IProxy> m_proxy_signal;
#endif
  /**
   * Currently selected player ID
   */
  unsigned int m_selected_player_id = -1;
  /**
   * Whether current selected player support PlayPause DBus method
   */
  bool m_play_pause_method = false;
  /**
   * Whether current selected player support Pause DBus method
   */
  bool m_pause_method = false;
  /**
   * Whether current selected player support Play DBus method
   */
  bool m_play_method = false;
  /**
   * Whether current selected player support Next DBus method
   */
  bool m_next_method = false;
  /**
   * Whether current selected player support Previous DBus method
   */
  bool m_previous_method = false;
  /**
   * Whether current selected player support SetPos DBus method
   */
  bool m_setpos_method = false;
  /**
   * Whether current selected player support Shuffle DBus property
   */
  bool m_is_shuffle_prop = false;
  /**
   * Whether current selected player support Position DBus property
   */
  bool m_is_pos_prop = false;
  /**
   * Whether current selected player support Volume DBus property
   */
  bool m_is_volume_prop = false;
  /**
   * Whether current selected player support PlaybackStatus DBus property
   */
  bool m_is_playback_status_prop = false;
  /**
   * Whether current selected player support Metadata DBus property
   */
  bool m_is_metadata_prop = false;
  /**
   * Whether current selected player support LoopStatus DBus property
   */
  bool m_is_repeat_prop = false;
  /**
   * Current status of LoopStatus property of player.
   * 0 is None
   * 1 is Playlist
   * 2 is Track
   */
  int m_repeat = 0;
  /**
   * m_song_title - title of song
   * m_song_artist - artist of song
   * m_song_length_str - formatted song length
   */
  std::string m_song_title, m_song_artist, m_song_length_str;
  /**
   * m_song_pos - Song current position, in seconds
   * m_song_length - Song length, in seconds
   */
  uint64_t m_song_pos, m_song_length;
  /**
   * m_is_shuffle - Whether shuffle enabled
   * m_is_playing - Whether song is playing
   */
  bool m_is_shuffle, m_is_playing;
  /**
   * Current player's volume
   */
  double m_song_volume;
#ifdef SUPPORT_AUDIO_OUTPUT
  /**
   * Pointer to current selected Music for local player
   */
  Mix_Music *m_current_music = nullptr;
#endif
  /**
   * Whether to use player class with gui or not
   */
  bool m_with_gui;

 public:
  /**
   * Constructs a new Player object.
   */
  Player(bool with_gui = true);
  /**
   * Destructs a new Player object.
   */
  ~Player();
  /**
   * Returns a vector of pairs representing available media players. Each pair
   * contains the identity of the media player and its DBus name.
   * If audio output is supported, a "Local" player is added to the list.
   * If DBus is available, the function queries the system for running media
   * players via DBus, and adds their identities and DBus names to the list.
   * @return A vector of pairs representing available media players.
   * If no players are found, an empty vector is returned.
   */
  std::vector<std::pair<std::string, std::string>> get_players();
  /**
   * Prints all currently available players DBus interfaces
   */
  void print_players();
  /**
   * Prints all currently available players names
   */
  void print_players_names();
  /**
   * Changes currently selected player
   * @param id - new player id (type: unsigned int)
   * @return true on success, false otherwise (type: bool)
   */
  bool select_player(unsigned int id);  // select player id
  /**
   * Sends PlayPause signal to DBus or Play/Pause current song for local player
   * @return true on success, false otherwise (type: bool)
   */
  bool send_play_pause();
  /**
   * Sends Pause signal to DBus or Pauses current song for local player
   * @return true on success, false otherwise (type: bool)
   */
  bool send_pause();
  /**
   * Sends Play signal to DBus or start playing current song for local player
   * @return true on success, false otherwise (type: bool)
   */
  bool send_play();
  /**
   * Sends Next signal to DBus or choose next song for local player
   * @return true on success, false otherwise (type: bool)
   */
  bool send_next();
  /**
   * Sends Previous signal to DBus or choose previous song for local player
   * @return true on success, false otherwise (type: bool)
   */
  bool send_previous();
  /**
   * Gets current shuffle status for currently selected player
   * @return true if shuffle enabled, false if shuffle disabled (type: bool)
   */
  bool get_shuffle();
  /**
   * Sets current shuffle status for currently selected player
   * @param isShuffle - new shuffle status (type: bool)
   * @return true if shuffle successully changed, false otherwise (type: bool)
   */
  bool set_shuffle(bool isShuffle);
  /**
   * Gets current LoopStatus for currently selected player
   * @return  0 if LoopStatus is "None", 1 if LoopStatus is "Playlist", 2 if
   * LoopStatus is "Track"
   */
  int get_repeat();
  /**
   * Sets current LoopStatus for currently selected player
   * @param new_repeat - new LoopStatus (type: int)
   * @return true if LoopStatus successully changed, false otherwise (type:
   * bool)
   */
  bool set_repeat(int new_repeat);
  /**
   * Gets current Position for currently selected player
   * @return current position in seconds (type: int64_t)
   */
  int64_t get_position();
  /**
   * Gets current Position for currently selected player
   * @return current position in formated time (type: std::string)
   */
  std::string get_position_str();
  /**
   * Sets current LoopStatus for currently selected player
   * @param pos - new position (type: int64_t)
   * @return true if LoopStatus successully changed, false otherwise (type:
   * bool)
   */
  bool set_position(int64_t pos);
  /**
   * Gets current song length for currently selected player
   * @return current song length in seconds (type: int64_t)
   */
  uint64_t get_song_length();
  /**
   * Gets current player volume
   * @return current volume from 0 to 1 (type: double)
   */
  double get_volume();
  /**
   * Sets current volume for currently selected player
   * @param volume - new volume (type: double)
   * @return true if volume successully changed, false otherwise (type:
   * bool)
   */
  bool set_volume(double volume);
  /**
   * Gets current playback status
   * @return true if some song is playing, false otherwise (type:
   * bool)
   */
  bool get_playback_status();
  /**
   * Gets current player name
   * @return current player name, (type: string)
   */
  std::string get_current_player_name();
  /**
   * Gets current output device PulseAudio sink
   * @return current output device PulseAudio sink, (type: unsigned short)
   */
  unsigned short get_current_device_sink_index();
  /**
   * Gets current player metadata in DBus format
   * From this you can get xesam:title, xesam:artist, mpris:length and so on
   *
   * @return current player metadata (type: std::vector of std::pair of
   * std::string, std::string)
   */
  std::vector<std::pair<std::string, std::string>> get_metadata();
  /**
   * Gets current available output PulseAudio devices
   *
   * @return current player metadata (type: std::vector of std::pair of
   * std::string, unsigned short)
   */
  std::vector<std::pair<std::string, unsigned short>> get_output_devices();
  /**
   * Sets new output PulseAudio device
   * @param sink index for new output device
   */
  void set_output_device(unsigned short);
  /**
   * Gets whether current player supports PlayPause method
   * @return true if player support PlayPause, false otherwise (type: bool)
   */
  bool get_play_pause_method() const;
  /**
   * Gets whether current player supports Pause method
   * @return true if player support Pause, false otherwise (type: bool)
   */
  bool get_pause_method() const;
  /**
   * Gets whether current player supports Play method
   * @return true if player support Play, false otherwise (type: bool)
   */
  bool get_play_method() const;
  /**
   * Gets whether current player supports Next method
   * @return true if player support Next, false otherwise (type: bool)
   */
  bool get_next_method() const;
  /**
   * Gets whether current player supports Previous method
   * @return true if player support Previous, false otherwise (type: bool)
   */
  bool get_previous_method() const;
  /**
   * Gets whether current player supports SetPosition method
   * @return true if player support SetPosition, false otherwise (type: bool)
   */
  bool get_setpos_method() const;
  /**
   * Gets whether current player supports Shuffle property
   * @return true if player support Shuffle property, false otherwise (type:
   * bool)
   */
  bool get_is_shuffle_prop() const;
  /**
   * Gets whether current player supports Position property
   * @return true if player support Position property, false otherwise (type:
   * bool)
   */
  bool get_is_pos_prop() const;
  /**
   * Gets whether current player supports Volume property
   * @return true if player support Volume property, false otherwise (type:
   * bool)
   */
  bool get_is_volume_prop() const;
  /**
   * Gets whether current player supports PlaybackStatus property
   * @return true if player support PlaybackStatus property, false otherwise
   * (type: bool)
   */
  bool get_is_playback_status_prop() const;
  /**
   * Gets whether current player supports Metadata property
   * @return true if player support Metadata property, false otherwise (type:
   * bool)
   */
  bool get_is_metadata_prop() const;
  /**
   * Gets whether current player supports Loop property
   * @return true if player support Loop property, false otherwise (type:
   * bool)
   */
  bool get_is_repeat_prop() const;
  /**
   * Gets count of current available players
   * @return count of players (type: unsigned int)
   */
  unsigned int get_count_of_players() const;
  /**
   * Gets whether current player playing some media right now
   * @return true if player playing some, false otherwise (type: bool)
   */
  bool get_is_playing() const;
  /**
   * Sets whether current player playing some media right now
   * @param new_is_playing new status whether player currently plays something
   */
  void set_is_playing(bool new_is_playing);

#ifdef HAVE_DBUS
  /**
   * Stops listening signals for player using DBus
   */
  void stop_listening_signals();
  /**
   * Start listening signals for player using DBus
   */
  void start_listening_signals();
  /**
   * Callback function which processes property changes from DBus
   */
  void on_properties_changed(sdbus::Signal &signal);
  /**
   * Callback function which processes new position of song
   */
  void on_seeked(sdbus::Signal &signal);
  /**
   * Thread function which prints current position
   */
  void update_position_thread();
#endif
  /**
   * Adds new observer, which be notified when player or song properties
   * changes
   * @param observer class object, that must be notified
   */
  void add_observer(PlayerObserver *observer);
  /**
   * Removes observer
   * @param observer class object, that must be removed
   */
  void remove_observer(PlayerObserver *observer);

  /**
   * Gets the name of the song.
   *
   * @return The name of the song as a string.
   */
  std::string get_song_name() const;

  /**
   * Sets the name of the song.
   *
   * @param new_song_name The new name of the song as a string.
   */
  void set_song_name(const std::string &new_song_name);

  /**
   * Gets the author of the song.
   *
   * @return The author of the song as a string.
   */
  std::string get_song_author() const;

  /**
   * Sets the author of the song.
   *
   * @param new_song_author The new author of the song as a string.
   */
  void set_song_author(const std::string &new_song_author);

  /**
   * Gets the length of the song as a string.
   *
   * @return The length of the song as a string in the format "mm:ss".
   */
  std::string get_song_length_str() const;

  /**
   * Sets the length of the song.
   *
   * @param new_song_length The new length of the song as a string in the format
   * "mm:ss".
   */
  void set_song_length(const std::string &new_song_length);

  /**
   * Retrieves the song data from an external data source.
   *
   * This function retrieves the song data from an external data source, such as
   * a file or a database. It sets the song name, author, and length based on
   * the data retrieved.
   */
  void get_song_data();

  /**
   * Notifies observers that the title of the currently playing song has
   * changed.
   */
  void notify_observers_song_title_changed();

  /**
   * Notifies observers that the artist of the currently playing song has
   * changed.
   */
  void notify_observers_song_artist_changed();

  /**
   * Notifies observers that the length of the currently playing song has
   * changed.
   */
  void notify_observers_song_length_changed();

  /**
   * Notifies observers that the shuffle status of the player has changed.
   */
  void notify_observers_is_shuffle_changed();

  /**
   * Notifies observers that the playing status of the player has changed.
   */
  void notify_observers_is_playing_changed();

  /**
   * Notifies observers that the volume of the player has changed.
   */
  void notify_observers_song_volume_changed();

  /**
   * Notifies observers that the position in the currently playing song has
   * changed.
   */
  void notify_observers_song_position_changed();

  /**
   * Notifies observers that the loop status of the player has changed.
   */
  void notify_observers_loop_status_changed();

#ifdef SUPPORT_AUDIO_OUTPUT
  /**
   * Opens an audio file for playback.
   *
   * @param filename The filename of the audio file to open.
   * @return True if the file was opened successfully, false otherwise.
   */
  bool open_audio(const std::string &filename);

  /**
   * Starts playing the currently open audio file.
   */
  void play_audio();

  /**
   * Stops playing the currently open audio file.
   */
  void stop_audio();

  /**
   * Pauses playback of the currently open audio file.
   */
  void pause_audio();

  /**
   * Gets a pointer to the Mix_Music object representing the currently open
   * audio file.
   *
   * @return A pointer to the Mix_Music object representing the currently open
   * audio file, or NULL if no file is currently open.
   */
  Mix_Music *get_music() const;

  /**
   * Gets a vector of pointers to the Mix_Music objects in the audio player's
   * playlist.
   *
   * @return A vector of pointers to the Mix_Music objects in the audio player's
   * playlist.
   */
  std::vector<Mix_Music *> get_playlist() const;

  /**
   * Adds a Mix_Music object to the audio player's playlist.
   *
   * @param music A pointer to the Mix_Music object to add to the playlist.
   */
  void add_to_playlist(Mix_Music *music);
#endif
};
#endif  // PLAYER_H
