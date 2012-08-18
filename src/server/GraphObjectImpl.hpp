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

#ifndef INGEN_ENGINE_GRAPHOBJECTIMPL_HPP
#define INGEN_ENGINE_GRAPHOBJECTIMPL_HPP

#include <cassert>
#include <cstddef>
#include <map>

#include "ingen/GraphObject.hpp"
#include "ingen/Resource.hpp"
#include "raul/Deletable.hpp"
#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"

namespace Raul { class Maid; }

namespace Ingen {

namespace Shared { class URIs; }

namespace Server {

class PatchImpl;
class Context;
class ProcessContext;
class BufferFactory;

/** An object on the audio graph (a Patch, Block, or Port).
 *
 * Each of these is a Raul::Deletable and so can be deleted in a realtime safe
 * way from anywhere, and they all have a map of variable for clients to store
 * arbitrary values in (which the engine puts no significance to whatsoever).
 *
 * \ingroup engine
 */
class GraphObjectImpl : public GraphObject
{
public:
	virtual ~GraphObjectImpl() {}

	const Raul::Symbol& symbol() const { return _symbol; }

	GraphObject*     graph_parent() const { return _parent; }
	GraphObjectImpl* parent()       const { return _parent; }

	/** Rename */
	virtual void set_path(const Raul::Path& new_path) {
		_path = new_path;
		const char* const new_sym = new_path.symbol();
		if (new_sym[0] != '\0') {
			_symbol = Raul::Symbol(new_sym);
		}
		set_uri(GraphObject::path_to_uri(new_path));
	}

	const Raul::Atom& get_property(const Raul::URI& key) const;

	/** The Patch this object is a child of. */
	virtual PatchImpl* parent_patch() const;

	const Raul::Path& path() const { return _path; }

	/** Prepare for a new (external) polyphony value.
	 *
	 * Preprocessor thread, poly is actually applied by apply_poly.
	 * \return true on success.
	 */
	virtual bool prepare_poly(BufferFactory& bufs, uint32_t poly) = 0;

	/** Apply a new (external) polyphony value.
	 *
	 * Audio thread.
	 *
	 * \param poly Must be <= the most recent value passed to prepare_poly.
	 * \param maid Any objects no longer needed will be pushed to this
	 */
	virtual bool apply_poly(
		ProcessContext& context, Raul::Maid& maid, uint32_t poly) = 0;

protected:
	GraphObjectImpl(Ingen::URIs&        uris,
	                GraphObjectImpl*    parent,
	                const Raul::Symbol& symbol);

	GraphObjectImpl* _parent;
	Raul::Path       _path;
	Raul::Symbol     _symbol;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_GRAPHOBJECTIMPL_HPP
