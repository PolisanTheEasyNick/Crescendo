#include "volumebutton.h"

VolumeButton::VolumeButton() : Gtk::ScaleButton(0, 1, 0.1) {
  set_value(0.5);
  auto adjustment = Gtk::Adjustment::create(0.5, 0, 1, 0.1, 0.1);
  set_adjustment(adjustment);
  set_icons({"audio-volume-muted-panel", "audio-volume-high-panel",
             "audio-volume-low-panel", "audio-volume-medium-panel",
             "audio-volume-high-panel"});

  get_first_child()->get_style_context()->remove_class("flat");
}
