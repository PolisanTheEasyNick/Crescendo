#include "volumebutton.h"

VolumeButton::VolumeButton() : Gtk::ScaleButton(0, 1, 0.01) {
  set_value(50);
  set_icons({"audio-volume-muted-symbolic", "audio-volume-low-symbolic",
             "audio-volume-medium-symbolic", "audio-volume-high-symbolic"});
}
