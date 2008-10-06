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

#ifndef UNREGISTERCLIENTEVENT_H
#define UNREGISTERCLIENTEVENT_H

#include "QueuedEvent.hpp"
#include <string>
using std::string;

namespace Ingen {

namespace Shared {
	class ClientInterface;
}
using Shared::ClientInterface;


/** Unregisters an OSC client so it no longer receives notifications.
 *
 * \ingroup engine
 */
class UnregisterClientEvent : public QueuedEvent
{
public:
	UnregisterClientEvent(Engine&                      engine,
	                      SharedPtr<Responder> responder,
	                      SampleCount                  timestamp,
	                      const string&                uri);

	void post_process();

private:
	string _uri;
};


} // namespace Ingen

#endif // UNREGISTERCLIENTEVENT_H