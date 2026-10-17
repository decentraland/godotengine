/**************************************************************************/
/*  app_delegate_service.h                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#import "godot_host.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"
#include "core/object/object.h"

// VIBE-CODED SPIKE: Object exposed to GDScript as the "RnHostPlugin" singleton, mirroring the
// Android plugin of the same name so the project's rn_bridge.gd works unchanged.
class RnHostBridge : public Object {
	GDCLASS(RnHostBridge, Object);

	godot_host_event_callback callback = nullptr;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("rn_event", "json"), &RnHostBridge::rn_event);
		ADD_SIGNAL(MethodInfo("rn_command", PropertyInfo(Variant::STRING, "json")));
	}

public:
	void set_callback(godot_host_event_callback p_callback) { callback = p_callback; }

	void rn_event(const String &p_json) {
		if (callback) {
			callback(p_json.utf8().get_data());
		}
	}

	void send_command(const String &p_json) { emit_signal("rn_command", p_json); }
};

static RnHostBridge *host_bridge = nullptr;

// Called by godot_host_init once Main::setup has finished, before autoloads are created.
void godot_host_register_bridge(void) {
	if (host_bridge) {
		return;
	}
	GDREGISTER_CLASS(RnHostBridge);
	host_bridge = memnew(RnHostBridge);
	Engine::get_singleton()->add_singleton(Engine::Singleton("RnHostPlugin", host_bridge));
}

void godot_host_set_event_callback(godot_host_event_callback callback) {
	if (host_bridge) {
		host_bridge->set_callback(callback);
	}
}

void godot_host_send_command(const char *json) {
	if (host_bridge && json) {
		host_bridge->send_command(String::utf8(json));
	}
}
