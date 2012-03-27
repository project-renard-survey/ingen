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

#include "ingen/shared/Module.hpp"
#include "ingen/shared/World.hpp"
#include "Engine.hpp"
#include "ServerInterfaceImpl.hpp"
#include "util.hpp"

using namespace Ingen;

struct IngenEngineModule : public Ingen::Shared::Module {
	virtual void load(Ingen::Shared::World* world) {
		Server::set_denormal_flags();
		SharedPtr<Server::Engine> engine(new Server::Engine(world));
		world->set_local_engine(engine);
		SharedPtr<Server::ServerInterfaceImpl> interface(
			new Server::ServerInterfaceImpl(*engine.get()));
		world->set_engine(interface);
		engine->add_event_source(interface);
		assert(world->local_engine() == engine);
	}
};

extern "C" {

Ingen::Shared::Module*
ingen_module_load()
{
	return new IngenEngineModule();
}

} // extern "C"
