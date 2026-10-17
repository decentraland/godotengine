/**************************************************************************/
/*  app_delegate_service.mm                                               */
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

#import "app_delegate_service.h"
#import "godot_host.h"

#import "godot_view_apple_embedded.h"
#import "godot_view_controller.h"
#import "os_apple_embedded.h"

#include "core/config/project_settings.h"
#include "core/os/main_loop.h"
#import "drivers/coreaudio/audio_driver_coreaudio.h"
#include "main/main.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioServices.h>

int gargc;
char **gargv;

extern int apple_embedded_main(int, char **);
extern void apple_embedded_finish();

@implementation GDTAppDelegateService

enum {
	SESSION_CATEGORY_AMBIENT,
	SESSION_CATEGORY_MULTI_ROUTE,
	SESSION_CATEGORY_PLAY_AND_RECORD,
	SESSION_CATEGORY_PLAYBACK,
	SESSION_CATEGORY_RECORD,
	SESSION_CATEGORY_SOLO_AMBIENT
};

static GDTViewController *mainViewController = nil;

+ (GDTViewController *)viewController {
	return mainViewController;
}

+ (void)setViewController:(GDTViewController *)viewController {
	mainViewController = viewController;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	// TODO: might be required to make an early return, so app wouldn't crash because of timeout.
	// TODO: logo screen is not displayed while shaders are compiling
	// DummyViewController(Splash/LoadingViewController) -> setup -> GodotViewController

	// Fetch the command-line arguments from NSProcessInfo
	NSArray *arguments = [[NSProcessInfo processInfo] arguments];
	gargc = (int)[arguments count];
	gargv = (char **)malloc(sizeof(char *) * gargc);
	for (int i = 0; i < gargc; i++) {
		NSString *arg = arguments[i];
		gargv[i] = strdup([arg UTF8String]);
	}

	if ([self startEngineWithArgc:gargc argv:gargv] != 0) {
		// bail, things did not go very well for us, should probably output a message on screen with our error code...
		exit(0);
		return NO;
	}

	return YES;
}

- (int)startEngineWithArgc:(int)argc argv:(char **)argv {
	int err = apple_embedded_main(argc, argv);

	if (err != 0) {
		return err;
	}

	[[NSNotificationCenter defaultCenter]
			addObserver:self
			   selector:@selector(onAudioInterruption:)
				   name:AVAudioSessionInterruptionNotification
				 object:[AVAudioSession sharedInstance]];

	int sessionCategorySetting = GLOBAL_GET("audio/general/ios/session_category");

	// Initialize with default Ambient category.
	AVAudioSessionCategory category = AVAudioSessionCategoryAmbient;
	AVAudioSessionCategoryOptions options = 0;

	if (GLOBAL_GET("audio/general/ios/mix_with_others")) {
		options |= AVAudioSessionCategoryOptionMixWithOthers;
	}

	if (sessionCategorySetting == SESSION_CATEGORY_MULTI_ROUTE) {
		category = AVAudioSessionCategoryMultiRoute;
	} else if (sessionCategorySetting == SESSION_CATEGORY_PLAY_AND_RECORD) {
		category = AVAudioSessionCategoryPlayAndRecord;
		options |= AVAudioSessionCategoryOptionDefaultToSpeaker;
		options |= AVAudioSessionCategoryOptionAllowBluetoothA2DP;
		options |= AVAudioSessionCategoryOptionAllowAirPlay;
	} else if (sessionCategorySetting == SESSION_CATEGORY_PLAYBACK) {
		category = AVAudioSessionCategoryPlayback;
	} else if (sessionCategorySetting == SESSION_CATEGORY_RECORD) {
		category = AVAudioSessionCategoryRecord;
	} else if (sessionCategorySetting == SESSION_CATEGORY_SOLO_AMBIENT) {
		category = AVAudioSessionCategorySoloAmbient;
	}

	[[AVAudioSession sharedInstance] setCategory:category withOptions:options error:nil];

	return 0;
}

