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

#include CONFIG_H_PATH
#include "module/global.hpp"
#include "module/World.hpp"

#include <cassert>
#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Ellipse.hpp>
#include "interface/EngineInterface.hpp"
#include "shared/Builder.hpp"
#include "shared/ClashAvoider.hpp"
#include "serialisation/Serialiser.hpp"
#include "client/PluginModel.hpp"
#include "client/PatchModel.hpp"
#include "client/NodeModel.hpp"
#include "client/ClientStore.hpp"
#include "App.hpp"
#include "PatchCanvas.hpp"
#include "PatchWindow.hpp"
#include "PatchPortModule.hpp"
#include "LoadPluginWindow.hpp"
#include "LoadSubpatchWindow.hpp"
#include "NewSubpatchWindow.hpp"
#include "Port.hpp"
#include "Connection.hpp"
#include "NodeModule.hpp"
#include "SubpatchModule.hpp"
#include "GladeFactory.hpp"
#include "WindowFactory.hpp"
#include "ThreadedLoader.hpp"
using Ingen::Client::ClientStore;
using Ingen::Serialisation::Serialiser;
using Ingen::Client::PluginModel;

namespace Ingen {
namespace GUI {


PatchCanvas::PatchCanvas(SharedPtr<PatchModel> patch, int width, int height)
	: Canvas(width, height)
	, _patch(patch)
	, _last_click_x(0)
	, _last_click_y(0)
	, _refresh_menu(false)
	, _menu(NULL)
	, _internal_menu(NULL)
	, _plugin_menu(NULL)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget("canvas_menu", _menu);
	
	/*xml->get_widget("canvas_menu_add_number_control", _menu_add_number_control);
	xml->get_widget("canvas_menu_add_button_control", _menu_add_button_control);*/
	xml->get_widget("canvas_menu_add_audio_input", _menu_add_audio_input);
	xml->get_widget("canvas_menu_add_audio_output", _menu_add_audio_output);
	xml->get_widget("canvas_menu_add_control_input", _menu_add_control_input);
	xml->get_widget("canvas_menu_add_control_output", _menu_add_control_output);
	xml->get_widget("canvas_menu_add_midi_input", _menu_add_midi_input);
	xml->get_widget("canvas_menu_add_midi_output", _menu_add_midi_output);
	xml->get_widget("canvas_menu_add_osc_input", _menu_add_osc_input);
	xml->get_widget("canvas_menu_add_osc_output", _menu_add_osc_output);
	xml->get_widget("canvas_menu_add_event_input", _menu_add_event_input);
	xml->get_widget("canvas_menu_add_event_output", _menu_add_event_output);
	xml->get_widget("canvas_menu_load_plugin", _menu_load_plugin);
	xml->get_widget("canvas_menu_load_patch", _menu_load_patch);
	xml->get_widget("canvas_menu_new_patch", _menu_new_patch);

	// Add port menu items
	_menu_add_audio_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"audio_input", "ingen:AudioPort", false));
	_menu_add_audio_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"audio_output", "ingen:AudioPort", true));
	_menu_add_control_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"control_input", "ingen:ControlPort", false));
	_menu_add_control_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"control_output", "ingen:ControlPort", true));
	_menu_add_midi_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"midi_input", "ingen:MIDIPort", false));
	_menu_add_midi_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"midi_output", "ingen:MIDIPort", true));
	_menu_add_osc_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"osc_input", "ingen:OSCPort", false));
	_menu_add_osc_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"osc_output", "ingen:OSCPort", true));
	_menu_add_event_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"event_input", "ingen:EventPort", false));
	_menu_add_event_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"event_output", "ingen:EventPort", true));
	
	// Add control menu items
	/*_menu_add_number_control->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_control), NUMBER));
	_menu_add_button_control->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_control), BUTTON));*/

	// Connect to model signals to track state
	_patch->signal_new_node.connect(sigc::mem_fun(this, &PatchCanvas::add_node));
	_patch->signal_removed_node.connect(sigc::mem_fun(this, &PatchCanvas::remove_node));
	_patch->signal_new_port.connect(sigc::mem_fun(this, &PatchCanvas::add_port));
	_patch->signal_removed_port.connect(sigc::mem_fun(this, &PatchCanvas::remove_port));
	_patch->signal_new_connection.connect(sigc::mem_fun(this, &PatchCanvas::connection));
	_patch->signal_removed_connection.connect(sigc::mem_fun(this, &PatchCanvas::disconnection));
	
	App::instance().store()->signal_new_plugin.connect(sigc::mem_fun(this, &PatchCanvas::add_plugin));
	
	// Connect widget signals to do things
	_menu_load_plugin->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_load_plugin));
	_menu_load_patch->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_load_patch));
	_menu_new_patch->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_new_patch));
}


