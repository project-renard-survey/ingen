/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_CONTROLBINDINGS_HPP
#define INGEN_ENGINE_CONTROLBINDINGS_HPP

#include <stdint.h>
#include <map>
#include "raul/SharedPtr.hpp"
#include "raul/Path.hpp"
#include "shared/LV2URIMap.hpp"

namespace Ingen {

class Engine;
class ProcessContext;
class EventBuffer;
class PortImpl;

class ControlBindings {
public:
	typedef std::map<int8_t, PortImpl*> Bindings;

	ControlBindings(Engine& engine, SharedPtr<Shared::LV2URIMap> map)
		: _engine(engine)
		, _map(map)
		, _learn_port(NULL)
		, _bindings(new Bindings())
	{}

	void learn(PortImpl* port);
	void process(ProcessContext& context, EventBuffer* buffer);

	/** Remove all bindings for @a path or children of @a path.
	 * The caller must safely drop the returned reference in the
	 * post-processing thread after at least one process thread has run.
	 */
	SharedPtr<Bindings> remove(const Raul::Path& path);

private:
	Engine&                      _engine;
	SharedPtr<Shared::LV2URIMap> _map;
	PortImpl*                    _learn_port;

	void set_port_value(ProcessContext& context, PortImpl* port, int8_t cc_value);
	void bind(ProcessContext& context, int8_t cc_num);

	SharedPtr<Bindings> _bindings;
};

} // namespace Ingen

#endif // INGEN_ENGINE_CONTROLBINDINGS_HPP
