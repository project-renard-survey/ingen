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

#ifndef INGEN_ENGINE_LV2PLUGIN_HPP
#define INGEN_ENGINE_LV2PLUGIN_HPP

#include <cstdlib>
#include <string>

#include <glibmm/module.h>
#include <boost/utility.hpp>

#include "lilv/lilv.h"
#include "raul/SharedPtr.hpp"

#include "PluginImpl.hpp"
#include "LV2Info.hpp"

namespace Ingen {
namespace Server {

class PatchImpl;
class NodeImpl;

/** Implementation of an LV2 plugin (loaded shared library).
 */
class LV2Plugin : public PluginImpl
{
public:
	LV2Plugin(SharedPtr<LV2Info> lv2_info, const std::string& uri);

	NodeImpl* instantiate(BufferFactory&     bufs,
	                      const std::string& name,
	                      bool               polyphonic,
	                      PatchImpl*         parent,
	                      Engine&            engine);

	const std::string symbol() const;

	SharedPtr<LV2Info> lv2_info() const { return _lv2_info; }

	const std::string& library_path() const;

	const LilvPlugin* lilv_plugin() const { return _lilv_plugin; }
	void              lilv_plugin(const LilvPlugin* p);

private:
	const LilvPlugin*  _lilv_plugin;
	SharedPtr<LV2Info> _lv2_info;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2PLUGIN_HPP