void
PatchCanvas::show_menu(GdkEvent* event)
{
	if (!_internal_menu || !_plugin_menu || _refresh_menu) {
		build_internal_menu();
#ifdef HAVE_SLV2
		build_plugin_menu();
#endif
		_refresh_menu = false;
	}
	_menu->popup(event->button.button, event->button.time);
}


void
PatchCanvas::build_internal_menu()
{
	if (_internal_menu) {
		_internal_menu->items().clear();
	} else {
		_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem("Internal",
				*(manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU)))));
		Gtk::MenuItem* internal_menu_item = &(_menu->items().back());
		_internal_menu = Gtk::manage(new Gtk::Menu());
		internal_menu_item->set_submenu(*_internal_menu);
		_menu->reorder_child(*internal_menu_item, 2);
	}
	
	SharedPtr<const ClientStore::Plugins> plugins = App::instance().store()->plugins();

	// Add Internal plugins
	for (ClientStore::Plugins::const_iterator i = plugins->begin(); i != plugins->end(); ++i) {
		SharedPtr<PluginModel> p = i->second;
		if (p->type() == Plugin::Internal) {
			_internal_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(p->name(),
					sigc::bind(sigc::mem_fun(this, &PatchCanvas::load_plugin), p)));
		}
	}
}


#ifdef HAVE_SLV2
size_t
PatchCanvas::build_plugin_class_menu(Gtk::Menu* menu,
		SLV2PluginClass plugin_class, SLV2PluginClasses classes)
{
	size_t num_items = 0;

	// Add submenus
	for (unsigned i=0; i < slv2_plugin_classes_size(classes); ++i) {
		SLV2PluginClass c = slv2_plugin_classes_get_at(classes, i);
		SLV2Value parent = slv2_plugin_class_get_parent_uri(c);

		if (parent && slv2_value_equals(parent, slv2_plugin_class_get_uri(plugin_class))) {
			Gtk::Menu_Helpers::MenuElem menu_elem = Gtk::Menu_Helpers::MenuElem(
					slv2_value_as_string(slv2_plugin_class_get_label(c)));

			Gtk::Menu* submenu = Gtk::manage(new Gtk::Menu());
			size_t sub_num_items = build_plugin_class_menu(submenu, c, classes);

			if (sub_num_items > 0) {
				menu->items().push_back(menu_elem);
				Gtk::MenuItem* menu_item = &(menu->items().back());
				menu_item->set_submenu(*submenu);
				++num_items;
			}
		}
	}
	
	SharedPtr<const ClientStore::Plugins> plugins = App::instance().store()->plugins();

	// Add LV2 plugins
	for (ClientStore::Plugins::const_iterator i = plugins->begin(); i != plugins->end(); ++i) {
		SLV2Plugin p = i->second->slv2_plugin();

		if (p && slv2_plugin_get_class(p) == plugin_class) {
			Glib::RefPtr<Gdk::Pixbuf> icon
					= App::instance().icon_from_path(PluginModel::get_lv2_icon_path(p), 16);
			if (icon) {
				Gtk::Image* image = new Gtk::Image(icon);
				menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(i->second->name(),
							*image,
							sigc::bind(sigc::mem_fun(this, &PatchCanvas::load_plugin), i->second)));
			} else {
				menu->items().push_back(Gtk::Menu_Helpers::MenuElem(i->second->name(),
							sigc::bind(sigc::mem_fun(this, &PatchCanvas::load_plugin), i->second)));
				++num_items;
			}
		}
	}

	return num_items;
}


