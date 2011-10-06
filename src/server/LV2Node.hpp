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

#ifndef INGEN_ENGINE_LV2NODE_HPP
#define INGEN_ENGINE_LV2NODE_HPP

#include <string>
#include <boost/intrusive_ptr.hpp>
#include "lilv/lilv.h"
#include "lv2/lv2plug.in/ns/ext/contexts/contexts.h"
#include "types.hpp"
#include "NodeImpl.hpp"
#include "LV2Features.hpp"

namespace Ingen {
namespace Server {

class LV2Plugin;

/** An instance of a LV2 plugin.
 *
 * \ingroup engine
 */
class LV2Node : public NodeImpl
{
public:
	LV2Node(LV2Plugin*         plugin,
	        const std::string& name,
	        bool               polyphonic,
	        PatchImpl*         parent,
	        SampleRate         srate);

	~LV2Node();

	bool instantiate(BufferFactory& bufs);

	bool prepare_poly(BufferFactory& bufs, uint32_t poly);
	bool apply_poly(Raul::Maid& maid, uint32_t poly);

	void activate(BufferFactory& bufs);
	void deactivate();

	void message_run(MessageContext& context);

	void process(ProcessContext& context);

	void set_port_buffer(uint32_t voice, uint32_t port_num,
			boost::intrusive_ptr<Buffer> buf, SampleCount offset);

protected:
	inline LilvInstance* instance(uint32_t voice) {
		return (LilvInstance*)(*_instances)[voice].get();
	}

	typedef Raul::Array< SharedPtr<void> > Instances;

	LV2Plugin*                                   _lv2_plugin;
	Instances*                                   _instances;
	Instances*                                   _prepared_instances;
	LV2_Contexts_MessageContext*                 _message_funcs;
	SharedPtr<Shared::LV2Features::FeatureArray> _features;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2NODE_HPP

