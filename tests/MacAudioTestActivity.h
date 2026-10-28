#pragma once
#if JUCE_MAC
extern "C" void* sonoCraftBeginAudioTestActivity();
extern "C" void sonoCraftEndAudioTestActivity(void*);
struct SonoCraftScopedAudioTestActivity {
    void* token = sonoCraftBeginAudioTestActivity();
    ~SonoCraftScopedAudioTestActivity() { sonoCraftEndAudioTestActivity(token); }
};
#endif
