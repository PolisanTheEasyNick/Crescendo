#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

// #include <gio/gfile.h>
#include <glib.h>
#include <gtkmm/alertdialog.h>
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/droptarget.h>
#include <gtkmm/filedialog.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturelongpress.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/popover.h>
#include <gtkmm/scale.h>
#include <gtkmm/scalebutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/viewport.h>
#include <sigc++/signal.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <random>
#include <thread>
#include <tuple>

#include "player.h"
#include "playlistrow.h"
#include "volumebutton.h"

/**

* A class representing the main window of the music player application
* This class inherits from both Gtk::ApplicationWindow and PlayerObserver.
* It is responsible for displaying the current playing song information and
* updating the UI accordingly when changes occur to the player's state.
*/
class PlayerWindow : public Gtk::ApplicationWindow, public PlayerObserver {
 public:
  /**
   * Default constructor for PlayerWindow
   */
  PlayerWindow();
  /**
   * Default destructor for PlayerWindow. Stops position update thread
   * and frees memory for m_playlist_scrolled_window
   */
  virtual ~PlayerWindow() {
    // Stop the position thread
    stop_position_thread();
    delete m_playlist_scrolled_window;
  }
  /**
   * Override method called when the current song's title changes
   * Updates the UI to display the new song title.
   * @param new_song The new song title
   */
  void on_song_title_changed(const std::string &new_song) override {
    Helper::get_instance().log("New song name PlayerWindow: " + new_song);
    m_song_title_label.set_label(new_song);
  }
  /**
   * Override method called when the current song's artist changes
   * Updates the UI to display the new song artist.
   * @param new_song_artist The new song artist
   */
  void on_song_artist_changed(const std::string &new_song_artist) override {
    Helper::get_instance().log("New song author PlayerWindow: " +
                               new_song_artist);
    m_song_artist_label.set_label(new_song_artist);
  }
  /**
   * Override method called when the current song's length changes
   * Updates the UI to display the new song length.
   * @param new_song_length The new song length
   */
  void on_song_length_changed(const std::string &new_song_length) override {
    Helper::get_instance().log("New song length PlayerWindow: " +
                               new_song_length);
    m_song_length_label.set_label(new_song_length);
  }
  /**
   * Override method called when the current player's shuffle state changes
   * Updates the UI to display the new player's shuffle state.
   * @param new_is_shuffle The new player's shuffle state.
   */
  void on_is_shuffle_changed(const bool &new_is_shuffle) override {
    Helper::get_instance().log("New song shuffle PlayerWindow: " +
                               std::to_string(new_is_shuffle));
    if (new_is_shuffle) {
      m_shuffle_button.get_style_context()->add_class("shuffle-enabled");
    } else {
      m_shuffle_button.get_style_context()->remove_class("shuffle-enabled");
    }
  }
  /**
   * Override method called when the current player's is_playing state changes
   * Updates the UI to display the new player's is_playing state.
   * @param new_is_playing The new player's is_playing state.
   */
  void on_is_playing_changed(const bool &new_is_playing) override {
    Helper::get_instance().log("New song isplaying PlayerWindow: " +
                               std::to_string(new_is_playing));
    if (new_is_playing) {
      // Resume the position thread
      resume_position_thread();
      m_playpause_button.set_icon_name("media-pause");
    } else {
      // Stop the position thread
      stop_position_thread();
      m_playpause_button.set_icon_name("media-play");
    }
  }
  /**
   * Override method called when the current player's volume changes
   * Updates the UI to display the new player's volume.
   * @param new_song_volume The new player's volume.
   */
  void on_song_volume_changed(const double &new_song_volume) override {
    Helper::get_instance().log("New song volume PlayerWindow: " +
                               std::to_string(new_song_volume));
    m_lock_volume_changing = true;
    m_volume_bar_scale_button.set_value(new_song_volume);
    m_lock_volume_changing = false;
  }

  /**
   * Override method called when the current song position changes
   * Updates the UI to display the new song position.
   * @param new_song_pos The new song position.
   */
  void on_song_position_changed(const uint64_t &new_song_pos) override {
    Helper::get_instance().log("New song pos: " + std::to_string(new_song_pos));
    m_current_pos_label.set_label(
        Helper::get_instance().format_time(new_song_pos));
    double pos = m_player.get_position();
    double len = m_player.get_song_length();
    double new_pos = pos / len;
    m_lock_pos_changing = true;
    m_progress_bar_song_scale.set_value(new_pos);
    m_lock_pos_changing = false;
  }

