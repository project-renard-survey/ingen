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

#ifndef INGEN_EVENTS_DELETE_HPP
#define INGEN_EVENTS_DELETE_HPP

#include "Event.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "ControlBindings.hpp"

namespace Raul {
	template<typename T> class Array;
	template<typename T> class ListNode;
}

namespace Ingen {
namespace Server {

class GraphObjectImpl;
class NodeImpl;
class PortImpl;
class DriverPort;
class CompiledPatch;

namespace Events {

class DisconnectAll;

/** \page methods
 * <h2>DELETE</h2>
 * As per WebDAV (RFC4918 S9.6).
 *
 * Remove an object from the engine and destroy it.
 *
 * \li All properties of the object are lost
 * \li All references to the object are lost (e.g. the parent's reference to
 *     this child is lost, any connections to the object are removed, etc.)
 */

/** DELETE a graph object (see \ref methods).
 * \ingroup engine
 */
class Delete : public Event
{
public:
	Delete(Engine&          engine,
	       Interface*       client,
	       int32_t          id,
	       FrameTime        timestamp,
	       const Raul::URI& uri);

	~Delete();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::URI                      _uri;
	Raul::Path                     _path;
	EngineStore::iterator          _store_iterator;
	SharedPtr<NodeImpl>            _node;                ///< Non-NULL iff a node
	SharedPtr<PortImpl>            _port;                ///< Non-NULL iff a port
	Raul::Deletable*               _garbage;
	DriverPort*                    _driver_port;
	PatchImpl::Nodes::Node*        _patch_node_listnode;
	Raul::List<PortImpl*>::Node*   _patch_port_listnode;
	Raul::Array<PortImpl*>*        _ports_array;         ///< New (external) ports for Patch
	CompiledPatch*                 _compiled_patch;      ///< Patch's new process order
	DisconnectAll*                 _disconnect_event;

	SharedPtr<ControlBindings::Bindings> _removed_bindings;

	SharedPtr< Raul::Table<Raul::Path, SharedPtr<GraphObject> > > _removed_table;

	Glib::RWLock::WriterLock _lock;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_DELETE_HPP
