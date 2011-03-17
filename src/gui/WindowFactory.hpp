/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_WINDOWFACTORY_HPP
#define INGEN_GUI_WINDOWFACTORY_HPP

#include <map>
#include <gtkmm.h>
#include "raul/SharedPtr.hpp"
#include "interface/GraphObject.hpp"

using namespace Ingen::Shared;

namespace Raul { class Path; }

namespace Ingen {

namespace Client { class PatchModel; class NodeModel; class ObjectModel; }
using Ingen::Client::PatchModel;
using Ingen::Client::NodeModel;
using Ingen::Client::ObjectModel;

namespace GUI {

class LoadPatchWindow;
class LoadPluginWindow;
class LoadRemotePatchWindow;
class NewSubpatchWindow;
class NodeControlWindow;
class PropertiesWindow;
class PatchView;
class PatchWindow;
class RenameWindow;
class UploadPatchWindow;


/** Manager/Factory for all windows.
 *
 * This serves as a nice centralized spot for all window management issues,
 * as well as an enumeration of all windows (the goal being to reduce that
 * number as much as possible).
 */
class WindowFactory {
public:
	WindowFactory();
	~WindowFactory();

	size_t num_open_patch_windows();

	PatchWindow*       patch_window(SharedPtr<PatchModel> patch);
	PatchWindow*       parent_patch_window(SharedPtr<NodeModel> node);
	NodeControlWindow* control_window(SharedPtr<NodeModel> node);

	void present_patch(SharedPtr<PatchModel> model,
	                   PatchWindow*          preferred = NULL,
	                   SharedPtr<PatchView>  view      = SharedPtr<PatchView>());

	void present_controls(SharedPtr<NodeModel> node);

	typedef GraphObject::Properties Properties;

	void present_load_plugin(SharedPtr<PatchModel> patch, Properties data=Properties());
	void present_load_patch(SharedPtr<PatchModel> patch, Properties data=Properties());
	void present_load_subpatch(SharedPtr<PatchModel> patch, Properties data=Properties());
	void present_load_remote_patch(SharedPtr<PatchModel> patch, Properties data=Properties());
	void present_upload_patch(SharedPtr<PatchModel> patch);
	void present_new_subpatch(SharedPtr<PatchModel> patch, Properties data=Properties());
	void present_rename(SharedPtr<ObjectModel> object);
	void present_properties(SharedPtr<ObjectModel> object);

	bool remove_patch_window(PatchWindow* win, GdkEventAny* ignored = NULL);

	void clear();

private:
	typedef std::map<Raul::Path, PatchWindow*>       PatchWindowMap;
	typedef std::map<Raul::Path, NodeControlWindow*> ControlWindowMap;

	PatchWindow* new_patch_window(SharedPtr<PatchModel> patch, SharedPtr<PatchView> view);


	NodeControlWindow* new_control_window(SharedPtr<NodeModel> node);
	bool               remove_control_window(NodeControlWindow* win, GdkEventAny* ignored);

	PatchWindowMap   _patch_windows;
	ControlWindowMap _control_windows;

	LoadPluginWindow*      _load_plugin_win;
	LoadPatchWindow*       _load_patch_win;
	LoadRemotePatchWindow* _load_remote_patch_win;
	UploadPatchWindow*     _upload_patch_win;
	NewSubpatchWindow*     _new_subpatch_win;
	PropertiesWindow*      _properties_win;
	RenameWindow*          _rename_win;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_WINDOWFACTORY_HPP
