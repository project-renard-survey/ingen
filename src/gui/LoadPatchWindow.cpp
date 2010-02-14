/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <sys/types.h>
#include <dirent.h>
#include <boost/optional/optional.hpp>
#include "interface/EngineInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "shared/runtime_paths.hpp"
#include "client/PatchModel.hpp"
#include "App.hpp"
#include "Configuration.hpp"
#include "LoadPatchWindow.hpp"
#include "ThreadedLoader.hpp"

using namespace Ingen::Serialisation;
using boost::optional;
using namespace std;
using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {


LoadPatchWindow::LoadPatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::FileChooserDialog(cobject)
	, _merge_ports(true)
{
	xml->get_widget("load_patch_poly_voices_radio", _poly_voices_radio);
	xml->get_widget("load_patch_poly_from_file_radio", _poly_from_file_radio);
	xml->get_widget("load_patch_poly_spinbutton", _poly_spinbutton);
	xml->get_widget("load_patch_merge_ports_radio", _merge_ports_radio);
	xml->get_widget("load_patch_insert_ports_radio", _insert_ports_radio);
	xml->get_widget("load_patch_ok_button", _ok_button);
	xml->get_widget("load_patch_cancel_button", _cancel_button);

	_poly_from_file_radio->signal_toggled().connect(
			sigc::mem_fun(this, &LoadPatchWindow::poly_from_file_selected));
	_poly_voices_radio->signal_toggled().connect(
			sigc::mem_fun(this, &LoadPatchWindow::poly_voices_selected));
	_merge_ports_radio->signal_toggled().connect(
			sigc::mem_fun(this, &LoadPatchWindow::merge_ports_selected));
	_insert_ports_radio->signal_toggled().connect(
			sigc::mem_fun(this, &LoadPatchWindow::insert_ports_selected));
	_ok_button->signal_clicked().connect(
			sigc::mem_fun(this, &LoadPatchWindow::ok_clicked));
	_cancel_button->signal_clicked().connect(
			sigc::mem_fun(this, &LoadPatchWindow::cancel_clicked));

	_poly_voices_radio->set_active(true);

	Gtk::FileFilter filt;
	filt.add_pattern("*.om");
	filt.set_name("Om patch files (XML, DEPRECATED) (*.om)");
	filt.add_pattern("*.ingen.lv2");
	filt.set_name("Ingen bundles (*.ingen.lv2)");
	filt.add_pattern("*.ingen.ttl");
	filt.set_name("Ingen patch files (*.ingen.ttl)");
	set_filter(filt);

	// Add global examples directory to "shortcut folders" (bookmarks)
	const string examples_dir = Shared::data_file_path("patches");
	DIR* d = opendir(examples_dir.c_str());
	if (d != NULL) {
		add_shortcut_folder(examples_dir);
		closedir(d);
	}
}


void
LoadPatchWindow::present(SharedPtr<PatchModel> patch, GraphObject::Properties data)
{
	set_patch(patch);
	_initial_data = data;
	Gtk::Window::present();
}


/** Sets the patch model for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadPatchWindow::set_patch(SharedPtr<PatchModel> patch)
{
	_patch = patch;
	_poly_spinbutton->set_value(patch->poly());
}


void
LoadPatchWindow::on_show()
{
	if (App::instance().configuration()->patch_folder().length() > 0)
		set_current_folder(App::instance().configuration()->patch_folder());
	Gtk::FileChooserDialog::on_show();
}


///// Event Handlers //////


void
LoadPatchWindow::poly_from_file_selected()
{
	_poly_spinbutton->property_sensitive() = false;
}


void
LoadPatchWindow::poly_voices_selected()
{
	_poly_spinbutton->property_sensitive() = true;
}

void
LoadPatchWindow::merge_ports_selected()
{
	_merge_ports = true;
}


void
LoadPatchWindow::insert_ports_selected()
{
	_merge_ports = false;
}


void
LoadPatchWindow::ok_clicked()
{
	if (!_patch)
		return;

	// If unset load_patch will load value
	optional<Path>   parent;
	optional<Symbol> symbol;

	if (_poly_voices_radio->get_active())
		_initial_data.insert(make_pair(
				App::instance().uris().ingen_polyphony,
				_poly_spinbutton->get_value_as_int()));

	if (!_patch->path().is_root()) {
		parent = _patch->path().parent();
		symbol = _patch->symbol();
	}

	cout << "MERGE PORTS: " << _merge_ports << endl;

	_patch.reset();
	hide();

	App::instance().loader()->load_patch(true, get_uri(), Path("/"),
			parent, symbol, _initial_data);
}


void
LoadPatchWindow::cancel_clicked()
{
	hide();
}


} // namespace GUI
} // namespace Ingen
