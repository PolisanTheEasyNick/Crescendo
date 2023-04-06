#include "playlistrow.h"

PlaylistRow::PlaylistRow(const std::string &author, const std::string &title,
                         const std::string &duration,
                         const std::string &filename) {
  // Create a grid with 3 columns
  Gtk::Grid *grid = Gtk::make_managed<Gtk::Grid>();
  grid->set_column_spacing(10);
  grid->set_halign(Gtk::Align::FILL);
  grid->set_valign(Gtk::Align::FILL);
  grid->set_hexpand();
  Gtk::Label *label_author_title = Gtk::make_managed<Gtk::Label>();
  if (author != "" && title != "") {
    label_author_title->set_label(author + " - " + title);
  } else if (author == "" && title != "") {
    label_author_title->set_label(title);
  } else if (author != "" && title == "") {
    label_author_title->set_label(author);
  } else {
    label_author_title->set_label(filename);
  }

  label_author_title->set_halign(Gtk::Align::START);
  label_author_title->set_valign(Gtk::Align::CENTER);
  label_author_title->set_hexpand();
  grid->attach(*label_author_title, 0, 0);
  // Add the song duration label to the third column, aligned to the right
  Gtk::Label *label_duration = Gtk::make_managed<Gtk::Label>(duration);
  label_duration->set_halign(Gtk::Align::END);
  label_duration->set_valign(Gtk::Align::CENTER);
  label_duration->set_hexpand();
  grid->attach(*label_duration, 2, 0, 1, 1);
  // Add the grid to the playlist row
  set_halign(Gtk::Align::FILL);
  set_hexpand();
  set_margin_end(20);
  set_child(*grid);
}
