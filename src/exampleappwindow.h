#ifndef GTKMM_EXAMPLEAPPWINDOW_H_
#define GTKMM_EXAMPLEAPPWINDOW_H_

#include <gtkmm.h>

class ExampleAppWindow : public Gtk::ApplicationWindow
{
public:
  ExampleAppWindow();

  void open_file_view(const Glib::RefPtr<Gio::File>& file);
};

#endif /* GTKMM_EXAMPLEAPPWINDOW_H */
