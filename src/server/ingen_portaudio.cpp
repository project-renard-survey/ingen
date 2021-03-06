/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "PortAudioDriver.hpp"
#include "Engine.hpp"

#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"
#include "ingen/types.hpp"

namespace ingen { namespace server { class Driver; } }

using namespace ingen;

struct IngenPortAudioModule : public ingen::Module {
	void load(ingen::World& world) override {
		if (((server::Engine*)world.engine().get())->driver()) {
			world.log().warn("Engine already has a driver\n");
			return;
		}

		server::PortAudioDriver* driver = new server::PortAudioDriver(
			*(server::Engine*)world.engine().get());
		driver->attach();
		((server::Engine*)world.engine().get())->set_driver(
			SPtr<server::Driver>(driver));
	}
};

extern "C" {

ingen::Module*
ingen_module_load()
{
	return new IngenPortAudioModule();
}

} // extern "C"