void
PatchCanvas::build_plugin_menu()
{
	if (_plugin_menu) {
		_plugin_menu->items().clear();
	} else {
		_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem("Plugin",
				*(manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU)))));
		Gtk::MenuItem* plugin_menu_item = &(_menu->items().back());
		_plugin_menu = Gtk::manage(new Gtk::Menu());
		plugin_menu_item->set_submenu(*_plugin_menu);
		_menu->reorder_child(*plugin_menu_item, 3);
	}

	Glib::Mutex::Lock lock(PluginModel::rdf_world()->mutex());
	SLV2PluginClass lv2_plugin = slv2_world_get_plugin_class(PluginModel::slv2_world());
	SLV2PluginClasses classes = slv2_world_get_plugin_classes(PluginModel::slv2_world());

	build_plugin_class_menu(_plugin_menu, lv2_plugin, classes);
}
#endif


void
PatchCanvas::build()
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());
	
	// Create modules for nodes
	for (ObjectModel::const_iterator i = App::instance().store()->children_begin(_patch);
			i != App::instance().store()->children_end(_patch); ++i) {
		SharedPtr<NodeModel> node = PtrCast<NodeModel>(i->second);
		if (node && node->parent() == _patch)
			add_node(node);
	}

	// Create pseudo modules for ports (ports on this canvas, not on our module)
	for (PortModelList::const_iterator i = _patch->ports().begin();
			i != _patch->ports().end(); ++i) {
		add_port(*i);
	}

	// Create connections
	for (PatchModel::Connections::const_iterator i = _patch->connections().begin();
			i != _patch->connections().end(); ++i) {
		connection(PtrCast<ConnectionModel>(*i));
	}
}

	
void
PatchCanvas::arrange(bool ingen_doesnt_use_length_hints)
{
	FlowCanvas::Canvas::arrange(false);
	
	for (list<boost::shared_ptr<Item> >::iterator i = _items.begin(); i != _items.end(); ++i)
		(*i)->store_location();
}

	
void
PatchCanvas::add_plugin(SharedPtr<PluginModel> pm)
{
	_refresh_menu = true;
}


void
PatchCanvas::add_node(SharedPtr<NodeModel> nm)
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());

	SharedPtr<PatchModel> pm = PtrCast<PatchModel>(nm);
	SharedPtr<NodeModule> module;
	if (pm) {
		module = SubpatchModule::create(shared_this, pm);
	} else {
		module = NodeModule::create(shared_this, nm);
		const PluginModel* plugm = dynamic_cast<const PluginModel*>(nm->plugin());
		if (plugm && plugm->icon_path() != "")
			module->set_icon(App::instance().icon_from_path(plugm->icon_path(), 100));
	}
	
	add_item(module);
	module->show();
	_views.insert(std::make_pair(nm, module));
}


void
PatchCanvas::remove_node(SharedPtr<NodeModel> nm)
{
	Views::iterator i = _views.find(nm);

	if (i != _views.end()) {
		remove_item(i->second);
		_views.erase(i);
	}
}


void
PatchCanvas::add_port(SharedPtr<PortModel> pm)
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());

	SharedPtr<PatchPortModule> view = PatchPortModule::create(shared_this, pm);
	_views.insert(std::make_pair(pm, view));
	add_item(view);
	view->show();
}


void
PatchCanvas::remove_port(SharedPtr<PortModel> pm)
{
	Views::iterator i = _views.find(pm);

	if (i != _views.end()) {
		remove_item(i->second);
		_views.erase(i);
	}
}


SharedPtr<FlowCanvas::Port>
PatchCanvas::get_port_view(SharedPtr<PortModel> port)
{
	SharedPtr<FlowCanvas::Port> ret;
	SharedPtr<FlowCanvas::Module> module = _views[port];
	
	// Port on this patch
	if (module) {
		ret = (PtrCast<PatchPortModule>(module))
			? *(PtrCast<PatchPortModule>(module)->ports().begin())
			: PtrCast<FlowCanvas::Port>(module);
	} else {
		module = PtrCast<NodeModule>(_views[port->parent()]);
		if (module)
			ret = module->get_port(port->path().name());
	}
	
	return ret;
}