  /**
   * Override method called when the current player loop status changes
   * Updates the UI to display the new song position.
   * @param new_loop_status The new player loop status.
   */
  void on_loop_status_changed(const int &new_loop_status) override {
    Helper::get_instance().log("New loop status: " +
                               std::to_string(new_loop_status));
    if (new_loop_status == -1 || new_loop_status == 0) {  // none
      m_repeat_button.set_icon_name("media-repeat-none");
    } else if (new_loop_status == 1) {  // playlist
      m_repeat_button.set_icon_name("media-repeat-all");
    } else if (new_loop_status == 2) {  // song
      m_repeat_button.set_icon_name("media-repeat-single");
    } else {  // none
      m_repeat_button.set_icon_name("media-repeat-none");
    }
  }

#ifdef SUPPORT_AUDIO_OUTPUT
  /**
   * Adds new song to playlist
   * @param filename Song path (type: std::string)
   */
  void add_song_to_playlist(const std::string &filename);
  /**
   * Callback function, which called after current music ends
   */
  static void on_music_ends();
#endif

 protected:
  static Player m_player;  // Player object
  /**
   * Callback function after button clicked
   */
  void on_playpause_clicked(), on_prev_clicked(), on_next_clicked(),
      on_shuffle_clicked(), on_player_choose_clicked(),
      on_device_choose_clicked(), on_loop_clicked();
  Gtk::Grid m_main_grid;  // Main UI Grid
  Gtk::Box m_control_buttons_box,
      m_volume_and_player_box;  // Box that contains buttons
  Gtk::Button m_playpause_button, m_prev_button, m_next_button,
      m_shuffle_button, m_player_choose_button, m_device_choose_button,
      m_add_song_to_playlist_button, m_repeat_button;
  Gtk::Label m_song_title_label, m_song_artist_label, m_current_pos_label,
      m_song_length_label;
  Gtk::Scale m_progress_bar_song_scale;  // Scale which contains progress bar
  /**
   * VolumeButton object, which creates button, that on click creates popup for
   * volume choosing
   */
  VolumeButton m_volume_bar_scale_button;
  Gtk::Popover m_player_choose_popover,
      m_device_choose_popover;  // Popover for choosing Player or Device
  Gtk::ListBox m_song_title_list,
      m_playlist_listbox;  // Listbox for title and artist or for playlist items
  static PlaylistRow
      *m_activated_row;  // Pointer to currently selected row in local player

  std::atomic_bool stop_flag{false};  // Flag to signal thread to stop
  std::mutex m_mutex;                 // Mutex to protect shared resources
  std::thread m_position_thread;      // Thread for updating position
  bool m_wait = false;                // Whether there is need to suspend thread
  static unsigned int m_current_track;  // current track index
  static Gtk::ScrolledWindow *m_playlist_scrolled_window;

 private:
  /**
   * Checks for player supported features and changes PlayerWindow UI
   * corresponsing to features
   */
  void check_buttons_features();
  /**
   * Callback function which called after new player choosed
   * @param unsigned short - new player index
   */
  void on_player_choosed(unsigned short);
  /**
   * Callback function which called after new audio output device choosed
   * @param unsigned short - new device sink index
   */
  void on_device_choosed(unsigned short);
  /**
   * Flags to lock changing position and volume in UI
   * For example, if position thread updates position in UI position bar, then
   * we need to stop sending position_changed signal to DBus, because position
   * thread just got new pos and updated it
   */
  bool m_lock_pos_changing = false, m_lock_volume_changing = false;
  void update_position_thread();  // Thread function for updating position
  void pause_position_thread();   // Pause position thread function
  void resume_position_thread();  // Resume position thread function
  void stop_position_thread();    // Stop position thread function
#ifdef SUPPORT_AUDIO_OUTPUT
  gboolean on_signal_accept(
      const std::shared_ptr<Gdk::Drop>
          &);  // signal when file or folder hovered at window
  gboolean on_signal_drop(const Glib::ValueBase &value, double,
                          double);  // when file or folder dropped at window
  void on_signal_leave();           // when dropping canceled
  void add_directory_files_to_playlist(
      const std::string
          &directory_path);  // at files at directory recursively to playlist
  sigc::connection m_conn_accept;               // connection for signal accept
  sigc::connection m_conn_drop;                 // connection for signal drop
  sigc::connection m_conn_leave;                // connection for signal leave
  Glib::RefPtr<Gtk::DropTarget> m_drop_target;  // drop target pointer
#endif
};

#endif  // PLAYERWINDOW_H
