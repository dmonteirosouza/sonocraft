#import <Foundation/Foundation.h>

// An offline regression has no physical CoreAudio device to keep the console
// process out of App Nap. Its timed UDP callbacks still model active audio.
extern "C" void* sonoCraftBeginAudioTestActivity() {
    @autoreleasepool {
        const auto options = NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical;
        id token = [NSProcessInfo.processInfo beginActivityWithOptions:options reason:@"SonoCraft offline audio-device regression"];
        return (__bridge_retained void*)token;
    }
}
extern "C" void sonoCraftEndAudioTestActivity(void* value) {
    @autoreleasepool {
        id token = (__bridge_transfer id)value;
        if (token) [NSProcessInfo.processInfo endActivity:token];
    }
}
