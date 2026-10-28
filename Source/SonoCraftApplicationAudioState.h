// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include "SonoCraftApplicationAudio.h"
#include "SonoCraftPadMonitor.h"

struct SonoCraftApplicationAudioState {
    SonoCraftPadMonitorQueue queue;
    std::atomic<bool> active {false}, reset {true}, requested {false};
    std::atomic<uint64_t> generation {0};
    std::atomic<double> rate {48000.0};
    std::atomic<float> peak {0};
    mutable juce::CriticalSection textLock;
    juce::String message {"Nenhuma fonte capturada."}, name;
    void setStatus(const juce::String& text) { const juce::ScopedLock lock(textLock); message = text; }
    void feed(const juce::AudioBuffer<float>& audio, int samples, uint64_t token) {
        if (!active.load() || generation.load() != token) return;
        peak = audio.getMagnitude(0, samples); queue.push(audio, samples, 1);
    }
};
class SonoCraftApplicationAudioBackend {
public:
    virtual ~SonoCraftApplicationAudioBackend() = default;
    virtual void start(const SonoCraftApplicationAudio::Application&, uint64_t token) = 0;
    virtual void stop() = 0;
};
std::unique_ptr<SonoCraftApplicationAudioBackend> createSonoCraftApplicationAudioBackend(std::shared_ptr<SonoCraftApplicationAudioState>);
juce::Array<SonoCraftApplicationAudio::Application> listSonoCraftAudioApplications();
