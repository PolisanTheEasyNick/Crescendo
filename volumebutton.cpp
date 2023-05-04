#include "volumebutton.h"

VolumeButton::VolumeButton() : Gtk::ScaleButton(0, 1, 0.1) {
  set_value(0.5);
  auto adjustment = Gtk::Adjustment::create(0.5, 0, 1, 0.1, 0.1);
  set_adjustment(adjustment);
  set_icons({"audio-volume-muted", "audio-volume-high", "audio-volume-low",
             "audio-volume-medium", "audio-volume-high"});

  get_first_child()->get_style_context()->remove_class("flat");

  // Since GTK4 have bug https://gitlab.gnome.org/GNOME/gtk/-/issues/4939
  // We will create our own signal for button release in created by stock
  // GestureClick controller of Gtk::Scale
  auto children = observe_children(); // get all children of ScaleButton
  GListModel *children_obj = children->gobj();
  int n_children = g_list_model_get_n_items(children_obj);
  for (int i = 0; i < n_children; i++) {
    GObject *child_gobj = (GObject *)g_list_model_get_item(children_obj, i);
    auto child = Glib::wrap(child_gobj, false);
    auto gtk_popover = dynamic_cast<Gtk::Popover *>(child.get());
    if (gtk_popover) { // find popover
      auto children_popover =
          gtk_popover->get_first_child()->get_first_child()->observe_children();
      GListModel *popover_children_obj = children_popover->gobj();
      int n_children_popover =
          g_list_model_get_n_items(children_obj); // get his children
      for (int j = 0; j < n_children_popover; j++) {
        GObject *child_popover_gobj =
            (GObject *)g_list_model_get_item(popover_children_obj, j);
        auto child_gtk_box = Glib::wrap(child_popover_gobj, false);
        auto gtk_scale = dynamic_cast<Gtk::Scale *>(child_gtk_box.get());
        if (gtk_scale) {                                       // find gtk scale
          auto controllers = gtk_scale->observe_controllers(); // get
                                                               // controllers
          GListModel *model = controllers->gobj();
          int n_controllers = g_list_model_get_n_items(model);
          for (int i = 0; i < n_controllers; i++) {
            g_list_model_get_item(model, i);
            GObject *controller_gobj =
                (GObject *)g_list_model_get_item(model, i);
            auto click_controller = Glib::wrap(controller_gobj, false);
            auto gesture_click =
                dynamic_cast<Gtk::GestureClick *>(click_controller.get());
            auto drag_controller =
                dynamic_cast<Gtk::GestureDrag *>(click_controller.get());
            if (gesture_click) { // find gesture click
              gesture_click->set_button(0);
              gesture_click->signal_released().connect(
                  [this, gtk_scale](int, double, double) {
                    // redraw on release
                    g_idle_add(
                        [](gpointer data) -> gboolean {
                          Gtk::Scale *gtk_scale =
                              static_cast<Gtk::Scale *>(data);
                          gtk_scale->queue_draw();
                          return false;
                        },
                        gtk_scale);
                  });
            } else if (drag_controller) { // and find drag controller
              drag_controller->set_button(0);
              drag_controller->signal_drag_update().connect(
                  [this, gtk_scale](double, double) {
                    // and redraw on dragging
                    // redraw on release
                    g_idle_add(
                        [](gpointer data) -> gboolean {
                          Gtk::Scale *gtk_scale =
                              static_cast<Gtk::Scale *>(data);
                          gtk_scale->queue_draw();
                          return false;
                        },
                        gtk_scale);
                  });
            }
          }
        }
      }
    }
  }
}
