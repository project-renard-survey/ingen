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

#include "App.hpp"
#include "ingen/ServerInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/PatchModel.hpp"
#include "client/ClientStore.hpp"
#include "NewSubpatchWindow.hpp"
#include "PatchView.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {

NewSubpatchWindow::NewSubpatchWindow(BaseObjectType*                   cobject,
                                     const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget("new_subpatch_name_entry", _name_entry);
	xml->get_widget("new_subpatch_message_label", _message_label);
	xml->get_widget("new_subpatch_polyphony_spinbutton", _poly_spinbutton);
	xml->get_widget("new_subpatch_ok_button", _ok_button);
	xml->get_widget("new_subpatch_cancel_button", _cancel_button);

	_name_entry->signal_changed().connect(sigc::mem_fun(this, &NewSubpatchWindow::name_changed));
	_ok_button->signal_clicked().connect(sigc::mem_fun(this, &NewSubpatchWindow::ok_clicked));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &NewSubpatchWindow::cancel_clicked));

	_ok_button->property_sensitive() = false;
}

void
NewSubpatchWindow::present(SharedPtr<const PatchModel> patch,
                           GraphObject::Properties     data)
{
	set_patch(patch);
	_initial_data = data;
	Gtk::Window::present();
}

/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
NewSubpatchWindow::set_patch(SharedPtr<const PatchModel> patch)
{
	_patch = patch;
}

/** Called every time the user types into the name input box.
 * Used to display warning messages, and enable/disable the OK button.
 */
void
NewSubpatchWindow::name_changed()
{
	string name = _name_entry->get_text();
	if (!Path::is_valid_name(name)) {
		_message_label->set_text("Name contains invalid characters.");
		_ok_button->property_sensitive() = false;
	} else if (App::instance().store()->find(_patch->path().base() + name)
			!= App::instance().store()->end()) {
		_message_label->set_text("An object already exists with that name.");
		_ok_button->property_sensitive() = false;
	} else if (name.length() == 0) {
		_message_label->set_text("");
		_ok_button->property_sensitive() = false;
	} else {
		_message_label->set_text("");
		_ok_button->property_sensitive() = true;
	}
}

void
NewSubpatchWindow::ok_clicked()
{
	App&           app  = App::instance();
	const Path     path = _patch->path().base() + Path::nameify(_name_entry->get_text());
	const uint32_t poly = _poly_spinbutton->get_value_as_int();

	// Create patch
	Resource::Properties props;
	props.insert(make_pair(app.uris().rdf_type,        app.uris().ingen_Patch));
	props.insert(make_pair(app.uris().ingen_polyphony, Atom(int32_t(poly))));
	props.insert(make_pair(app.uris().ingen_enabled,   Atom(bool(true))));
	app.engine()->put(path, props, Resource::INTERNAL);

	// Set external (node perspective) properties
	props = _initial_data;
	props.insert(make_pair(app.uris().rdf_type, app.uris().ingen_Patch));
	app.engine()->put(path, _initial_data, Resource::EXTERNAL);

	hide();
}

void
NewSubpatchWindow::cancel_clicked()
{
	hide();
}

} // namespace GUI
} // namespace Ingen