void
PatchCanvas::connection(SharedPtr<ConnectionModel> cm)
{
	assert(cm);
	
	const SharedPtr<FlowCanvas::Port> src = get_port_view(cm->src_port());
	const SharedPtr<FlowCanvas::Port> dst = get_port_view(cm->dst_port());

	if (src && dst) {
		add_connection(boost::shared_ptr<GUI::Connection>(new GUI::Connection(shared_from_this(),
				cm, src, dst, src->color() + 0x22222200)));
	} else {
		cerr << "[PatchCanvas] ERROR: Unable to find ports to connect "
			<< cm->src_port_path() << " -> " << cm->dst_port_path() << endl;
	}
}


void
PatchCanvas::disconnection(SharedPtr<ConnectionModel> cm)
{
	const SharedPtr<FlowCanvas::Port> src = get_port_view(cm->src_port());
	const SharedPtr<FlowCanvas::Port> dst = get_port_view(cm->dst_port());
		
	if (src && dst)
		remove_connection(src, dst);
	else
		cerr << "[PatchCanvas] ERROR: Unable to find ports to disconnect "
			<< cm->src_port_path() << " -> " << cm->dst_port_path() << endl;
}


void
PatchCanvas::connect(boost::shared_ptr<FlowCanvas::Connectable> src_port,
                     boost::shared_ptr<FlowCanvas::Connectable> dst_port)
{
	const boost::shared_ptr<Ingen::GUI::Port> src
		= boost::dynamic_pointer_cast<Ingen::GUI::Port>(src_port);

	const boost::shared_ptr<Ingen::GUI::Port> dst
		= boost::dynamic_pointer_cast<Ingen::GUI::Port>(dst_port);
	
	if (!src || !dst)
		return;

	// Midi binding/learn shortcut
	if (src->model()->type().is_event() && dst->model()->type().is_control())
	{
		cerr << "[PatchCanvas] FIXME: MIDI binding shortcut" << endl;
#if 0
		SharedPtr<PluginModel> pm(new PluginModel(PluginModel::Internal, "", "midi_control_in", ""));
		SharedPtr<NodeModel> nm(new NodeModel(pm, _patch->path().base()
			+ src->name() + "-" + dst->name(), false));
		nm->set_variable("canvas-x", Atom((float)
			(dst->module()->property_x() - dst->module()->width() - 20)));
		nm->set_variable("canvas-y", Atom((float)
			(dst->module()->property_y())));
		App::instance().engine()->create_node_from_model(nm.get());
		App::instance().engine()->connect(src->model()->path(), nm->path() + "/MIDI_In");
		App::instance().engine()->connect(nm->path() + "/Out_(CR)", dst->model()->path());
		App::instance().engine()->midi_learn(nm->path());
		
		// Set control node range to port's user range
		
		App::instance().engine()->set_port_value_queued(nm->path().base() + "Min",
			dst->model()->get_variable("user-min").get_float());
		App::instance().engine()->set_port_value_queued(nm->path().base() + "Max",
			dst->model()->get_variable("user-max").get_float());
#endif
	} else {
		App::instance().engine()->connect(src->model()->path(), dst->model()->path());
	}
}


void
PatchCanvas::disconnect(boost::shared_ptr<FlowCanvas::Connectable> src_port,
                        boost::shared_ptr<FlowCanvas::Connectable> dst_port)
{
	const boost::shared_ptr<Ingen::GUI::Port> src
		= boost::dynamic_pointer_cast<Ingen::GUI::Port>(src_port);

	const boost::shared_ptr<Ingen::GUI::Port> dst
		= boost::dynamic_pointer_cast<Ingen::GUI::Port>(dst_port);
	
	App::instance().engine()->disconnect(src->model()->path(),
	                                     dst->model()->path());
}