- (void)onAudioInterruption:(NSNotification *)notification {
	if ([notification.name isEqualToString:AVAudioSessionInterruptionNotification]) {
		if ([[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] isEqualToNumber:[NSNumber numberWithInt:AVAudioSessionInterruptionTypeBegan]]) {
			NSLog(@"Audio interruption began");
			OS_AppleEmbedded::get_singleton()->on_focus_out();
		} else if ([[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] isEqualToNumber:[NSNumber numberWithInt:AVAudioSessionInterruptionTypeEnded]]) {
			NSLog(@"Audio interruption ended");
			OS_AppleEmbedded::get_singleton()->on_focus_in();
		}
	}
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
	if (OS::get_singleton()->get_main_loop()) {
		OS::get_singleton()->get_main_loop()->notification(MainLoop::NOTIFICATION_OS_MEMORY_WARNING);
	}
}

- (void)applicationWillTerminate:(UIApplication *)application {
	apple_embedded_finish();
}

// When application goes to background (e.g. user switches to another app or presses Home),
// then applicationWillResignActive -> applicationDidEnterBackground are called.
// When user opens the inactive app again,
// applicationWillEnterForeground -> applicationDidBecomeActive are called.

// There are cases when applicationWillResignActive -> applicationDidBecomeActive
// sequence is called without the app going to background. For example, that happens
// if you open the app list without switching to another app or open/close the
// notification panel by swiping from the upper part of the screen.

- (void)sceneDidDisconnect:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::get_singleton()->on_focus_out();
}

- (void)sceneWillResignActive:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::get_singleton()->on_focus_out();
}

- (void)sceneDidBecomeActive:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::get_singleton()->on_focus_in();
}

- (void)sceneDidEnterBackground:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::get_singleton()->on_enter_background();
}

- (void)sceneWillEnterForeground:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::get_singleton()->on_exit_background();
}

- (void)applicationWillResignActive:(UIApplication *)application {
	OS_AppleEmbedded::get_singleton()->on_focus_out();
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	OS_AppleEmbedded::get_singleton()->on_focus_in();
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
	OS_AppleEmbedded::get_singleton()->on_enter_background();
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	OS_AppleEmbedded::get_singleton()->on_exit_background();
}

- (void)dealloc {
	self.window = nil;
}

@end

// MARK: Host API (see godot_host.h)

static bool host_mode = false;
static GDTAppDelegateService *host_service = nil;

bool godot_host_is_hosted(void) {
	return host_mode;
}

int godot_host_init(int argc, const char *const *argv) {
	if (host_service) {
		return 0;
	}
	host_mode = true;
	gargc = argc;
	gargv = (char **)malloc(sizeof(char *) * argc);
	for (int i = 0; i < argc; i++) {
		gargv[i] = strdup(argv[i]);
	}
	host_service = [GDTAppDelegateService new];
	int err = [host_service startEngineWithArgc:gargc argv:gargv];
	if (err != 0) {
		host_service = nil;
		return err;
	}
	godot_host_register_bridge();
	return 0;
}

void godot_host_focus_in(void) {
	if (OS_AppleEmbedded::get_singleton()) {
		OS_AppleEmbedded::get_singleton()->on_focus_in();
	}
}

void godot_host_focus_out(void) {
	if (OS_AppleEmbedded::get_singleton()) {
		OS_AppleEmbedded::get_singleton()->on_focus_out();
	}
}

void godot_host_enter_background(void) {
	if (OS_AppleEmbedded::get_singleton()) {
		OS_AppleEmbedded::get_singleton()->on_enter_background();
	}
}

void godot_host_exit_background(void) {
	if (OS_AppleEmbedded::get_singleton()) {
		OS_AppleEmbedded::get_singleton()->on_exit_background();
	}
}

void godot_host_memory_warning(void) {
	if (OS::get_singleton() && OS::get_singleton()->get_main_loop()) {
		OS::get_singleton()->get_main_loop()->notification(MainLoop::NOTIFICATION_OS_MEMORY_WARNING);
	}
}

void godot_host_shutdown(void) {
	apple_embedded_finish();
	host_service = nil;
}
