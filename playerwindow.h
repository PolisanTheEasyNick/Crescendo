#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include "player.h"
#include "playlistrow.h"
#include "volumebutton.h"
#include <atomic>
#include <chrono>
#include <gio/gfile.h>
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
#include <mutex>
#include <random>
#include <sigc++/signal.h>
#include <thread>
#include <tuple>

class PlayerWindow : public Gtk::ApplicationWindow, public PlayerObserver {
public:
  PlayerWindow();
  PlayerWindow(int argc, char *argv[]);
  virtual ~PlayerWindow() {
    // Stop the position thread
    stop_position_thread();
    delete m_playlist_scrolled_window;
  }
  void on_song_title_changed(const std::string &new_song) override {
    std::cout << "New song name PlayerWindow: " << new_song << std::endl;
    m_song_title_label.set_label(new_song);
  }
  void on_song_artist_changed(const std::string &new_song_author) override {
    std::cout << "New song author PlayerWindow: " << new_song_author
              << std::endl;
    m_song_artist_label.set_label(new_song_author);
  }
  void on_song_length_changed(const std::string &new_song_length) override {
    std::cout << "New song length PlayerWindow: " << new_song_length
              << std::endl;
    m_song_length_label.set_label(new_song_length);
  }
  void on_is_shuffle_changed(const bool &new_is_shuffle) override {
    std::cout << "New song shuffle PlayerWindow: " << new_is_shuffle
              << std::endl;
    if (new_is_shuffle) {
      m_shuffle_button.get_style_context()->add_class("shuffle-enabled");
    } else {
      m_shuffle_button.get_style_context()->remove_class("shuffle-enabled");
    }
  }
  void on_is_playing_changed(const bool &new_is_playing) override {
    std::cout << "New song isplaying PlayerWindow: " << new_is_playing
              << std::endl;
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
  void on_song_volume_changed(const double &new_song_volume) override {
    std::cout << "New song volume PlayerWindow: " << new_song_volume
              << std::endl;
    m_lock_volume_changing = true;
    m_volume_bar_scale_button.set_value(new_song_volume);
    m_lock_volume_changing = false;
  }

  void on_song_position_changed(const uint64_t &new_song_pos) override {
    std::cout << "New song pos: " << new_song_pos << std::endl;
    m_current_pos_label.set_label(
        Helper::get_instance().format_time(new_song_pos));
    double pos = m_player.get_position();
    double len = m_player.get_song_length();
    double new_pos = pos / len;
    m_lock_pos_changing = true;
    m_progress_bar_song_scale.set_value(new_pos);
    m_lock_pos_changing = false;
  }

  void on_loop_status_changed(const int &new_loop_status) override {
    std::cout << "New loop status: " << new_loop_status << std::endl;
    if (new_loop_status == -1 || new_loop_status == 0) { // none
      m_loop_button.set_icon_name("media-repeat-none");
    } else if (new_loop_status == 1) { // playlist
      m_loop_button.set_icon_name("media-repeat-all");
    } else if (new_loop_status == 2) { // song
      m_loop_button.set_icon_name("media-repeat-single");
    } else { // none
      m_loop_button.set_icon_name("media-repeat-none");
    }
  }

#ifdef SUPPORT_AUDIO_OUTPUT
  void add_song_to_playlist(const std::string &filename);
  static void on_music_ends();
#endif

protected:
  static Player m_player;
  void on_playpause_clicked(), on_prev_clicked(), on_next_clicked(),
      on_shuffle_clicked(), on_player_choose_clicked(),
      on_device_choose_clicked(), on_loop_clicked();
  Gtk::Grid m_main_grid;
  Gtk::Box m_control_buttons_box, m_volume_and_player_box;
  Gtk::Button m_playpause_button, m_prev_button, m_next_button,
      m_shuffle_button, m_player_choose_button, m_device_choose_button,
      m_add_song_to_playlist_button, m_loop_button;
  Gtk::Label m_song_title_label, m_song_artist_label, m_current_pos_label,
      m_song_length_label;
  Gtk::Scale m_progress_bar_song_scale;
  VolumeButton m_volume_bar_scale_button;
  Gtk::Popover m_player_choose_popover, m_device_choose_popover;
  Gtk::ListBox m_song_title_list, m_playlist_listbox;
  static PlaylistRow *m_activated_row;
  std::atomic_bool stop_flag{false}; // Flag to signal thread to stop
  std::mutex m_mutex;                // Mutex to protect shared resources
  std::thread m_position_thread;     // Thread for updating position
  bool m_wait = false;               // Whether there is need to suspend thread
  static unsigned int m_current_track;
  static Gtk::ScrolledWindow *m_playlist_scrolled_window;

private:
  void check_buttons_features();
  void on_player_choosed(unsigned short);
  void on_device_choosed(unsigned short);
  bool m_lock_pos_changing = false, m_lock_volume_changing = false;
  void update_position_thread();
  void pause_position_thread();
  void resume_position_thread();
  void stop_position_thread();
#ifdef SUPPORT_AUDIO_OUTPUT
  gboolean on_signal_accept(const std::shared_ptr<Gdk::Drop> &);
  gboolean on_signal_drop(const Glib::ValueBase &value, double, double);
  void add_directory_files_to_playlist(const std::string &directory_path);
  sigc::connection m_conn_accept;
  sigc::connection m_conn_drop;
  Glib::RefPtr<Gtk::DropTarget> m_drop_target;
#endif
};

#endif // PLAYERWINDOW_H
