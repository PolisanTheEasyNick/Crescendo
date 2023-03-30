#ifndef VOLUMEBUTTON_H
#define VOLUMEBUTTON_H

#include <gtkmm/cssprovider.h>
#include <gtkmm/scalebutton.h>
#include <gtkmm/settings.h>
#include <gtkmm/stylecontext.h>
#include <iostream>

class VolumeButton : public Gtk::ScaleButton {
public:
  VolumeButton();
  virtual ~VolumeButton() {}
};
#endif // VOLUMEBUTTON_H
