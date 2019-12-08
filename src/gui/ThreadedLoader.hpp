/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_THREADEDLOADER_HPP
#define INGEN_GUI_THREADEDLOADER_HPP

#include "ingen/FilePath.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Parser.hpp"
#include "ingen/Serialiser.hpp"
#include "raul/Semaphore.hpp"

#include <boost/optional/optional.hpp>
#include <sigc++/sigc++.h>

#include <list>
#include <mutex>
#include <thread>
#include <utility>

namespace ingen {

class URI;

namespace client { class GraphModel; }

namespace gui {

class App;

/** Thread for loading graph files.
 *
 * This is a seperate thread so it can send all the loading message without
 * blocking everything else, so the app can respond to the incoming events
 * caused as a result of the graph loading, while the graph loads.
 *
 * Implemented as a slave with a list of closures (events) which processes
 * all events in the (mutex protected) list each time it's whipped.
 *
 * \ingroup GUI
 */
class ThreadedLoader
{
public:
	ThreadedLoader(App&            app,
	               SPtr<Interface> engine);

	~ThreadedLoader();

	void load_graph(bool                          merge,
	                const FilePath&               file_path,
	                boost::optional<Raul::Path>   engine_parent,
	                boost::optional<Raul::Symbol> engine_symbol,
	                boost::optional<Properties>   engine_data);

	void save_graph(SPtr<const client::GraphModel> model, const URI& uri);

	SPtr<Parser> parser();

private:
	void load_graph_event(const FilePath&               file_path,
	                      boost::optional<Raul::Path>   engine_parent,
	                      boost::optional<Raul::Symbol> engine_symbol,
	                      boost::optional<Properties>   engine_data);

	void save_graph_event(SPtr<const client::GraphModel> model,
	                      const URI&                     filename);

	/** Returns nothing and takes no parameters (because they have all been bound) */
	using Closure = sigc::slot<void>;

	void run();

	App&               _app;
	Raul::Semaphore    _sem;
	SPtr<Interface>    _engine;
	std::mutex         _mutex;
	std::list<Closure> _events;
	bool               _exit_flag;
	std::thread        _thread;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_LOADERRTHREAD_HPP
