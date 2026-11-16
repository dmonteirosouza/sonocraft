#pragma once
#include "SonoCraftAudioHealth.h"
#include <atomic>

// Called by the standalone holder, independently of editor visibility. All device
// operations run on the message thread; the audio callback only advances a counter.
class SonoCraftAudioRecovery {
public:
    explicit SonoCraftAudioRecovery(juce::AudioDeviceManager& owner) : manager(owner) {}
    void audioCallbackCompleted() noexcept { callbacks.fetch_add(1, std::memory_order_relaxed); }
    SonoCraftAudioHealth state() const { return health; }
    unsigned attempts() const { return totalAttempts; }
    const juce::String& error() const { return lastError; }

    void poll(bool playbackRequested, double now) {
        const auto current = manager.getAudioDeviceSetup();
        const auto type = manager.getCurrentAudioDeviceType();
        const auto xml = manager.createStateXml();
        const auto selection = xml ? xml->toString() : juce::String{};
        const auto count = callbacks.load(std::memory_order_relaxed);
        // Observe explicit selections too: a failed open may have cleared the
        // manager's names. Choosing None afterwards must cancel pending retries.
        if (!initialised || current != observedSetup || type != observedType || selection != observedSelection) {
            target = current; observedSetup = current; observedType = type; observedSelection = selection;
            initialised = true; lastCount = count; lastProgress = now; madeProgress = false;
            unhealthySince = healthySince = -1; nextAttempt = 0; consecutiveAttempts = 0; lastError.clear();
        }
        const bool hasInput = target.inputDeviceName.isNotEmpty() && (target.useDefaultInputChannels || !target.inputChannels.isZero());
        const bool hasOutput = target.outputDeviceName.isNotEmpty() && (target.useDefaultOutputChannels || !target.outputChannels.isZero());
        if (!playbackRequested || (!hasInput && !hasOutput)) {
            health = SonoCraftAudioHealth::disabled; initialised = false; return;
        }
        if (count != lastCount) { lastCount = count; lastProgress = now; madeProgress = true; }
        auto* device = manager.getCurrentAudioDevice();
        if (device && device->isPlaying() && madeProgress && now - lastProgress < 2000) {
            health = SonoCraftAudioHealth::running; unhealthySince = -1; lastError.clear();
            if (healthySince < 0) healthySince = now;
            if (now - healthySince >= 10000) { consecutiveAttempts = 0; nextAttempt = 0; }
            return;
        }
        healthySince = -1;
        if (unhealthySince < 0) unhealthySince = now;
        health = consecutiveAttempts ? SonoCraftAudioHealth::recovering
                                    : device && device->isPlaying() && !madeProgress && now - lastProgress < 2000
                                        ? SonoCraftAudioHealth::starting : SonoCraftAudioHealth::stopped;
        if (now - unhealthySince < 2000 || now < nextAttempt) return;

        // No default-device fallback: a disconnected headset must never turn on
        // another microphone or move the user's output to another destination.
        auto* backend = manager.getCurrentDeviceTypeObject();
        if (!backend || (hasInput && !backend->getDeviceNames(true).contains(target.inputDeviceName))
                     || (hasOutput && !backend->getDeviceNames(false).contains(target.outputDeviceName))) {
            health = SonoCraftAudioHealth::unavailable;
            lastError = juce::String::fromUTF8("Reconecte o dispositivo selecionado ou escolha outro em Ajustes > Áudio.");
            return;
        }
        health = SonoCraftAudioHealth::recovering;
        ++totalAttempts;
        consecutiveAttempts = juce::jmin(consecutiveAttempts + 1u, 4u);
        const double delays[] { 0, 5000, 10000, 20000, 30000 };
        nextAttempt = now + delays[consecutiveAttempts];
        // restartLastAudioDevice alone does nothing when the stopped object still
        // exists. Close first, then reopen the exact setup without saving defaults.
        manager.closeAudioDevice();
        lastError = manager.setAudioDeviceSetup(target, false);
        observedSetup = manager.getAudioDeviceSetup();
        // Preserve the requested setup after an error, but observe any real user
        // change on the following tick. Connections, captures and processor survive.
        lastCount = callbacks.load(std::memory_order_relaxed);
        lastProgress = now; madeProgress = false;
    }
private:
    juce::AudioDeviceManager& manager;
    std::atomic<unsigned> callbacks { 0 };
    juce::AudioDeviceManager::AudioDeviceSetup target, observedSetup;
    juce::String observedType, observedSelection, lastError;
    SonoCraftAudioHealth health = SonoCraftAudioHealth::starting;
    unsigned lastCount = 0, consecutiveAttempts = 0, totalAttempts = 0;
    double lastProgress = 0, unhealthySince = -1, healthySince = -1, nextAttempt = 0;
    bool initialised = false, madeProgress = false;
};
