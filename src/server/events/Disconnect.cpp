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

#include <list>

#include <glibmm/thread.h>

#include "ingen/Store.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "raul/log.hpp"

#include "Broadcaster.hpp"
#include "Buffer.hpp"
#include "DuplexPort.hpp"
#include "EdgeImpl.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"
#include "events/Disconnect.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Disconnect::Disconnect(Engine&              engine,
                       SharedPtr<Interface> client,
                       int32_t              id,
                       SampleCount          timestamp,
                       const Raul::Path&    tail_path,
                       const Raul::Path&    head_path)
	: Event(engine, client, id, timestamp)
	, _tail_path(tail_path)
	, _head_path(head_path)
	, _patch(NULL)
	, _impl(NULL)
	, _compiled_patch(NULL)
{
}

Disconnect::Impl::Impl(Engine&     e,
                       PatchImpl*  patch,
                       OutputPort* s,
                       InputPort*  d)
	: _engine(e)
	, _src_output_port(s)
	, _dst_input_port(d)
	, _patch(patch)
	, _edge(patch->remove_edge(_src_output_port, _dst_input_port))
	, _buffers(NULL)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	NodeImpl* const src_node = _src_output_port->parent_node();
	NodeImpl* const dst_node = _dst_input_port->parent_node();

	for (std::list<NodeImpl*>::iterator i = dst_node->providers().begin();
	     i != dst_node->providers().end(); ++i) {
		if ((*i) == src_node) {
			dst_node->providers().erase(i);
			break;
		}
	}

	for (std::list<NodeImpl*>::iterator i = src_node->dependants().begin();
	     i != src_node->dependants().end(); ++i) {
		if ((*i) == dst_node) {
			src_node->dependants().erase(i);
			break;
		}
	}

	_dst_input_port->decrement_num_edges();

	if (_dst_input_port->num_edges() == 0) {
		_buffers = new Raul::Array<BufferRef>(_dst_input_port->poly());
		_dst_input_port->get_buffers(*_engine.buffer_factory(),
		                             _buffers,
		                             _dst_input_port->poly(),
		                             false);

		const bool is_control = _dst_input_port->is_a(PortType::CONTROL) ||
			_dst_input_port->is_a(PortType::CV);
		const float value = is_control ? _dst_input_port->value().get_float() : 0;
		for (uint32_t i = 0; i < _buffers->size(); ++i) {
			if (is_control) {
				Buffer* buf = _buffers->at(i).get();
				buf->set_block(value, 0, buf->nframes());
			} else {
				_buffers->at(i)->clear();
			}
		}
	}
}

bool
Disconnect::pre_process()
{
	Glib::RWLock::WriterLock lock(_engine.store()->lock());

	if (_tail_path.parent().parent() != _head_path.parent().parent()
	    && _tail_path.parent() != _head_path.parent().parent()
	    && _tail_path.parent().parent() != _head_path.parent()) {
		return Event::pre_process_done(PARENT_DIFFERS, _head_path);
	}

	PortImpl* tail = dynamic_cast<PortImpl*>(_engine.store()->get(_tail_path));
	if (!tail) {
		return Event::pre_process_done(PORT_NOT_FOUND, _tail_path);
	}

	PortImpl* head = dynamic_cast<PortImpl*>(_engine.store()->get(_head_path));
	if (!head) {
		return Event::pre_process_done(PORT_NOT_FOUND, _head_path);
	}
	
	NodeImpl* const src_node = tail->parent_node();
	NodeImpl* const dst_node = head->parent_node();

	if (src_node->parent_patch() != dst_node->parent_patch()) {
		// Edge to a patch port from inside the patch
		assert(src_node->parent() == dst_node || dst_node->parent() == src_node);
		if (src_node->parent() == dst_node) {
			_patch = dynamic_cast<PatchImpl*>(dst_node);
		} else {
			_patch = dynamic_cast<PatchImpl*>(src_node);
		}
	} else if (src_node == dst_node && dynamic_cast<PatchImpl*>(src_node)) {
		// Edge from a patch input to a patch output (pass through)
		_patch = dynamic_cast<PatchImpl*>(src_node);
	} else {
		// Normal edge between nodes with the same parent
		_patch = src_node->parent_patch();
	}

	assert(_patch);

	if (!_patch->has_edge(tail, head)) {
		return Event::pre_process_done(NOT_FOUND, _head_path);
	}

	if (src_node == NULL || dst_node == NULL) {
		return Event::pre_process_done(PARENT_NOT_FOUND, _head_path);
	}

	_impl = new Impl(_engine,
	                 _patch,
	                 dynamic_cast<OutputPort*>(tail),
	                 dynamic_cast<InputPort*>(head));

	if (_patch->enabled())
		_compiled_patch = _patch->compile();

	return Event::pre_process_done(SUCCESS);
}

bool
Disconnect::Impl::execute(ProcessContext& context, bool set_dst_buffers)
{
	EdgeImpl* const port_edge =
		_dst_input_port->remove_edge(context, _src_output_port);
	if (!port_edge) {
		return false;
	}

	if (set_dst_buffers) {
		if (_buffers) {
			_engine.maid()->dispose(_dst_input_port->set_buffers(context, _buffers));
		} else {
			_dst_input_port->setup_buffers(*_engine.buffer_factory(),
			                               _dst_input_port->poly(),
			                               true);
		}
		_dst_input_port->connect_buffers();
	} else {
		_dst_input_port->recycle_buffers();
	}

	assert(_edge);
	assert(port_edge == _edge.get());

	return true;
}

void
Disconnect::execute(ProcessContext& context)
{
	if (_status == SUCCESS) {
		if (!_impl->execute(context, true)) {
			_status = NOT_FOUND;
			return;
		}

		_engine.maid()->dispose(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}

void
Disconnect::post_process()
{
	if (!respond()) {
		_engine.broadcaster()->disconnect(_tail_path, _head_path);
	}

	delete _impl;
}

} // namespace Events
} // namespace Server
} // namespace Ingen

