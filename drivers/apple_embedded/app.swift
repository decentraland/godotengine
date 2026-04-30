/**************************************************************************/
/*  app.swift                                                             */
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

import SwiftUI
import UIKit

struct GodotSwiftUIViewController: UIViewControllerRepresentable {

	func makeUIViewController(context: Context) -> GDTViewController {
		let viewController = GDTViewController()
		GDTAppDelegateService.viewController = viewController
		return viewController
	}

	func updateUIViewController(_ uiViewController: GDTViewController, context: Context) {
		// NOOP
	}

}

@main
struct SwiftUIApp: App {
	@UIApplicationDelegateAdaptor(GDTApplicationDelegate.self) var appDelegate

	var body: some Scene {
		WindowGroup {
			GodotSwiftUIViewController()
				.ignoresSafeArea()
				// SwiftUI WindowGroup wraps the scene with a SwiftUI-internal
				// scene delegate, so events delivered to
				// scene:continueUserActivity: / scene:openURLContexts: do not
				// reach the GDTApplicationDelegate configured via
				// application:configurationForConnectingSceneSession:options:.
				// Bridge them to the @UIApplicationDelegateAdaptor instance —
				// the real GDTApplicationDelegate — through the legacy
				// AppDelegate selectors, which iterate `services` and dispatch
				// to any registered plugin.
				.onContinueUserActivity(NSUserActivityTypeBrowsingWeb) { userActivity in
					_ = appDelegate.application(UIApplication.shared,
					                            continue: userActivity,
					                            restorationHandler: { _ in })
				}
				// iOS 17+ delivers HTTPS Universal Links through the openURL
				// pipeline rather than continueUserActivity, so this branch is
				// what actually fires for Universal Links in practice.
				.onOpenURL { url in
					_ = appDelegate.application(UIApplication.shared, open: url, options: [:])
				}
		}
	}
}
