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

#include <raul/PathTable.hpp>
#include <raul/TableImpl.hpp>
#include "Store.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


Store::const_iterator
Store::children_begin(SharedPtr<Shared::GraphObject> o) const
{
	const_iterator parent = find(o->path());
	assert(parent != end());
	++parent;
	return parent;
}


Store::const_iterator
Store::children_end(SharedPtr<Shared::GraphObject> o) const
{
	const_iterator parent = find(o->path());
	assert(parent != end());
	return find_descendants_end(parent);
}


SharedPtr<Shared::GraphObject>
Store::find_child(SharedPtr<Shared::GraphObject> parent, const string& child_name) const
{
	const_iterator pi = find(parent->path());
	assert(pi != end());
	const_iterator children_end = find_descendants_end(pi);
	const_iterator child = find(pi, children_end, parent->path().base() + child_name);
	if (child != end())
		return child->second;
	else
		return SharedPtr<Shared::GraphObject>();
}


} // namespace Shared
} // namespace Ingen