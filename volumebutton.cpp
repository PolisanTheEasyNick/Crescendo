#include "volumebutton.h"

VolumeButton::VolumeButton() : Gtk::ScaleButton(0, 1, 0.1) {
  set_value(0.5);
  auto adjustment = Gtk::Adjustment::create(0.5, 0, 1, 0.1, 0.1);
  set_adjustment(adjustment);
  set_icons({"audio-volume-muted", "audio-volume-high", "audio-volume-low",
             "audio-volume-medium", "audio-volume-high"});

  get_first_child()->get_style_context()->remove_class("flat");
  auto children = observe_children();
  GListModel *children_obj = children->gobj();
  int n_children = g_list_model_get_n_items(children_obj);
  for (int i = 0; i < n_children; i++) {
    GObject *child_gobj = (GObject *)g_list_model_get_item(children_obj, i);
    auto child = Glib::wrap(child_gobj, false);
    auto gtk_popover = dynamic_cast<Gtk::Popover *>(child.get());
    if (gtk_popover) {
      auto children_popover =
          gtk_popover->get_first_child()->get_first_child()->observe_children();
      GListModel *popover_children_obj = children_popover->gobj();
      int n_children_popover = g_list_model_get_n_items(children_obj);
      for (int j = 0; j < n_children_popover; j++) {
        GObject *child_popover_gobj =
            (GObject *)g_list_model_get_item(popover_children_obj, j);
        auto child_gtk_box = Glib::wrap(child_popover_gobj, false);
        auto gtk_scale = dynamic_cast<Gtk::Scale *>(child_gtk_box.get());
        if (gtk_scale) {
          auto controllers = gtk_scale->observe_controllers();
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
            if (gesture_click) {
              gesture_click->set_button(0);
              gesture_click->signal_released().connect(
                  [this, gtk_scale](int, double, double) {
                    gtk_scale->queue_draw();
                  });
            } else if (drag_controller) {
              drag_controller->set_button(0);
              drag_controller->signal_drag_update().connect(
                  [this, gtk_scale](double, double) {
                    gtk_scale->queue_draw();
                  });
            }
          }
        }
      }
    }
  }
  //  GListModel *model = controllers->gobj();
  //  int n_controllers = g_list_model_get_n_items(model);
  //  for (int i = 0; i < n_controllers; i++) {
  //    std::cout << "Controller " << std::endl;

  //    GObject *controller_gobj = (GObject *)g_list_model_get_item(model, i);
  //    auto click_controller = Glib::wrap(controller_gobj, false);
  //    auto gesture_click = dynamic_cast<Gtk::GestureClick *>(
  //        click_controller
  //            .get()); // cast the Glib::Object to a Gtk::GestureClick*
  //    if (gesture_click) {
  //      std::cout << "gesture click" << std::endl;
  //      gesture_click->set_button(0);
  //      gesture_click->signal_released().connect([this](int, double, double)
  //      {
  //        std::cout << "Volume released" << std::endl;
  //      });
  //    }
  //  }
}
