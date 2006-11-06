/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef PATCHLIBRARIAN_H
#define PATCHLIBRARIAN_H

#include <map>
#include <utility>
#include <string>
#include <stdexcept>
#include <cassert>
#include <boost/optional/optional.hpp>
#include <raptor.h>
#include "raul/SharedPtr.h"
#include "raul/Path.h"
#include "raul/Atom.h"
#include "ObjectModel.h"

using std::string;
using boost::optional;

namespace Ingen {
namespace Client {

class PatchModel;
class NodeModel;
class PortModel;
class ConnectionModel;
class PresetModel;
class ModelEngineInterface;


/** Namespace prefix macros. */
#define NS_RDF(x) "http://www.w3.org/1999/02/22-rdf-syntax-ns#" x
#define NS_INGEN(x) "http://codeson.net/ns/ingen#" x

	
/** Handles all patch saving and loading.
 *
 * \ingroup IngenClient
 */
class Serializer
{
public:
	Serializer(SharedPtr<ModelEngineInterface> engine);
	~Serializer();

	void          path(const string& path) { _patch_search_path = path; }
	const string& path()                   { return _patch_search_path; }
	
	string find_file(const string& filename, const string& additional_path = "");
	
	bool load_patch(bool                    merge,
	                const string&           data_base_uri,
	                const Path&             data_path,
	                MetadataMap             engine_data,
	                optional<const Path&>   engine_parent = optional<const Path&>(),
	                optional<const string&> engine_name = optional<const string&>(),
	                optional<size_t>        engine_poly = optional<size_t>());

	
	void   start_to_filename(const string& filename)            throw (std::logic_error);
	void   start_to_string()                                    throw (std::logic_error);
	void   serialize(SharedPtr<ObjectModel> object)             throw (std::logic_error);
	void   serialize_connection(SharedPtr<ConnectionModel> c)   throw (std::logic_error);
	string finish()                                             throw (std::logic_error);

private:

	// Model -> RDF
	
	void serialize_patch(SharedPtr<PatchModel> p);

	void serialize_node(SharedPtr<NodeModel> n, const string ns_prefix="");
	
	void serialize_port(SharedPtr<PortModel> p, const string ns_prefix="");
	



	// Triple -> RDF
	
    void serialize_resource(const string& subject_uri,
	                        const string& predicate_uri,
	                        const string& object_uri);
	
    void serialize_resource_blank(const string& node_id,
	                              const string& predicate_uri,
	                              const string& object_uri);
	
	void serialize_atom(const string& subject_uri,
	                    const string& predicate_uri,
	                    const Atom&   atom);

	void   setup_prefixes();
	string expand_uri(const string& uri);


	raptor_serializer*               _serializer;
	unsigned char*                   _string_output;
	string                           _patch_search_path;
	map<string, string>              _prefixes;
	SharedPtr<ModelEngineInterface> _engine;
};


} // namespace Client
} // namespace Ingen

#endif // PATCHLIBRARIAN_H
