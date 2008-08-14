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

#include <string>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <raul/Process.hpp>
#include CONFIG_H_PATH
#include "interface/EngineInterface.hpp"
#include "module/World.hpp"
#include "engine/tuning.hpp"
#include "engine/Engine.hpp"
#include "engine/QueuedEngineInterface.hpp"
#include "client/OSCClientReceiver.hpp"
#include "client/OSCEngineSender.hpp"
#include "client/ThreadedSigClientInterface.hpp"
#include "client/Store.hpp"
#include "client/PatchModel.hpp"
#include "module/Module.hpp"
#include "App.hpp"
#include "WindowFactory.hpp"
#include "ConnectWindow.hpp"
using Ingen::QueuedEngineInterface;
using Ingen::Client::ThreadedSigClientInterface;

namespace Ingen {
namespace GUI {


// Paste together some interfaces to get the combination we want


struct OSCSigEmitter : public OSCClientReceiver, public ThreadedSigClientInterface {
	OSCSigEmitter(size_t queue_size, int listen_port)
		: Ingen::Shared::ClientInterface()
		, OSCClientReceiver(listen_port)
		, ThreadedSigClientInterface(queue_size)
	{
	}
};


// ConnectWindow


ConnectWindow::ConnectWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::Dialog(cobject)
	, _mode(CONNECT_REMOTE)
	, _ping_id(-1)
	, _attached(false)
	, _connect_stage(0)
	, _new_engine(NULL)
{
	xml->get_widget("connect_icon",                 _icon);
	xml->get_widget("connect_progress_bar",         _progress_bar);
	xml->get_widget("connect_progress_label",       _progress_label);
	xml->get_widget("connect_server_radiobutton",   _server_radio);
	xml->get_widget("connect_url_entry",            _url_entry);
	xml->get_widget("connect_launch_radiobutton",   _launch_radio);
	xml->get_widget("connect_port_spinbutton",      _port_spinbutton);
	xml->get_widget("connect_internal_radiobutton", _internal_radio);
	xml->get_widget("connect_disconnect_button",    _disconnect_button);
	xml->get_widget("connect_connect_button",       _connect_button);
	xml->get_widget("connect_quit_button",          _quit_button);

	_server_radio->signal_toggled().connect(sigc::mem_fun(this, &ConnectWindow::server_toggled));
	_launch_radio->signal_toggled().connect(sigc::mem_fun(this, &ConnectWindow::launch_toggled));
	_internal_radio->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::internal_toggled));
	_disconnect_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::disconnect));
	_connect_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::connect));
	_quit_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::quit));

	_engine_module = Ingen::Shared::load_module("ingen_engine");

	if (!_engine_module) {
		cerr << "Unable to load ingen_engine module, internal engine unavailable." << endl;
		cerr << "If you are running from the source tree, run ingenuity_dev." << endl;
	}

	bool found = _engine_module->get_symbol("new_engine", (void*&)_new_engine);

	if (!found) {
		cerr << "Unable to find module entry point, internal engine unavailable." << endl;
		_engine_module.reset();
	}

    server_toggled();
}


void
ConnectWindow::start(Ingen::Shared::World* world)
{
	if (world->local_engine) {
		_mode = INTERNAL;
		_internal_radio->set_active(true);
	}
	
	set_connected_to(world->engine);

	show();
	connect();
}


