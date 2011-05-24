/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_GUI_CONNECTWINDOW_HPP
#define INGEN_GUI_CONNECTWINDOW_HPP

#include <gtkmm.h>

#include "lilv/lilv.h"
#include "raul/SharedPtr.hpp"

#include "Window.hpp"

namespace Ingen {
namespace GUI {

class App;

/** The initially visible "Connect to engine" window.
 *
 * This handles actually connecting to the engine and making sure everything
 * is ready before really launching the app (eg wait for the root patch).
 *
 * \ingroup GUI
 */
class ConnectWindow : public Dialog
{
public:
	ConnectWindow(BaseObjectType*                   cobject,
	              const Glib::RefPtr<Gtk::Builder>& xml);

	void set_connected_to(SharedPtr<ServerInterface> engine);
	void start(Ingen::Shared::World* world);
	void on_response(int32_t id) { _attached = true; }

	bool attached()  const { return _finished_connecting; }
	bool quit_flag() const { return _quit_flag; }

private:
	enum Mode { CONNECT_REMOTE, LAUNCH_REMOTE, INTERNAL };

	void server_toggled();
	void launch_toggled();
	void internal_toggled();

	void disconnect();
	void connect(bool existing);
	void activate();
	void deactivate();
	void quit_clicked();
	void on_show();
	void on_hide();

	void load_widgets();
	void set_connecting_widget_states();

	bool gtk_callback();
	void quit();

	const Glib::RefPtr<Gtk::Builder> _xml;

	Mode    _mode;
	int32_t _ping_id;
	bool    _attached;
	bool    _finished_connecting;
	bool    _widgets_loaded;
	int     _connect_stage;
	bool    _quit_flag;

	Gtk::Image*        _icon;
	Gtk::ProgressBar*  _progress_bar;
	Gtk::Label*        _progress_label;
	Gtk::Entry*        _url_entry;
	Gtk::RadioButton*  _server_radio;
	Gtk::SpinButton*   _port_spinbutton;
	Gtk::RadioButton*  _launch_radio;
	Gtk::RadioButton*  _internal_radio;
	Gtk::Button*       _activate_button;
	Gtk::Button*       _deactivate_button;
	Gtk::Button*       _disconnect_button;
	Gtk::Button*       _connect_button;
	Gtk::Button*       _quit_button;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_CONNECTWINDOW_HPP
