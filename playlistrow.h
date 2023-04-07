#ifndef PLAYLISTROW_H
#define PLAYLISTROW_H

#include <gtkmm/box.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/listboxrow.h>

class PlaylistRow : public Gtk::ListBoxRow {
public:
  const std::string m_filename; // file name for row item
  /**
   * Constructor with parameters
   * @param artist - song author (type: std::string)
   * @param title - song title (type: std::string)
   * @param duration - song duration in formated string (type: std::string)
   * @param filename - song file path (type: std::string)
   */
  PlaylistRow(const std::string &artist, const std::string &title,
              const std::string &duration, const std::string &filename);
  Gtk::Label *label_artist_title;          // label with artist and title
  void highlight();                        // highlight text with accent color
  void stop_highlight();                   // disable highlight
  const std::string &get_filename() const; // get file path
};

#endif // PLAYLISTROW_H
