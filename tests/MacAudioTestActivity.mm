#import <Foundation/Foundation.h>
#include <pthread/qos.h>
#include <stdio.h>

// An offline regression has no physical CoreAudio device to keep the console
// process out of App Nap. Its timed UDP callbacks still model active audio.
extern "C" void* sonoCraftBeginAudioTestActivity() {
    // This console fixture supplies the audio callback itself. Process activity
    // alone does not give that thread an interactive timer policy on CI VMs:
    // stack samples showed 99% in mach_wait_until, with 3 ms waits taking 30 ms.
    const int error = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    if (error != 0) fprintf(stderr, "Audio fixture QoS unavailable: %d\n", error);
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
