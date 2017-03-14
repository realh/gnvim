#ifndef GTKMM_EXAMPLEAPPLICATION_H
#define GTKMM_EXAMPLEAPPLICATION_H

#include <gtkmm.h>

class ExampleAppWindow;

class ExampleApplication: public Gtk::Application
{
public:
  ExampleApplication();

public:
  static Glib::RefPtr<ExampleApplication> create();

protected:
  // Override default signal handlers:
  void on_activate() override;
  void on_open(const Gio::Application::type_vec_files& files,
    const Glib::ustring& hint) override;

private:
  ExampleAppWindow* create_appwindow();
  void on_hide_window(Gtk::Window* window);
};

#endif /* GTKMM_EXAMPLEAPPLICATION_H */
