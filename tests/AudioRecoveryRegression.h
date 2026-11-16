#include "../Source/SonoCraftAudioRecovery.h"
#include "../Source/SonoCraftChannelPresentation.h"

namespace AudioRecoveryTest {
struct BackendState { bool available = true, failOpen = false; int opens = 0; };
class Device final : public juce::AudioIODevice {
public:
    explicit Device(BackendState& s) : AudioIODevice("Test output", "Recovery test"), state(s) {}
    juce::StringArray getOutputChannelNames() override { return {"L", "R"}; }
    juce::StringArray getInputChannelNames() override { return {"Mic"}; }
    juce::Array<double> getAvailableSampleRates() override { return {44100, 48000}; }
    juce::Array<int> getAvailableBufferSizes() override { return {256, 512}; }
    int getDefaultBufferSize() override { return 512; }
    juce::String open(const juce::BigInteger& in, const juce::BigInteger& out, double sr, int bs) override {
        ++state.opens;
        if (state.failOpen) return "Simulated driver error";
        inputs = in; outputs = out; rate = sr; block = bs; opened = true; return {};
    }
    void close() override { stop(); opened = false; }
    bool isOpen() override { return opened; }
    void start(juce::AudioIODeviceCallback* cb) override { callback = cb; playing = true; cb->audioDeviceAboutToStart(this); }
    void stop() override { playing = false; if (callback) callback->audioDeviceStopped(); callback = nullptr; }
    bool isPlaying() override { return playing; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return block; }
    double getCurrentSampleRate() override { return rate; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override { return outputs; }
    juce::BigInteger getActiveInputChannels() const override { return inputs; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }
    void pump() {
        if (!playing || !callback) return;
        juce::AudioBuffer<float> silence(2, block); silence.clear();
        callback->audioDeviceIOCallbackWithContext(nullptr, 0, silence.getArrayOfWritePointers(), outputs.countNumberOfSetBits(), block, {});
    }
private:
    BackendState& state;
    juce::AudioIODeviceCallback* callback = nullptr;
    juce::BigInteger inputs, outputs;
    double rate = 48000;
    int block = 512;
    bool opened = false, playing = false;
};
class Type final : public juce::AudioIODeviceType {
public:
    explicit Type(BackendState& s) : AudioIODeviceType("Recovery test"), state(s) {}
    void scanForDevices() override {}
    juce::StringArray getDeviceNames(bool input) const override {
        return state.available ? juce::StringArray{input ? "Test input" : "Test output"} : juce::StringArray{};
    }
    int getDefaultDeviceIndex(bool) const override { return 0; }
    int getIndexOfDevice(juce::AudioIODevice*, bool) const override { return 0; }
    bool hasSeparateInputsAndOutputs() const override { return true; }
    juce::AudioIODevice* createDevice(const juce::String&, const juce::String&) override { return new Device(state); }
private:
    BackendState& state;
};
class Callback final : public juce::AudioIODeviceCallback {
public:
    explicit Callback(SonoCraftAudioRecovery& r) : recovery(r) {}
    void audioDeviceAboutToStart(juce::AudioIODevice*) override { ++starts; }
    void audioDeviceStopped() override { ++stops; }
    void audioDeviceIOCallbackWithContext(const float* const*, int, float* const*, int, int, const juce::AudioIODeviceCallbackContext&) override {
        recovery.audioCallbackCompleted();
    }
    int starts = 0, stops = 0;
private:
    SonoCraftAudioRecovery& recovery;
};
}

static bool testAudioRecovery() {
    using namespace AudioRecoveryTest;
    using Health = SonoCraftAudioHealth;
    BackendState backend;
    juce::AudioDeviceManager manager;
    // Register only a fake device backend. Never open the user's audio hardware.
    manager.addAudioDeviceType(std::make_unique<Type>(backend));
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.outputDeviceName = "Test output"; setup.sampleRate = 48000; setup.bufferSize = 512;
    setup.useDefaultInputChannels = setup.useDefaultOutputChannels = false;
    setup.outputChannels.setRange(0, 2, true);
    if (manager.setAudioDeviceSetup(setup, true).isNotEmpty()) return false;
    SonoCraftAudioRecovery recovery(manager);
    Callback callback(recovery);
    manager.addAudioCallback(&callback);
    struct Cleanup { juce::AudioDeviceManager& m; Callback& c; ~Cleanup() { m.removeAudioCallback(&c); m.closeAudioDevice(); } } cleanup {manager, callback};
    auto check = [](bool value, const char* reason) { if (!value) std::cerr << "FAIL recovery: " << reason << '\n'; return value; };
    auto pump = [&] { static_cast<Device*>(manager.getCurrentAudioDevice())->pump(); };
    recovery.poll(true, 0);
    for (int time = 500; time <= 15000; time += 500) { pump(); recovery.poll(true, time); }
    if (!check(recovery.state() == Health::running && recovery.attempts() == 0, "silent callbacks are healthy")) return false;
    const auto saved = manager.createStateXml()->toString();
    // The reported incident: object still exists but isPlaying() is false.
    manager.getCurrentAudioDevice()->stop();
    recovery.poll(true, 15500);
    if (!check(recovery.state() == Health::stopped && recovery.attempts() == 0, "report stopped before restarting")) return false;
    recovery.poll(true, 17000);
    if (!check(recovery.attempts() == 0, "debounce a short stop")) return false;
    recovery.poll(true, 17500);
    if (!check(recovery.state() == Health::recovering, "successful open alone is not proof of processing")) return false;
    pump(); recovery.poll(true, 18000);
    if (!check(recovery.state() == Health::running && recovery.attempts() == 1 && callback.starts == 2
        && manager.getAudioDeviceSetup() == setup && manager.createStateXml()->toString() == saved, "reopen same device/setup without changing saved selections")) return false;
    // Device claims to play, but its callback stops. Driver errors preserve the
    // intended setup even though JUCE clears its current names on open failure.
    backend.failOpen = true;
    recovery.poll(true, 20500); recovery.poll(true, 22500);
    if (!check(recovery.attempts() == 2 && recovery.error().isNotEmpty(), "stalled callback recovery and error")) return false;
    recovery.poll(true, 23000); recovery.poll(true, 32000);
    if (!check(recovery.attempts() == 2, "backoff after failure")) return false;
    backend.failOpen = false;
    recovery.poll(true, 32500); pump(); recovery.poll(true, 33000);
    if (!check(recovery.state() == Health::running && manager.getAudioDeviceSetup() == setup, "retry retained device after failed open")) return false;
    for (int time = 33500; time <= 44000; time += 500) { pump(); recovery.poll(true, time); }
    manager.getCurrentAudioDevice()->stop(); backend.available = false;
    recovery.poll(true, 44500); recovery.poll(true, 46500);
    const auto attempts = recovery.attempts();
    if (!check(recovery.state() == Health::unavailable, "missing device is explicit")) return false;
    recovery.poll(true, 80000);
    if (!check(recovery.attempts() == attempts, "never use a default device when chosen device is absent")) return false;
    backend.available = true; recovery.poll(true, 80500); pump(); recovery.poll(true, 81000);
    if (!check(recovery.state() == Health::running, "recover when original device returns")) return false;
    manager.getCurrentAudioDevice()->stop(); backend.failOpen = true;
    recovery.poll(true, 81500); recovery.poll(true, 85500);
    if (!check(recovery.error().isNotEmpty() && manager.getCurrentAudioDevice() == nullptr, "failed retry before explicit None")) return false;
    juce::AudioDeviceManager::AudioDeviceSetup none = setup; none.outputDeviceName.clear(); none.outputChannels.clear();
    manager.setAudioDeviceSetup(none, true);
    recovery.poll(true, 86000); recovery.poll(true, 120000);
    if (!check(recovery.state() == Health::disabled && manager.getCurrentAudioDevice() == nullptr, "None cancels pending failed-device retries")) return false;
    backend.failOpen = false; manager.setAudioDeviceSetup(setup, true);
    recovery.poll(false, 121000); recovery.poll(false, 150000);
    if (!check(recovery.state() == Health::disabled, "intentional suspension never recovers")) return false;

    SonoCraftSourceMixer::View source;
    source.active = source.sending = true; source.level = .2f;
    for (auto health : {Health::starting, Health::stopped, Health::recovering, Health::unavailable, Health::disabled}) {
        auto status = SonoCraftPresentation::describeSource(source, true, false, {}, health);
        if (!check(status.state != "Enviando" && status.tone == SonoCraftPresentation::Tone::attention, "no sending claim with stopped engine and stale level")) return false;
    }
    if (!check(SonoCraftPresentation::describeSource(source, true, false, {}, Health::running).state == "Enviando", "healthy audible send")) return false;
    source.level = 0;
    if (!check(SonoCraftPresentation::describeSource(source, true, false, {}, Health::running).state == "Sem som agora", "active capture with silence")) return false;
    source.level = .2f;
    if (!check(SonoCraftPresentation::describeSource(source, false, false, {}, Health::running).state != "Enviando"
        && SonoCraftPresentation::describeSource(source, true, true, {}, Health::running).state != "Enviando", "connection and send mute still required")) return false;
    std::cout << "PASS: device stop/stall recovery, backoff, missing device, explicit None, preserved setup and truthful send status\n";
    return true;
}
