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

#include <signal.h>

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/optional.hpp>

#include "raul/Path.hpp"

#include "serd/serd.h"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#include "ingen_config.h"

#include "ingen/AtomReader.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Parser.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/Store.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "ingen/filesystem.hpp"
#include "ingen/runtime_paths.hpp"
#include "ingen/types.hpp"

#include "TestClient.hpp"

using namespace std;
using namespace ingen;

World* world = nullptr;

static void
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
	set_bundle_path_from_code((void*)&ingen_try);

	// Create world
	try {
		world = new World(nullptr, nullptr, nullptr);
		world->load_configuration(argc, argv);
	} catch (std::exception& e) {
		cout << "ingen: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	// Get mandatory command line arguments
	const Atom& load    = world->conf().option("load");
	const Atom& execute = world->conf().option("execute");
	if (!load.is_valid() || !execute.is_valid()) {
		cerr << "Usage: ingen_test --load START_GRAPH --execute COMMANDS_FILE" << endl;
		return EXIT_FAILURE;
	}

	// Get start graph and commands file options
	const char* load_path        = (const char*)load.get_body();
	char*       real_start_graph = realpath(load_path, nullptr);
	if (!real_start_graph) {
		cerr << "error: initial graph '" << load_path << "' does not exist" << endl;
		return EXIT_FAILURE;
	}

	const std::string start_graph    = real_start_graph;
	const FilePath    cmds_file_path = (const char*)execute.get_body();
	free(real_start_graph);

	// Load modules
	ingen_try(world->load_module("server"),
	          "Unable to load server module");

	// Initialise engine
	ingen_try(bool(world->engine()),
	          "Unable to create engine");
	world->engine()->init(48000.0, 4096, 4096);
	world->engine()->activate();

	// Load graph
	if (!world->parser()->parse_file(*world, *world->interface(), start_graph)) {
		cerr << "error: failed to load initial graph " << start_graph << endl;
		return EXIT_FAILURE;
	}
	world->engine()->flush_events(std::chrono::milliseconds(20));

	// Read commands

	LV2_URID_Map* map    = &world->uri_map().urid_map_feature()->urid_map;
	Sratom*       sratom = sratom_new(map);

	sratom_set_object_mode(sratom, SRATOM_OBJECT_MODE_BLANK_SUBJECT);

	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, map);

	AtomForgeSink out(&forge);

	// AtomReader to read commands from a file and send them to engine
	AtomReader atom_reader(world->uri_map(),
	                       world->uris(),
	                       world->log(),
	                       *world->interface().get());

	// AtomWriter to serialise responses from the engine
	SPtr<Interface> client(new TestClient(world->log()));

	world->interface()->set_respondee(client);
	world->engine()->register_client(client);

	SerdURI cmds_base;
	SerdNode cmds_file_uri = serd_node_new_file_uri(
		(const uint8_t*)cmds_file_path.c_str(),
		nullptr, &cmds_base, true);
	Sord::Model* cmds = new Sord::Model(*world->rdf_world(),
	                                    (const char*)cmds_file_uri.buf);
	SerdEnv* env = serd_env_new(&cmds_file_uri);
	cmds->load_file(env, SERD_TURTLE, cmds_file_path);
	Sord::Node nil;
	int n_events = 0;
	for (;; ++n_events) {
		std::string subject_str = (fmt("msg%1%") % n_events).str();
		Sord::URI subject(*world->rdf_world(), subject_str,
		                  (const char*)cmds_file_uri.buf);
		Sord::Iter iter = cmds->find(subject, nil, nil);
		if (iter.end()) {
			break;
		}

		out.clear();
		sratom_read(sratom, &forge, world->rdf_world()->c_obj(),
		            cmds->c_obj(), subject.c_obj());

#if 0
		const LV2_Atom* atom = out.atom();
		cerr << "READ " << atom->size << " BYTES" << endl;
		cerr << sratom_to_turtle(
			sratom,
			&world->uri_map().urid_unmap_feature()->urid_unmap,
			(const char*)cmds_file_uri.buf,
			nullptr, nullptr, atom->type, atom->size, LV2_ATOM_BODY(atom)) << endl;
#endif

		if (!atom_reader.write(out.atom(), n_events + 1)) {
			return EXIT_FAILURE;
		}

		world->engine()->flush_events(std::chrono::milliseconds(20));
	}

	delete cmds;

	// Save resulting graph
	auto              r        = world->store()->find(Raul::Path("/"));
	const std::string base     = cmds_file_path.stem();
	const std::string out_name = base.substr(0, base.find('.')) + ".out.ingen";
	const FilePath    out_path = filesystem::current_path() / out_name;
	world->serialiser()->write_bundle(r->second, URI(out_path));

	// Undo every event (should result in a graph identical to the original)
	for (int i = 0; i < n_events; ++i) {
		world->interface()->undo();
		world->engine()->flush_events(std::chrono::milliseconds(20));
	}

	// Save completely undone graph
	r = world->store()->find(Raul::Path("/"));
	const std::string undo_name = base.substr(0, base.find('.')) + ".undo.ingen";
	const FilePath    undo_path = filesystem::current_path() / undo_name;
	world->serialiser()->write_bundle(r->second, URI(undo_path));

	// Redo every event (should result in a graph identical to the pre-undo output)
	for (int i = 0; i < n_events; ++i) {
		world->interface()->redo();
		world->engine()->flush_events(std::chrono::milliseconds(20));
	}

	// Save completely redone graph
	r = world->store()->find(Raul::Path("/"));
	const std::string redo_name = base.substr(0, base.find('.')) + ".redo.ingen";
	const FilePath    redo_path = filesystem::current_path() / redo_name;
	world->serialiser()->write_bundle(r->second, URI(redo_path));

	serd_env_free(env);
	sratom_free(sratom);
	serd_node_free(&cmds_file_uri);

	// Shut down
	world->engine()->deactivate();

	delete world;
	return EXIT_SUCCESS;
}