bool
PatchCanvas::canvas_event(GdkEvent* event)
{
	assert(event);
	
	bool ret = false;

	switch (event->type) {

	case GDK_BUTTON_PRESS:
		if (event->button.button == 3) {
			_last_click_x = (int)event->button.x;
			_last_click_y = (int)event->button.y;
			show_menu(event);
			ret = true;
		}
		break;
	
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		ret = canvas_key_event(&event->key);
		
	default:
		break;
	}

	return (ret ? true : Canvas::canvas_event(event));
}

	
bool
PatchCanvas::canvas_key_event(GdkEventKey* event)
{
	switch (event->type) {
	case GDK_KEY_PRESS:
		switch (event->keyval) {
		case GDK_Delete:
			destroy_selection();
			return true;
		case GDK_e:
			if (_patch->get_editable() == true)
				_patch->set_editable(false);
			else
				_patch->set_editable(true);
			return true;
		default:
			return false;
		}
	default:
		return false;
	}
}


void
PatchCanvas::destroy_selection()
{
	for (list<boost::shared_ptr<Item> >::iterator m = _selected_items.begin(); m != _selected_items.end(); ++m) {
		boost::shared_ptr<NodeModule> module = boost::dynamic_pointer_cast<NodeModule>(*m);
		if (module) {
			App::instance().engine()->destroy(module->node()->path());
		} else {
			boost::shared_ptr<PatchPortModule> port_module = boost::dynamic_pointer_cast<PatchPortModule>(*m);
			if (port_module)
				App::instance().engine()->destroy(port_module->port()->path());
		}
	}

}


void
PatchCanvas::copy_selection()
{
	Serialiser serialiser(*App::instance().world(), App::instance().store());
	serialiser.start_to_string(_patch->path(), "http://example.org/");

	for (list<boost::shared_ptr<Item> >::iterator m = _selected_items.begin(); m != _selected_items.end(); ++m) {
		boost::shared_ptr<NodeModule> module = boost::dynamic_pointer_cast<NodeModule>(*m);
		if (module) {
			serialiser.serialise(module->node());
		} else {
			boost::shared_ptr<PatchPortModule> port_module = boost::dynamic_pointer_cast<PatchPortModule>(*m);
			if (port_module)
				serialiser.serialise(port_module->port());
		}
	}
	
	for (list<boost::shared_ptr<FlowCanvas::Connection> >::iterator c = _selected_connections.begin();
			c != _selected_connections.end(); ++c) {
		boost::shared_ptr<Connection> connection = boost::dynamic_pointer_cast<Connection>(*c);
		if (connection)
			serialiser.serialise_connection(_patch, connection->model());
	}
	
	string result = serialiser.finish();
	
	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	clipboard->set_text(result);
}

	
void
PatchCanvas::paste()
{
	Glib::ustring str = Gtk::Clipboard::get()->wait_for_text();
	SharedPtr<Parser> parser = App::instance().loader()->parser();
	if (!parser) {
		cerr << "Unable to load parser, paste unavailable" << endl;
		return;
	}

	clear_selection();

	Builder builder(*App::instance().engine());
	ClientStore clipboard;
	clipboard.set_plugins(App::instance().store()->plugins());
	clipboard.new_patch("/", _patch->poly());
	
	ClashAvoider avoider(*App::instance().store().get(), _patch->path(), clipboard, &clipboard);
	parser->parse_string(App::instance().world(), &avoider, str, "/",
			boost::optional<Glib::ustring>(), (Glib::ustring)_patch->path());
	
	for (Store::iterator i = clipboard.begin(); i != clipboard.end(); ++i) {
		cout << "************ OBJECT: " << i->first << endl;
		if (_patch->path() == "/" && i->first == "/") {
			//cout << "SKIPPING ROOT " << _patch->path() << " :: " << i->first << endl;
			continue;
		} else if (i->first.parent() != "/") {
			//cout << "SKIPPING NON ROOTED OBJECT " << i->first << endl;
			continue;
		}
		GraphObject::Variables::iterator x = i->second->variables().find("ingenuity:canvas-x");
		if (x != i->second->variables().end())
			x->second = x->second.get_float() + 20.0f;
		GraphObject::Variables::iterator y = i->second->variables().find("ingenuity:canvas-y");
		if (y != i->second->variables().end())
			y->second = y->second.get_float() + 20.0f;
		if (i->first.parent() == "/") {
			GraphObject::Properties::iterator s = i->second->properties().find("ingen:selected");
			if (s != i->second->properties().end())
				s->second = true;
			else
				i->second->properties().insert(make_pair("ingen:selected", true));
		}
		builder.build(_patch->path(), i->second);
	}

	//avoider.set_target(*App::instance().engine());

	for (ClientStore::ConnectionRecords::const_iterator i = clipboard.connection_records().begin();
			i != clipboard.connection_records().end(); ++i) {
		cout << "CONNECTING " << i->first << " -> " << i->second << endl;
		App::instance().engine()->connect(i->first, i->second);
	}
}
	

