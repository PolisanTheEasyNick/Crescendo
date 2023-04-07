#include "playlistrow.h"

const std::string &PlaylistRow::get_filename() const { return m_filename; }

PlaylistRow::PlaylistRow(const std::string &artist, const std::string &title,
                         const std::string &duration,
                         const std::string &filename)
    : m_filename(filename) {
  // Create a grid with 3 columns
  Gtk::Grid *grid = Gtk::make_managed<Gtk::Grid>();
  grid->set_column_spacing(10);
  grid->set_halign(Gtk::Align::FILL);
  grid->set_valign(Gtk::Align::FILL);
  grid->set_hexpand();
  label_artist_title = Gtk::make_managed<Gtk::Label>();
  if (artist != "" && title != "") { // if there is artist and title
    label_artist_title->set_label(artist + " - " + title); // write it
  } else if (artist == "" && title != "") {  // if only title accessible
    label_artist_title->set_label(title);    // write only title
  } else if (artist != "" && title == "") {  // if only artist
    label_artist_title->set_label(artist);   // write artist
  } else {                                   // if nothing
    label_artist_title->set_label(filename); // write file path
  }

  label_artist_title->set_halign(Gtk::Align::START);
  label_artist_title->set_valign(Gtk::Align::CENTER);
  label_artist_title->set_hexpand();
  label_artist_title->set_can_focus(false);
  label_artist_title->set_ellipsize(
      Pango::EllipsizeMode::END); // set ability to cut string and add "..." if
                                  // label too long
  // Define a CSS style for the class
  auto css_provider = Gtk::CssProvider::create();
  css_provider->load_from_data(
      ".highlight { color: @theme_selected_bg_color; }"); // add class for
                                                          // highlighting

  // Apply the CSS style to the button
  auto context = label_artist_title->get_style_context();
  context->add_provider(
      css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER); // add css provider with
                                                       // out .highlight class
  grid->attach(*label_artist_title, 0, 0);
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
  set_selectable(false);
  set_can_focus(false);
  set_child(*grid);
}

void PlaylistRow::highlight() {
  label_artist_title->add_css_class("highlight");
}

void PlaylistRow::stop_highlight() {
  label_artist_title->remove_css_class("highlight");
}
