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
				// HTTPS Universal Links: in a SwiftUI WindowGroup, the
				// UIWindowScene's delegate is a SwiftUI-internal proxy, not
				// GDTApplicationDelegate, even when
				// application:configurationForConnectingSceneSession:options:
				// returns delegateClass = [GDTApplicationDelegate class]. So
				// `scene.delegate as? GDTApplicationDelegate` always fails
				// here. Forward the activity to the real GDTApplicationDelegate
				// (the @UIApplicationDelegateAdaptor instance) via the legacy
				// application:continueUserActivity:restorationHandler:, which
				// iterates `services` exactly like scene:continueUserActivity:
				// would have.
				.onContinueUserActivity(NSUserActivityTypeBrowsingWeb) { userActivity in
					NSLog("[DEEPLINK] SwiftUI .onContinueUserActivity fired, activityType=%@ webpageURL=%@",
					      userActivity.activityType, userActivity.webpageURL?.absoluteString ?? "nil")

					let scenes = UIApplication.shared.connectedScenes
					NSLog("[DEEPLINK] connectedScenes count=%lu", UInt(scenes.count))
					for s in scenes {
						NSLog("[DEEPLINK]   scene=%@ delegateClass=%@ activationState=%ld",
						      String(describing: type(of: s)),
						      s.delegate.map { String(describing: type(of: $0)) } ?? "nil",
						      s.activationState.rawValue)
					}

					NSLog("[DEEPLINK] forwarding to appDelegate.application(_:continue:restorationHandler:)")
					// Direct call (no optional chaining): GDTApplicationDelegate's
					// concrete Objective-C implementation exposes
					// application(_:continue:restorationHandler:) as a non-optional
					// method on the class to Swift, even though the
					// UIApplicationDelegate protocol marks it @objc optional.
					// The Bool return is logged purely to confirm the dispatch
					// reached a service implementation.
					let handled = appDelegate.application(UIApplication.shared,
					                                      continue: userActivity,
					                                      restorationHandler: { _ in })
					NSLog("[DEEPLINK] application(_:continue:restorationHandler:) returned %@", String(handled))
				}
				// HTTPS Universal Links on iOS 17+ in a SwiftUI WindowGroup are
				// delivered through the openURL pipeline as a plain URL, NOT
				// through scene:continueUserActivity: / .onContinueUserActivity.
				// scene:willContinueUserActivityWithType: still fires as a
				// heads-up that an NSUserActivityTypeBrowsingWeb activity is
				// inbound, but iOS then routes the URL via .onOpenURL instead
				// of dispatching the activity. Without this bridge SwiftUI
				// silently consumes the URL. Forward to the legacy AppDelegate
				// application:openURL:options: which the fork's
				// GDTApplicationDelegate already iterates `services` against —
				// this is the same path warm-start custom URL schemes already
				// use successfully.
				.onOpenURL { url in
					NSLog("[DEEPLINK] SwiftUI .onOpenURL fired, url=%@", url.absoluteString)
					NSLog("[DEEPLINK] forwarding to appDelegate.application(_:open:options:)")
					let handled = appDelegate.application(UIApplication.shared, open: url, options: [:])
					NSLog("[DEEPLINK] application(_:open:options:) returned %@", String(handled))
				}
		}
	}
}