string
PatchCanvas::generate_port_name(const string& base)
{
	string name = base;

	char num_buf[5];
	for (uint i=1; i < 9999; ++i) {
		snprintf(num_buf, 5, "%u", i);
		name = base + "_";
		name += num_buf;
		if (!_patch->get_port(name))
			break;
	}

	assert(Path::is_valid(string("/") + name));

	return name;
}

void
PatchCanvas::menu_add_control(ControlType type)
{
	// FIXME: bundleify

	GraphObject::Variables data = get_initial_data();
	float x = data["ingenuity:canvas-x"].get_float();
	float y = data["ingenuity:canvas-y"].get_float();
	
	cerr << "ADD CONTROL: " << (unsigned)type << " @ " << x << ", " << y << endl;

	add_item(boost::shared_ptr<FlowCanvas::Item>(
			new FlowCanvas::Ellipse(shared_from_this(), "control", x, y, 20, 20, true)));
}

void
PatchCanvas::menu_add_port(const string& name, const string& type, bool is_output)
{
	const Path& path = _patch->path().base() + generate_port_name(name);
	App::instance().engine()->bundle_begin();
	App::instance().engine()->new_port(path, _patch->num_ports(), type, is_output);
	GraphObject::Variables data = get_initial_data();
	for (GraphObject::Variables::const_iterator i = data.begin(); i != data.end(); ++i)
		App::instance().engine()->set_variable(path, i->first, i->second);
	App::instance().engine()->bundle_end();
}


void
PatchCanvas::load_plugin(SharedPtr<PluginModel> plugin)
{
	string name = plugin->default_node_name();
	unsigned offset = App::instance().store()->child_name_offset(_patch->path(), name);
	if (offset != 0) {
		std::stringstream ss;
		ss << name << "_" << offset;
		name = ss.str();
	}
		
	const Path path = _patch->path().base() + name;
	// FIXME: polyphony?
	App::instance().engine()->new_node(path, plugin->uri());
	GraphObject::Variables data = get_initial_data();
	for (GraphObject::Variables::const_iterator i = data.begin(); i != data.end(); ++i)
		App::instance().engine()->set_variable(path, i->first, i->second);
}


/** Try to guess a suitable location for a new module.
 */
void
PatchCanvas::get_new_module_location(double& x, double& y)
{
	int scroll_x;
	int scroll_y;
	get_scroll_offsets(scroll_x, scroll_y);
	x = scroll_x + 20;
	y = scroll_y + 20;
}


GraphObject::Variables
PatchCanvas::get_initial_data()
{
	GraphObject::Variables result;
	
	result["ingenuity:canvas-x"] = Atom((float)_last_click_x);
	result["ingenuity:canvas-y"] = Atom((float)_last_click_y);
	
	return result;
}

void
PatchCanvas::menu_load_plugin()
{
	App::instance().window_factory()->present_load_plugin(_patch, get_initial_data());
}


void
PatchCanvas::menu_load_patch()
{
	App::instance().window_factory()->present_load_subpatch(_patch, get_initial_data());
}


void
PatchCanvas::menu_new_patch()
{
	App::instance().window_factory()->present_new_subpatch(_patch, get_initial_data());
}


} // namespace GUI
} // namespace Ingen
