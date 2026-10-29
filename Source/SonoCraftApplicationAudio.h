// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include "JuceHeader.h"
#include <memory>

struct SonoCraftApplicationAudioState;
class SonoCraftApplicationAudioBackend;
class SonoCraftBrowserAudioBridge;

// Captured application output stays separate from the physical microphone.
// Capture does not change the application's playback device or replay it locally.
class SonoCraftApplicationAudio {
public:
    struct Application { juce::String name; uint32_t processId = 0; juce::String identifier; };
    struct Discovery {
        juce::Array<Application> applications;
        juce::String message;
        juce::Array<uint32_t> runningProcessIds;
        juce::Array<Application> runningApplications;
        bool hasProcessList=false;
    };
    static Discovery applicationsWithAudio();
    static bool requestCapturePermission();
    bool needsCapturePermission() const;
    SonoCraftApplicationAudio();
    ~SonoCraftApplicationAudio();
    static juce::Array<Application> applications();
    void start(const Application& application);
    void stop();
    juce::String startBrowserTab();
    bool isActive() const;
    bool isCaptureRequested() const;
    juce::String status() const;
    juce::String sourceName() const;
    float level() const;
    void render(juce::AudioBuffer<float>& output, int samples, double rate);
private:
    std::shared_ptr<SonoCraftApplicationAudioState> state;
    std::unique_ptr<SonoCraftApplicationAudioBackend> backend;
    std::unique_ptr<SonoCraftBrowserAudioBridge> browserBridge;
};
