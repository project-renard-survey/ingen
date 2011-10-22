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

#ifndef INGEN_GUI_CONFIGURATION_HPP
#define INGEN_GUI_CONFIGURATION_HPP

#include <stdint.h>

#include <string>

#include "raul/SharedPtr.hpp"

namespace Ingen { namespace Client { class PortModel; } }
using Ingen::Client::PortModel;
using std::string;

namespace Ingen {
namespace GUI {

class App;
class Port;

/** Singleton state manager for the entire app.
 *
 * Stores settings like color preferences, search paths, etc.
 * (ie any user-defined preferences to be stoed in the rc file).
 *
 * \ingroup GUI
 */
class Configuration
{
public:
	Configuration(App& app);
	~Configuration();

	void load_settings(string filename = "");
	void save_settings(string filename = "");

	void apply_settings();

	const string& patch_folder()                    { return _patch_folder; }
	void          set_patch_folder(const string& f) { _patch_folder = f; }

	uint32_t get_port_color(const PortModel* p);

	enum NameStyle { PATH, HUMAN, NONE };

	NameStyle name_style() const          { return _name_style; }
	void      set_name_style(NameStyle s) { _name_style = s; }

private:
	App& _app;

	/** Most recent patch folder shown in open dialog */
	string _patch_folder;

	NameStyle _name_style;

	uint32_t _audio_port_color;
	uint32_t _control_port_color;
	uint32_t _event_port_color;
	uint32_t _string_port_color;
	uint32_t _value_port_color;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_CONFIGURATION_HPP

