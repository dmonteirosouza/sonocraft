// Manual integration fixture: capture the 660 Hz browser tab while a separate
// 440 Hz tab keeps playing. No microphone, real connection, or file recording.
#include "../Source/SonoCraftApplicationAudio.h"
#include <iostream>
#if JUCE_MAC
#include <mach/mach_time.h>
#endif
int main() {
    juce::ScopedJuceInitialiser_GUI runtime;
    SonoCraftApplicationAudio source;
    const auto url=source.startBrowserTab();
    if (url.isEmpty()) return 1;
    std::cout << "CAPTURE_URL " << url << std::endl;
    const auto timeout=juce::Time::getMillisecondCounterHiRes()+120000;
    juce::dsp::FFT fft(13);
    std::vector<float> spectrum(16384,0);
    juce::AudioBuffer<float> audio(2,512);
    int filled=0, windows=0; double privatePower=0,sharedPower=0;
    auto deadline=juce::Time::getMillisecondCounterHiRes();
    while (juce::Time::getMillisecondCounterHiRes()<timeout && windows<20) {
        source.render(audio,512,48000);
        if (source.isActive() && audio.getMagnitude(0,512)>0.001f) {
            for (int i=0;i<512;++i) spectrum[size_t(filled++)]=audio.getSample(0,i);
            if (filled==8192) {
                fft.performFrequencyOnlyForwardTransform(spectrum.data());
                for (int bin=72;bin<=78;++bin) privatePower+=spectrum[size_t(bin)]*spectrum[size_t(bin)];
                for (int bin=110;bin<=116;++bin) sharedPower+=spectrum[size_t(bin)]*spectrum[size_t(bin)];
                std::fill(spectrum.begin(),spectrum.end(),0); filled=0; ++windows;
            }
        }
        const auto now=juce::Time::getMillisecondCounterHiRes(); if (now-deadline>100) deadline=now;
        deadline+=512*1000.0/48000.0;
#if JUCE_MAC
        const auto remaining=deadline-juce::Time::getMillisecondCounterHiRes();
        if (remaining>0) { mach_timebase_info_data_t base;mach_timebase_info(&base);mach_wait_until(mach_absolute_time()+uint64_t(remaining*1e6*base.denom/base.numer)); }
#else
        while (juce::Time::getMillisecondCounterHiRes()<deadline) juce::Thread::sleep(1);
#endif
    }
    source.stop();
    const bool passed=windows==20 && sharedPower>1 && sharedPower>privatePower*20;
    std::cout << (passed ? "PASS" : "FAIL") << ": selected-tab isolation, shared660=" << sharedPower << " private440=" << privatePower << " windows=" << windows << std::endl;
    return passed ? 0 : 1;
}
