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

#ifndef INGEN_ENGINE_NODEIMPL_HPP
#define INGEN_ENGINE_NODEIMPL_HPP

#include <string>
#include "raul/IntrusivePtr.hpp"
#include "interface/Node.hpp"
#include "GraphObjectImpl.hpp"
#include "types.hpp"

namespace Raul { template <typename T> class List; class Maid; }

namespace Ingen {

namespace Shared { class Plugin; class Node; class Port; }

class Buffer;
class BufferFactory;
class Context;
class MessageContext;
class PatchImpl;
class PluginImpl;
class PortImpl;
class ProcessContext;


/** A Node (or "module") in a Patch (which is also a Node).
 *
 * A Node is a unit with input/output ports, a process() method, and some other
 * things.
 *
 * This is a pure abstract base class for any Node, it contains no
 * implementation details/data whatsoever.  This is the interface you need to
 * implement to add a new Node type.
 *
 * \ingroup engine
 */
class NodeImpl : public GraphObjectImpl, virtual public Ingen::Shared::Node
{
public:
	NodeImpl(GraphObjectImpl* parent, const Raul::Symbol& symbol)
		: GraphObjectImpl(parent, symbol)
	{}

	/** Activate this Node.
	 *
	 * This function will be called in a non-realtime thread before it is
	 * inserted in to a patch.  Any non-realtime actions that need to be
	 * done before the Node is ready for use should be done here.
	 */
	virtual void activate(BufferFactory& bufs) = 0;
	virtual void deactivate()                  = 0;
	virtual bool activated()                   = 0;

	/** Parallelism: Reset flags for start of a new cycle.
	 */
	virtual void reset_input_ready() = 0;

	/** Parallelism: Claim this node (to wait on its input).
	 * Only one thread will ever take this lock on a particular Node.
	 * \return true if lock was aquired, false otherwise
	 */
	virtual bool process_lock() = 0;

	/** Parallelism: Unclaim this node (let someone else wait on its input).
	 * Only a thread which successfully called process_lock may call this.
	 */
	virtual void process_unlock() = 0;

	/** Parallelism: Wait for signal that input is ready.
	 * Only a thread which successfully called process_lock may call this.
	 */
	virtual void wait_for_input(size_t num_providers) = 0;

	/** Parallelism: Signal that input is ready.  Realtime safe.
	 * Calling this will wake up the thread which blocked on wait_for_input
	 * if there is one, and otherwise cause it to return true the next call.
	 * \return true if lock was aquired and input is ready, false otherwise
	 */
	virtual void signal_input_ready() = 0;

	/** Parallelism: Return the number of providers that have signalled.
	 */
	virtual unsigned n_inputs_ready() const = 0;

	/** Run the node for one instant in the message thread.
	 */
	virtual void message_run(MessageContext& context) = 0;

	/** Flag a port as valid (for message context) */
	virtual void set_port_valid(uint32_t index) = 0;

	/** Return a bit vector of which ports are valid */
	virtual void* valid_ports() = 0;

	/** Clear all bits in valid_ports() */
	virtual void reset_valid_ports() = 0;

	/** Run the node for @a nframes input/output.
	 *
	 * @a start and @a end are transport times: end is not redundant in the case
	 * of varispeed, where end-start != nframes.
	 */
	virtual void process(ProcessContext& context) = 0;

	virtual void set_port_buffer(uint32_t voice, uint32_t port_num,
			IntrusivePtr<Buffer> buf, SampleCount offset) = 0;

	virtual uint32_t num_ports() const = 0;

	virtual Shared::Port* port(uint32_t index) const = 0;
	virtual PortImpl*     port_impl(uint32_t index) const = 0;

	/** Used by the process order finding algorithm (ie during connections) */
	virtual bool traversed() const  = 0;
	virtual void traversed(bool b)  = 0;

	/** Nodes that are connected to this Node's inputs.
	 * (This Node depends on them)
	 */
	virtual Raul::List<NodeImpl*>* providers()                         = 0;
	virtual void                   providers(Raul::List<NodeImpl*>* l) = 0;

	/** Nodes are are connected to this Node's outputs.
	 * (They depend on this Node)
	 */
	virtual Raul::List<NodeImpl*>* dependants()                         = 0;
	virtual void                   dependants(Raul::List<NodeImpl*>* l) = 0;

	/** The Patch this Node belongs to. */
	virtual PatchImpl* parent_patch() const = 0;

	/** Information about the Plugin this Node is an instance of.
	 * Not the best name - not all nodes come from plugins (ie Patch)
	 */
	virtual PluginImpl* plugin_impl() const = 0;

	/** Information about the Plugin this Node is an instance of.
	 * Not the best name - not all nodes come from plugins (ie Patch)
	 */
	virtual const Shared::Plugin* plugin() const = 0;

	virtual void set_buffer_size(Context& context, BufferFactory& bufs,
			Shared::PortType type, size_t size) = 0;
};


} // namespace Ingen

#endif // INGEN_ENGINE_NODEIMPL_HPP
