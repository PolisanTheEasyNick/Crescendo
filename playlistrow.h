#ifndef PLAYLISTROW_H
#define PLAYLISTROW_H

#include <gtkmm/box.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/listboxrow.h>

class PlaylistRow : public Gtk::ListBoxRow {
public:
  // Define a signal for double click
  sigc::signal<void, PlaylistRow *> signal_row_double_clicked();
  const std::string m_filename;
  PlaylistRow(const std::string &author, const std::string &title,
              const std::string &duration, const std::string &filename);
  Gtk::Label *label_author_title;
  void highlight();
  void stop_highlight();
  const std::string &get_filename() const;
};

#endif // PLAYLISTROW_H
