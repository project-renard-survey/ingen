/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_PATCH_BOX_HPP
#define INGEN_GUI_PATCH_BOX_HPP

#include <string>

#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/menushell.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/statusbar.h>

#include "raul/SharedPtr.hpp"

#include "Window.hpp"

namespace Raul {
class Atom;
class Path;
}

namespace Ingen {

namespace Client {
class PatchModel;
class PortModel;
class ObjectModel;
}

namespace GUI {

class BreadCrumbs;
class LoadPatchBox;
class LoadPluginWindow;
class NewSubpatchWindow;
class PatchDescriptionWindow;
class PatchView;
class PatchWindow;
class SubpatchModule;

/** A window for a patch.
 *
 * \ingroup GUI
 */
class PatchBox : public Gtk::VBox
{
public:
	PatchBox(BaseObjectType*                   cobject,
	         const Glib::RefPtr<Gtk::Builder>& xml);
	~PatchBox();

	static SharedPtr<PatchBox> create(
		App& app, SharedPtr<const Client::PatchModel> patch);

	void init_box(App& app);

	void set_patch(SharedPtr<const Client::PatchModel> pc,
	               SharedPtr<PatchView>                view);

	void set_window(PatchWindow* win) { _window = win; }

	void show_documentation(const std::string& doc, bool html);
	void hide_documentation();

	SharedPtr<const Client::PatchModel> patch() const { return _patch; }
	SharedPtr<PatchView>                view()  const { return _view; }

	void show_port_status(const Client::PortModel* model,
	                      const Raul::Atom&        value);

	void set_patch_from_path(const Raul::Path& path, SharedPtr<PatchView> view);

	void object_entered(const Client::ObjectModel* model);
	void object_left(const Client::ObjectModel* model);

private:
	void patch_port_added(SharedPtr<const Client::PortModel> port);
	void patch_port_removed(SharedPtr<const Client::PortModel> port);
	void show_status(const Client::ObjectModel* model);

	int message_dialog(const Glib::ustring& message,
	                   const Glib::ustring& secondary_text,
	                   Gtk::MessageType     type,
	                   Gtk::ButtonsType     buttons);

	void event_import();
	void event_save();
	void event_save_as();
	void event_draw();
	void event_copy();
	void event_paste();
	void event_delete();
	void event_select_all();
	void event_close();
	void event_quit();
	void event_fullscreen_toggled();
	void event_status_bar_toggled();
	void event_human_names_toggled();
	void event_port_names_toggled();
	void event_zoom_in();
	void event_zoom_out();
	void event_zoom_normal();
	void event_arrange();
	void event_show_properties();
	void event_show_engine();
	void event_clipboard_changed(GdkEventOwnerChange* ev);

	App*                                _app;
	SharedPtr<const Client::PatchModel> _patch;
	SharedPtr<PatchView>                _view;
	PatchWindow*                        _window;

	sigc::connection new_port_connection;
	sigc::connection removed_port_connection;
	sigc::connection edit_mode_connection;

	Gtk::MenuItem*      _menu_import;
	Gtk::MenuItem*      _menu_save;
	Gtk::MenuItem*      _menu_save_as;
	Gtk::MenuItem*      _menu_draw;
	Gtk::MenuItem*      _menu_cut;
	Gtk::MenuItem*      _menu_copy;
	Gtk::MenuItem*      _menu_paste;
	Gtk::MenuItem*      _menu_delete;
	Gtk::MenuItem*      _menu_select_all;
	Gtk::MenuItem*      _menu_close;
	Gtk::MenuItem*      _menu_quit;
	Gtk::CheckMenuItem* _menu_human_names;
	Gtk::CheckMenuItem* _menu_show_port_names;
	Gtk::CheckMenuItem* _menu_show_status_bar;
	Gtk::MenuItem*      _menu_zoom_in;
	Gtk::MenuItem*      _menu_zoom_out;
	Gtk::MenuItem*      _menu_zoom_normal;
	Gtk::MenuItem*      _menu_fullscreen;
	Gtk::MenuItem*      _menu_arrange;
	Gtk::MenuItem*      _menu_view_engine_window;
	Gtk::MenuItem*      _menu_view_control_window;
	Gtk::MenuItem*      _menu_view_patch_properties;
	Gtk::MenuItem*      _menu_view_messages_window;
	Gtk::MenuItem*      _menu_view_patch_tree_window;
	Gtk::MenuItem*      _menu_help_about;

	Gtk::VBox*          _vbox;
	Gtk::Alignment*     _alignment;
	BreadCrumbs*        _breadcrumbs;
	Gtk::Statusbar*     _status_bar;

	Gtk::HPaned*         _doc_paned;
	Gtk::ScrolledWindow* _doc_scrolledwindow;

	sigc::connection _entered_connection;
	sigc::connection _left_connection;

	/** Invisible bin used to store breadcrumbs when not shown by a view */
	Gtk::Alignment _breadcrumb_bin;

	bool _has_shown_documentation;
	bool _enable_signal;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PATCH_BOX_HPP
