/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef COMPILED_PATCH_HPP
#define COMPILED_PATCH_HPP

#include <iostream>
#include <vector>
#include <raul/List.hpp>
#include <raul/Deletable.hpp>
#include <boost/utility.hpp>

using Raul::List;

using namespace std;

namespace Ingen {


/** All information required about a node to execute it in an audio thread.
 */
struct CompiledNode {
	CompiledNode(NodeImpl* n, size_t np, List<NodeImpl*>* d)
		: _node(n), _n_providers(np)
	{
		// Copy to a vector for maximum iteration speed and cache optimization
		// (Need to take a copy anyway)
		
		_dependants.reserve(d->size());
		for (List<NodeImpl*>::iterator i = d->begin(); i != d->end(); ++i)
			_dependants.push_back(*i);
	}

	NodeImpl*                node()        const { return _node; }
	size_t                   n_providers() const { return _n_providers; }
	const vector<NodeImpl*>& dependants()  const { return _dependants; }

private:
	NodeImpl*         _node;
	size_t            _n_providers; ///< Number of input ready signals to trigger run
	vector<NodeImpl*> _dependants; ///< Nodes this one's output ports are connected to
};


/** A patch and a set of connections, "compiled" into a flat structure with
 * the correct order so the audio thread(s) can execute it without
 * threading problems (since the preprocessor thread fiddles with other
 * things).
 *
 * Currently objects still have some 'heavyweight' connection state, but
 * eventually this should be the only place a particular set of connections
 * in a patch is stored, so various "connection presets" can be switched
 * in a realtime safe way.
 *
 * The nodes contained here are sorted in the order they must be executed.
 * The parallel processing algorithm guarantees no node will be executed
 * before it's providers, using this order as well as semaphores.
 */
struct CompiledPatch : public std::vector<CompiledNode>
                     , public Raul::Deletable
                     , public boost::noncopyable {
	/*CompiledPatch() : std::vector<CompiledNode>() {}
	CompiledPatch(size_t reserve) : std::vector<CompiledNode>(reserve) {}*/
};


} // namespace Ingen

#endif // COMPILED_PATCH_HPP