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

#include "Connection.hpp"
#include "Port.hpp"

#include "ingen/client/EdgeModel.hpp"

using namespace std;

namespace Ingen {
namespace GUI {

Connection::Connection(Ganv::Canvas&                              canvas,
                       boost::shared_ptr<const Client::EdgeModel> model,
                       Ganv::Node*                                src,
                       Ganv::Node*                                dst,
                       uint32_t                                   color)
	: Ganv::Edge(canvas, src, dst, color)
	, _edge_model(model)
{
}

}   // namespace GUI
}   // namespace Ingen
