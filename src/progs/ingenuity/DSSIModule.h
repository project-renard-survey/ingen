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

#ifndef DSSIMODULE_H
#define DSSIMODULE_H

#include "OmModule.h"

namespace OmGtk {

class DSSIController;

/* Module for a DSSI node.
 *
 * \ingroup OmGtk
 */
class DSSIModule : public OmModule
{
public:
	DSSIModule(OmFlowCanvas* canvas, DSSIController* node);
	virtual ~DSSIModule() {}

	void on_double_click(GdkEventButton* ev);
};


} // namespace OmGtk

#endif // DSSIMODULE_H

