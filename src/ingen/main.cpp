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

#include <signal.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <string>

#include <boost/optional.hpp>

#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/thread.h>

#include "raul/Configuration.hpp"
#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/Thread.hpp"
#include "raul/log.hpp"

#include "serd/serd.h"
#include "sord/sordmm.hpp"

#include "ingen-config.h"

#include "ingen/EngineBase.hpp"
#include "ingen/EngineInterface.hpp"
#include "serialisation/Parser.hpp"
#include "shared/Configuration.hpp"
#include "shared/World.hpp"
#include "shared/runtime_paths.hpp"
#ifdef WITH_BINDINGS
#include "bindings/ingen_bindings.hpp"
#endif

using namespace std;
using namespace Raul;
using namespace Ingen;

static const timespec main_rate = { 0, 125000000 }; // 1/8 second

Ingen::Shared::World* world = NULL;

void
ingen_interrupt(int)
{
	cout << "ingen: Interrupted" << endl;
	if (world->local_engine())
		world->local_engine()->quit();
	delete world;
	exit(EXIT_FAILURE);
}

void
ingen_try(bool cond, const char* msg)
{
	if (!cond) {
		cerr << "ingen: Error: " << msg << endl;
		delete world;
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char** argv)
{
	Shared::Configuration conf;

	// Parse command line options
	try {
		conf.parse(argc, argv);
	} catch (std::exception& e) {
		cout << "ingen: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	// Verify option sanity
	if (argc <= 1) {
		conf.print_usage("ingen", cout);
		return EXIT_FAILURE;
	} else if (conf.option("help").get_bool()) {
		conf.print_usage("ingen", cout);
		return EXIT_SUCCESS;
	}

	// Set bundle path from executable location so resources can be found
	Shared::set_bundle_path_from_code((void*)&main);

	SharedPtr<Shared::EngineInterface> engine_interface;

	Glib::thread_init();
#if HAVE_SOUP
	g_type_init();
#endif

	Ingen::Shared::World* world = new Ingen::Shared::World(&conf, argc, argv);

#if INGEN_JACK_SESSION
	if (conf.option("uuid").get_string()) {
		world->set_jack_uuid(conf.option("uuid").get_string());
	}
#endif

	// Run engine
	if (conf.option("engine").get_bool()) {
		ingen_try(world->load_module("engine"),
		          "Unable to load engine module");

		ingen_try(world->local_engine(),
		          "Unable to create engine");

		engine_interface = world->engine();

		// Not loading a GUI, load network engine interfaces
		if (!conf.option("gui").get_bool()) {
			#ifdef HAVE_LIBLO
			ingen_try(world->load_module("osc"),
			          "Unable to load OSC module");
			#endif
			#ifdef HAVE_SOUP
			ingen_try(world->load_module("http"),
			          "Unable to load HTTP module");
			#endif
		}
	}

	// If we don't have a local engine interface (for GUI), use network
	if (!engine_interface) {
		const char* const uri = conf.option("connect").get_string();
		ingen_try((engine_interface = world->interface(uri)),
		          (string("Unable to create interface to `") + uri + "'").c_str());
	}

	// Activate the engine, if we have one
	if (world->local_engine()) {
		ingen_try(world->load_module("jack"),
		          "Unable to load jack module");
	}

	world->set_engine(engine_interface);

	// Load a patch
	if (conf.option("load").is_valid() && engine_interface) {
		boost::optional<Path>   parent;
		boost::optional<Symbol> symbol;
		const Raul::Atom&       path_option = conf.option("path");

		if (path_option.is_valid()) {
			if (Path::is_valid(path_option.get_string())) {
				const Path p(path_option.get_string());
				if (!p.is_root()) {
					parent = p.parent();
					symbol = p.symbol();
				}
			} else {
				cerr << "Invalid path given: '" << path_option << endl;
			}
		}

		ingen_try(world->load_module("serialisation"),
		          "Unable to load serialisation module");

		ingen_try(world->parser(),
		          "Unable to create parser");

		string uri = conf.option("load").get_string();
		if (!serd_uri_string_has_scheme((const uint8_t*)uri.c_str())) {
			// Does not start with legal URI scheme, assume path
			uri = Glib::filename_to_uri(
				(Glib::path_is_absolute(uri))
				? uri
				: Glib::build_filename(Glib::get_current_dir(), uri));
		}

		engine_interface->load_plugins();
		if (conf.option("gui").get_bool())
			engine_interface->get("ingen:plugins");
		world->parser()->parse_file(
			world, engine_interface.get(), uri, parent, symbol);
	}

	// Load GUI
	if (conf.option("gui").get_bool())
		ingen_try(world->load_module("gui"),
		          "Unable to load GUI module");

	// Run a script
	if (conf.option("run").is_valid()) {
#ifdef WITH_BINDINGS
		ingen_try(world->load_module("bindings"),
		          "Unable to load bindings module");

		world->run("application/x-python", conf.option("run").get_string());
#else
		cerr << "This build of ingen does not support scripting." << endl;
#endif

	// Run main loop
	} else if (world->local_engine() && !conf.option("gui").get_bool()) {
		// Set up signal handlers that will set quit_flag on interrupt
		signal(SIGINT, ingen_interrupt);
		signal(SIGTERM, ingen_interrupt);

		// Activate the enginie
		world->local_engine()->activate();

		// Run engine main loop until interrupt
		while (world->local_engine()->main_iteration()) {
			nanosleep(&main_rate, NULL);
		}
		info << "Finished main loop" << endl;

		world->local_engine()->deactivate();
	}

	// Shut down
	if (world->local_engine())
		world->local_engine()->deactivate();

	delete world;

	return 0;
}

