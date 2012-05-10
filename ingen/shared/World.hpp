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

#ifndef INGEN_MODULE_WORLD_HPP
#define INGEN_MODULE_WORLD_HPP

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "ingen/shared/Forge.hpp"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/Atom.hpp"
#include "raul/Configuration.hpp"
#include "raul/SharedPtr.hpp"

typedef struct LilvWorldImpl LilvWorld;

namespace Sord { class World; }

namespace Ingen {

class EngineBase;
class Interface;

namespace Serialisation {
class Serialiser;
class Parser;
}

namespace Shared {

class LV2Features;
class URIs;
class LV2URIMap;
class Store;

/** The "world" all Ingen modules may share.
 *
 * All loaded components of Ingen, as well as things requiring shared access
 * and/or locking (e.g. Sord, Lilv).
 *
 * Ingen modules are shared libraries which modify the World when loaded
 * using World::load, e.g. loading the "ingen_serialisation" module will
 * set World::serialiser and World::parser to valid objects.
 */
class World : public boost::noncopyable {
public:
	World(Raul::Configuration* conf,
	      int&                 argc,
	      char**&              argv,
	      LV2_URID_Map*        map,
	      LV2_URID_Unmap*      unmap);

	virtual ~World();

	virtual bool load_module(const char* name);
	virtual bool run_module(const char* name);

	virtual void unload_modules();

	typedef SharedPtr<Interface> (*InterfaceFactory)(
			World*               world,
			const std::string&   engine_url,
			SharedPtr<Interface> respondee);

	virtual void add_interface_factory(const std::string& scheme,
	                                   InterfaceFactory   factory);

	virtual SharedPtr<Interface> interface(
		const std::string&   engine_url,
		SharedPtr<Interface> respondee);

	virtual bool run(const std::string& mime_type,
	                 const std::string& filename);

	virtual void set_local_engine(SharedPtr<EngineBase> e);
	virtual void set_engine(SharedPtr<Interface> e);
	virtual void set_serialiser(SharedPtr<Serialisation::Serialiser> s);
	virtual void set_parser(SharedPtr<Serialisation::Parser> p);
	virtual void set_store(SharedPtr<Store> s);

	virtual SharedPtr<EngineBase>                local_engine();
	virtual SharedPtr<Interface>                 engine();
	virtual SharedPtr<Serialisation::Serialiser> serialiser();
	virtual SharedPtr<Serialisation::Parser>     parser();
	virtual SharedPtr<Store>                     store();

	virtual Sord::World* rdf_world();

	virtual SharedPtr<URIs>      uris();
	virtual SharedPtr<LV2URIMap> lv2_uri_map();

	virtual int&    argc();
	virtual char**& argv();

	virtual Raul::Configuration* conf();
	virtual void set_conf(Raul::Configuration* c);

	virtual Ingen::Forge& forge();

	virtual LV2Features* lv2_features();

	virtual LilvWorld* lilv_world();

	virtual void        set_jack_uuid(const std::string& uuid);
	virtual std::string jack_uuid();

private:
	class Pimpl;

	Pimpl* _impl;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_MODULE_WORLD_HPP
