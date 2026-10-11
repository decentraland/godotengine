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

#pragma once

// VIBE-CODED SPIKE: C API for hosting Godot inside an app that owns UIApplicationMain
// (e.g. React Native). Build the engine with `embedded_host=yes` so the SwiftUI @main in
// app.swift is left out, then:
//   1. Call godot_host_init() once, early (before creating a GDTViewController).
//   2. Add a GDTViewController as a child view controller; it starts and drives the engine.
//   3. Forward the host's lifecycle with the godot_host_* functions below.
// The project .pck must be named after the host executable (<exec>.pck) and sit in the app
// bundle root, because --main-pack is compiled out of release templates.

#import <UIKit/UIKit.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Boots the engine with the given command line (argv[0] is the host executable). Idempotent.
/// Returns 0 on success; never exits the process.
int godot_host_init(int argc, const char *const *argv);

bool godot_host_is_hosted(void);

// Internal: registers the bridge singleton. Called by godot_host_init.
void godot_host_register_bridge(void);

void godot_host_focus_in(void);
void godot_host_focus_out(void);
void godot_host_enter_background(void);
void godot_host_exit_background(void);
void godot_host_memory_warning(void);
void godot_host_shutdown(void);

/// JSON bridge between the host app and the project's RnBridge autoload. The engine registers a
/// "RnHostPlugin" singleton (signal `rn_command(json)`, method `rn_event(json)`) in godot_host_init.
typedef void (*godot_host_event_callback)(const char *json);

/// Receives every `rn_event` call from the project. Invoked on the engine (main) thread.
void godot_host_set_event_callback(godot_host_event_callback callback);

/// Delivers a command to the project as the `rn_command` signal. Call on the main thread.
void godot_host_send_command(const char *json);

#ifdef __cplusplus
}
#endif
