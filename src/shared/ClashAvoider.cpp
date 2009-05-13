/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#include <sstream>
#include "ClashAvoider.hpp"
#include "Store.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


/** Always returns a valid Raul::Path */
const Path
ClashAvoider::map_path(const Raul::Path& in)
{
	//cout << "MAP PATH: " << in;
	//cout << endl << "**** MAP PATH: " << in << endl;
	
	unsigned offset = 0;
	bool has_offset = false;
	size_t pos = in.find_last_of("_");
	if (pos != string::npos && pos != (in.length()-1)) {
		const std::string trailing = in.substr(in.find_last_of("_")+1);
		has_offset = (sscanf(trailing.c_str(), "%u", &offset) > 0);
	}

	//cout << "OFFSET: " << offset << endl;
		
	// Path without _n suffix
	Path base_path = in;
	if (has_offset)
		base_path = base_path.substr(0, base_path.find_last_of("_"));

	//cout << "BASE: " << base_path << endl;

	SymbolMap::iterator m = _symbol_map.find(in);
	if (m != _symbol_map.end()) {
		//cout << " (1) " << m->second << endl;
		return m->second;
	} else {
		typedef std::pair<SymbolMap::iterator, bool> InsertRecord;
			
		// See if parent is mapped
		Path parent = in.parent();
		do {
			//cout << "CHECK: " << parent << endl;
			SymbolMap::iterator p = _symbol_map.find(parent);
			if (p != _symbol_map.end()) {
				const Path mapped = p->second.base() + in.substr(parent.base().length());
				InsertRecord i = _symbol_map.insert(make_pair(in, mapped));
				//cout << " (2) " << i.first->second << endl;
				return i.first->second;
			}
			parent = parent.parent();
		} while (!parent.is_root());

		// No clash, use symbol unmodified
		if (!exists(in) && _symbol_map.find(in) == _symbol_map.end()) {
			InsertRecord i = _symbol_map.insert(make_pair(in, in));
			assert(i.second);
			//cout << " (3) " << i.first->second << endl;;
			return i.first->second;
			
		// Append _2 _3 etc until an unused symbol is found
		} else {
			while (true) {
				Offsets::iterator o = _offsets.find(base_path);
				if (o != _offsets.end()) {
					offset = ++o->second;
				} else {
					string parent_str = in.parent().base();
					parent_str = parent_str.substr(0, parent_str.find_last_of("/"));
					if (parent_str == "")
						parent_str = "/";
					//cout << "***** PARENT: " << parent_str << endl;
				}
				
				if (offset == 0)
					offset = 2;

				std::stringstream ss;
				ss << base_path << "_" << offset;
				if (!exists(ss.str())) {
					const string name = (base_path.length() > 1) ? base_path.name() : "_";
					string str = ss.str();
					InsertRecord i = _symbol_map.insert(make_pair(in, str));
					//cout << "HIT: offset = " << offset << ", str = " << str << endl;
					offset = _store.child_name_offset(in.parent(), name, false);
					_offsets.insert(make_pair(base_path, offset));
					//cout << " (4) " << i.first->second << endl;;
					return i.first->second;
				} else {
					//cout << "MISSED OFFSET: " << in << " => " << ss.str() << endl;
					if (o != _offsets.end())
						offset = ++o->second;
					else
						++offset;
				}
			}
		}
	}
}


bool
ClashAvoider::exists(const Raul::Path& path) const
{
	bool exists = (_store.find(path) != _store.end());
	if (exists)
		return true;

	if (_also_avoid)
		return (_also_avoid->find(path) != _also_avoid->end());
	else
		return false;
}


bool
ClashAvoider::new_object(const GraphObject* object)
{
	return false;
}


void
ClashAvoider::new_patch(const Raul::Path& path,
                        uint32_t          poly)
{
	if (!path.is_root())
		_target.new_patch(map_path(path), poly);
}


void
ClashAvoider::new_node(const Raul::Path& path,
                       const Raul::URI&  plugin_uri)
{
	_target.new_node(map_path(path), plugin_uri);
}


void
ClashAvoider::new_port(const Raul::Path& path,
                       const Raul::URI&  type,
                       uint32_t          index,
                       bool              is_output)
{
	_target.new_port(map_path(path), type, index, is_output);
}


void
ClashAvoider::rename(const Raul::Path& old_path,
                     const Raul::Path& new_path)
{
	_target.rename(map_path(old_path), map_path(new_path));
}


void
ClashAvoider::connect(const Raul::Path& src_port_path,
                      const Raul::Path& dst_port_path)
{
	_target.connect(map_path(src_port_path), map_path(dst_port_path));
}


void
ClashAvoider::disconnect(const Raul::Path& src_port_path,
                         const Raul::Path& dst_port_path)
{
	_target.disconnect(map_path(src_port_path), map_path(dst_port_path));
}


void
ClashAvoider::set_variable(const Raul::Path& subject_path,
                           const Raul::URI&  predicate,
                           const Raul::Atom& value)
{
	_target.set_variable(map_path(subject_path), predicate, value);
}


void
ClashAvoider::set_property(const Raul::Path& subject_path,
                           const Raul::URI&  predicate,
                           const Raul::Atom& value)
{
	_target.set_property(map_path(subject_path), predicate, value);
}


void
ClashAvoider::set_port_value(const Raul::Path& port_path,
                             const Raul::Atom& value)
{
	_target.set_port_value(map_path(port_path), value);
}


void
ClashAvoider::set_voice_value(const Raul::Path& port_path,
                              uint32_t          voice,
                              const Raul::Atom& value)
{
	_target.set_voice_value(map_path(port_path), voice, value);
}


void
ClashAvoider::destroy(const Raul::Path& path)
{
	_target.destroy(map_path(path));
}


void
ClashAvoider::clear_patch(const Raul::Path& path)
{
	_target.clear_patch(map_path(path));
}


} // namespace Shared
} // namespace Ingen