void
ConnectWindow::set_connected_to(SharedPtr<Shared::EngineInterface> engine)
{
	if (engine) {
		_icon->set(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		_progress_bar->set_fraction(1.0);
		_url_entry->set_sensitive(false);
		_connect_button->set_sensitive(false);
		_disconnect_button->set_label("gtk-disconnect");
		_disconnect_button->set_sensitive(true);
		_port_spinbutton->set_sensitive(false);
		_launch_radio->set_sensitive(false);
		_internal_radio->set_sensitive(false);
	} else {
		_icon->set(Gtk::Stock::DISCONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		_progress_bar->set_fraction(0.0);
		_connect_button->set_sensitive(true);
		_disconnect_button->set_sensitive(false);

		if (_new_engine)
			_internal_radio->set_sensitive(true);
		else
			_internal_radio->set_sensitive(false);

        _server_radio->set_sensitive(true);
        _launch_radio->set_sensitive(true);

        if ( _mode == CONNECT_REMOTE )
            _url_entry->set_sensitive(true);
        else if ( _mode == LAUNCH_REMOTE )
            _port_spinbutton->set_sensitive(true);

		_progress_label->set_text(string("Disconnected"));
	}

	App::instance().world()->engine = engine;
}


/** Launch (if applicable) and connect to the Engine.
 *
 * This will create the EngineInterface and ClientInterface and initialize
 * the App with them.
 */
void
ConnectWindow::connect()
{
	assert(!_attached);
	assert(!App::instance().client());

	_connect_button->set_sensitive(false);
	_disconnect_button->set_label("gtk-cancel");
	_disconnect_button->set_sensitive(true);

    // Avoid user messing with our parameters whilst we're trying to connect.
    _server_radio->set_sensitive(false);
    _launch_radio->set_sensitive(false);
    _internal_radio->set_sensitive(false);
    _url_entry->set_sensitive(false);
    _port_spinbutton->set_sensitive(false);

	_connect_stage = 0;
		
	Ingen::Shared::World* world = App::instance().world();

	if (_mode == CONNECT_REMOTE) {
		world->engine = SharedPtr<EngineInterface>(
				new OSCEngineSender(_url_entry->get_text()));

		OSCSigEmitter* ose = new OSCSigEmitter(1024, 16181); // FIXME: args
		SharedPtr<ThreadedSigClientInterface> client(ose);
		App::instance().attach(client);
		
		Glib::signal_timeout().connect(
			sigc::mem_fun(App::instance(), &App::gtk_main_iteration), 40, G_PRIORITY_DEFAULT);

		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);

	} else if (_mode == LAUNCH_REMOTE) {

		int port = _port_spinbutton->get_value_as_int();
		char port_str[6];
		snprintf(port_str, 6, "%u", port);
		const string cmd = string("ingen -e --engine-port=").append(port_str);

		if (Raul::Process::launch(cmd)) {
			world->engine = SharedPtr<EngineInterface>(
					new OSCEngineSender(string("osc.udp://localhost:").append(port_str)));

			OSCSigEmitter* ose = new OSCSigEmitter(1024, 16181); // FIXME: args
			SharedPtr<ThreadedSigClientInterface> client(ose);
			App::instance().attach(client);

			Glib::signal_timeout().connect(
					sigc::mem_fun(App::instance(), &App::gtk_main_iteration), 40, G_PRIORITY_DEFAULT);

			Glib::signal_timeout().connect(
					sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);

		} else {
			cerr << "Failed to launch ingen process." << endl;
		}

	} else if (_mode == INTERNAL) {
		Ingen::Shared::World* world = App::instance().world();
		assert(_new_engine);
		if ( ! world->local_engine)
			world->local_engine = SharedPtr<Engine>(_new_engine(world));
		
		if ( ! world->engine)
			world->engine = world->local_engine->new_queued_interface();
		
		SharedPtr<SigClientInterface> client(new SigClientInterface());
		
		world->local_engine->start_jack_driver();
		world->local_engine->activate(1); // FIXME: parallelism
		
		App::instance().attach(client);

		Glib::signal_timeout().connect(
			sigc::mem_fun(App::instance(), &App::gtk_main_iteration), 40, G_PRIORITY_DEFAULT);
		
		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);
	}
}


