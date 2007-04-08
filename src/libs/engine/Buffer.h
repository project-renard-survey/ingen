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

#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <cassert>
#include <boost/utility.hpp>
#include "types.h"
#include "DataType.h"

namespace Ingen {

	
class Buffer : public boost::noncopyable
{
public:
	Buffer(DataType type, size_t size)
		: _type(type)
		, _size(size)
	{}

	virtual ~Buffer() {}
	
	virtual void clear() = 0;
	virtual void prepare(SampleCount nframes) = 0;
	
	virtual bool is_joined() const = 0;
	virtual bool is_joined_to(Buffer* buf) const = 0;
	virtual bool join(Buffer* buf) = 0;
	virtual void unjoin() = 0;

	virtual void resize(size_t size) { _size = size; }

	DataType type() const { return _type; }
	size_t   size() const { return _size; }

protected:
	DataType _type;
	size_t   _size;
};


} // namespace Ingen

#endif // BUFFER_H
