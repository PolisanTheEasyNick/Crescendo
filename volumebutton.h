#ifndef VOLUMEBUTTON_H
#define VOLUMEBUTTON_H

#include "gtkmm/scalebutton.h"
#include <iostream>

class VolumeButton : public Gtk::ScaleButton {
public:
  VolumeButton();
  virtual ~VolumeButton() {}
};
#endif // VOLUMEBUTTON_H