#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include "player.h"
#include "playlistrow.h"
#include "volumebutton.h"
#include <atomic>
#include <chrono>
#include <glib.h>
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/cssprovider.h>
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
    // m_mutex.unlock();
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

  void add_song_to_playlist(std::string filename);

protected:
  Player m_player;
  std::vector<std::tuple<std::string, std::string, std::string, std::string>>
      m_playlist; // one tuple is: filename - title - artist - length in string
                  // (0:00)
  void on_playpause_clicked(), on_prev_clicked(), on_next_clicked(),
      on_shuffle_clicked(), on_player_choose_clicked(),
      on_device_choose_clicked();
  Gtk::Grid m_main_grid;
  Gtk::Box m_control_buttons_box, m_volume_and_player_box;
  Gtk::Button m_playpause_button, m_prev_button, m_next_button,
      m_shuffle_button, m_player_choose_button, m_device_choose_button,
      m_add_song_to_playlist_button;
  Gtk::Label m_song_title_label, m_song_artist_label, m_current_pos_label,
      m_song_length_label;
  Gtk::Scale m_progress_bar_song_scale;
  VolumeButton m_volume_bar_scale_button;
  Gtk::Popover m_player_choose_popover, m_device_choose_popover;
  Gtk::ListBox m_song_title_list, m_playlist_listbox;
  Gtk::ScrolledWindow m_playlist_scrolled_window;
  std::atomic_bool stop_flag{false}; // Flag to signal thread to stop
  std::mutex m_mutex;                // Mutex to protect shared resources
  std::thread m_position_thread;     // Thread for updating position
  bool m_wait = false;               // Whether there is need to suspend thread

private:
  void check_buttons_features();
  void on_player_choosed(unsigned short);
  void on_device_choosed(unsigned short);
  bool m_lock_pos_changing = false, m_lock_volume_changing = false;
  void update_position_thread();
  void pause_position_thread();
  void resume_position_thread();
  void stop_position_thread();
};

#endif // PLAYERWINDOW_H