void
ConnectWindow::disconnect()
{
	_connect_stage = -1;
	_attached = false;

	_progress_bar->set_fraction(0.0);
	_connect_button->set_sensitive(false);
	_disconnect_button->set_sensitive(false);

	App::instance().detach();
	set_connected_to(SharedPtr<Ingen::Shared::EngineInterface>());
	
	_connect_button->set_sensitive(true);
	_disconnect_button->set_sensitive(false);
}


void
ConnectWindow::quit()
{
	if (_attached) {
		Gtk::MessageDialog d(*this, "This will exit the GUI, but the engine will "
			"remain running (if it is remote).\n\nAre you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
			d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			d.add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		int ret = d.run();
		if (ret == Gtk::RESPONSE_CLOSE)
			Gtk::Main::quit();
	} else {
		Gtk::Main::quit();
	}
}


void
ConnectWindow::server_toggled()
{
	_url_entry->set_sensitive(true);
	_port_spinbutton->set_sensitive(false);
	_mode = CONNECT_REMOTE;
}


void
ConnectWindow::launch_toggled()
{
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(true);
	_mode = LAUNCH_REMOTE;
}


void
ConnectWindow::internal_toggled()
{
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(false);
	_mode = INTERNAL;
}


bool
ConnectWindow::gtk_callback()
{
	/* If I call this a "state machine" it's not ugly code any more */
	
	// Timing stuff for repeated attach attempts
	timeval now;
	gettimeofday(&now, NULL);
	static timeval last = now;
	
	/* Connecting to engine */
	if (_connect_stage == 0) {

		_attached = false;

		assert(App::instance().engine());
		assert(App::instance().client());

		_progress_label->set_text("Connecting to engine...");
		present();

		App::instance().client()->signal_response_ok.connect(
				sigc::mem_fun(this, &ConnectWindow::response_ok_received));
		
		_ping_id = rand();
		while (_ping_id == -1)
			_ping_id = rand();

		App::instance().engine()->set_next_response_id(_ping_id);
		App::instance().engine()->ping();
		++_connect_stage;


	} else if (_connect_stage == 1) {
		if (_attached) {
			App::instance().engine()->activate();
			++_connect_stage;
		} else {
			const float ms_since_last = (now.tv_sec - last.tv_sec) * 1000.0f +
				(now.tv_usec - last.tv_usec) * 0.001f;
			if (ms_since_last > 1000) {
				App::instance().engine()->set_next_response_id(_ping_id);
				App::instance().engine()->ping();
				last = now;
			}
		}
	} else if (_connect_stage == 2) {
		_progress_label->set_text(string("Requesting root patch..."));
		App::instance().engine()->request_all_objects();
		++_connect_stage;
	} else if (_connect_stage == 3) {
		if (App::instance().store()->objects().size() > 0) {
			SharedPtr<PatchModel> root = PtrCast<PatchModel>(App::instance().store()->object("/"));
			if (root) {
				set_connected_to(App::instance().engine());
				App::instance().window_factory()->present_patch(root);
				++_connect_stage;
			}
		}
	} else if (_connect_stage == 4) {
		_progress_label->set_text(string("Loading plugins..."));
		App::instance().engine()->load_plugins();
		++_connect_stage;
	} else if (_connect_stage == 5) {
		App::instance().engine()->request_plugins();
		++_connect_stage;
	} else if (_connect_stage == 6) {
		_progress_label->set_text("Connected to engine");
		_connect_stage = 0; // set ourselves up for next time (if there is one)
		hide();
		return false; // deregister this callback
	}
	
	_progress_bar->pulse();
	
	if (_connect_stage == -1) { // we were cancelled
		_icon->set(Gtk::Stock::DISCONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		_progress_bar->set_fraction(0.0);
		_connect_button->set_sensitive(true);
		_disconnect_button->set_sensitive(false);
		_disconnect_button->set_label("gtk-disconnect");
		_progress_label->set_text(string("Disconnected"));
		return false;
	} else {
		return true;
	}
}


} // namespace GUI
} // namespace Ingen
