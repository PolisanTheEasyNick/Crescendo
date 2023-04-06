#ifndef PLAYLISTROW_H
#define PLAYLISTROW_H

#include <gtkmm/box.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/listboxrow.h>

class PlaylistRow : public Gtk::ListBoxRow {
public:
  PlaylistRow(const std::string &author, const std::string &title,
              const std::string &duration, const std::string &filename);
};

#endif // PLAYLISTROW_H
