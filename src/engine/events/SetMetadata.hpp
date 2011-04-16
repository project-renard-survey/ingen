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

#ifndef INGEN_EVENTS_SETMETADATA_HPP
#define INGEN_EVENTS_SETMETADATA_HPP

#include <vector>
#include "raul/URI.hpp"
#include "shared/ResourceImpl.hpp"
#include "QueuedEvent.hpp"

namespace Ingen {

class GraphObjectImpl;
class PatchImpl;
class CompiledPatch;

namespace Events {

/** \page methods
 * <h2>POST</h2>
 * As per HTTP (RFC2616 S9.5).
 *
 * Append properties to a graph object.
 *
 * An object can have several properties with a single predicate.
 * POST appends properties without modifying or removing existing properties.
 */

/** \page methods
 * <h2>PUT</h2>
 * As per HTTP (RFC2616 S9.6).
 *
 * Set properties of a graph object, or create an object.
 *
 * An object can have several properties with a single predicate.
 * \li If the object does not yet exist, the message must contain sufficient
 * information to create the object (e.g. known rdf:type properties, etc.)
 * \li If the object does exist, a PUT removes all existing object properties
 * with predicates that match any property in the message, then adds all
 * properties from the message.
 */

class SetPortValue;

/** Set properties of a graph object.
 * \ingroup engine
 */
class SetMetadata : public QueuedEvent
{
public:
	SetMetadata(
			Engine&                             engine,
			SharedPtr<Request>                  request,
			SampleCount                         timestamp,
			bool                                create,
			Shared::Resource::Graph             context,
			const Raul::URI&                    subject,
			const Shared::Resource::Properties& properties,
			const Shared::Resource::Properties& remove=Shared::Resource::Properties());

	~SetMetadata();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType {
		NO_ERROR,
		NOT_FOUND,
		INTERNAL,
		BAD_OBJECT_TYPE,
		BAD_VALUE_TYPE
	};

	enum SpecialType {
		NONE,
		ENABLE,
		ENABLE_BROADCAST,
		POLYPHONY,
		POLYPHONIC,
		CONTROL_BINDING
	};

	typedef std::vector<SetPortValue*> SetEvents;

	QueuedEvent*                 _create_event;
	SetEvents                    _set_events;
	std::vector<SpecialType>     _types;
	std::vector<SpecialType>     _remove_types;
	Raul::URI                    _subject;
	Shared::Resource::Properties _properties;
	Shared::Resource::Properties _remove;
	Shared::ResourceImpl*        _object;
	PatchImpl*                   _patch;
	CompiledPatch*               _compiled_patch;
	std::string                  _error_predicate;
	bool                         _create;
	Shared::Resource::Graph      _context;

	SharedPtr<ControlBindings::Bindings> _old_bindings;
};

} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_SETMETADATA_HPP
