#pragma once
#include "JuceHeader.h"
#include <array>
#include <atomic>
#include <cmath>

// One audio producer and one headphone-device consumer. No locks or allocation
// occur in push/render; independent device clocks are matched gradually.
class SonoCraftPadMonitorQueue {
public:
    void push(const juce::AudioBuffer<float>& pads, int samples, float gain) {
        int start1, size1, start2, size2;
        fifo.prepareToWrite(samples, start1, size1, start2, size2);
        for (int i = 0; i < size1 + size2; ++i) {
            const int index = i < size1 ? start1 + i : start2 + i - size1;
            for (int ch = 0; ch < 2; ++ch)
                data[ch][index] = gain * pads.getSample(juce::jmin(ch, pads.getNumChannels()-1), i);
        }
        fifo.finishedWrite(size1 + size2);
    }
    void discard() { fifo.finishedRead(fifo.getNumReady()); phase = 0; primed = false; }
    void render(float* const* output, int channels, int samples, double sourceRate, double outputRate, float volume) {
        for (int ch = 0; ch < channels; ++ch) if (output[ch]) juce::FloatVectorOperations::clear(output[ch], samples);
        const int ready = fifo.getNumReady();
        const double baseRatio = sourceRate / juce::jmax(1.0, outputRate);
        const int target = juce::jmax(512, int(samples * baseRatio * 2));
        if (!primed) { if (ready < target) return; primed = true; }
        const double correction = juce::jlimit(-0.005, 0.005, (ready - target) * 0.000002);
        const double ratio = baseRatio * (1 + correction);
        const int needed = int(std::ceil(phase + samples * ratio)) + 1;
        if (ready < needed) { primed = false; return; }
        int start1, size1, start2, size2;
        fifo.prepareToRead(needed, start1, size1, start2, size2);
        auto read = [&](int ch, int offset) { return data[ch][offset < size1 ? start1 + offset : start2 + offset - size1]; };
        for (int i = 0; i < samples; ++i) {
            const int offset = int(phase);
            const float fraction = float(phase - offset);
            if (channels == 1 && output[0]) {
                const float left = juce::jmap(fraction, read(0, offset), read(0, offset+1));
                const float right = juce::jmap(fraction, read(1, offset), read(1, offset+1));
                output[0][i] = volume * 0.5f * (left + right);
            } else for (int ch = 0; ch < juce::jmin(2, channels); ++ch)
                if (output[ch]) output[ch][i] = volume * juce::jmap(fraction, read(ch, offset), read(ch, offset + 1));
            phase += ratio;
        }
        const int consumed = int(phase);
        fifo.finishedRead(consumed); phase -= consumed;
    }
private:
    static constexpr int capacity = 65536;
    juce::AbstractFifo fifo { capacity };
    std::array<std::array<float, capacity>, 2> data {};
    double phase = 0;
    bool primed = false;
};

class SonoCraftPadMonitor final : private juce::AudioIODeviceCallback {
public:
    struct Device { juce::String type, name; };
    ~SonoCraftPadMonitor() override { stop(); }
    juce::Array<Device> devices() {
        juce::Array<Device> result;
        ensureTypes();
        for (auto* type : types) {
            if (type->getTypeName().containsIgnoreCase("exclusive")) continue;
            type->scanForDevices();
            for (auto& name : type->getDeviceNames(false)) result.add({type->getTypeName(), name});
        }
        return result;
    }
    juce::String start(const Device& selected) {
        stop();
        ensureTypes();
        for (auto* type : types) {
            if (type->getTypeName() != selected.type) continue;
            device.reset(type->createDevice(selected.name, {}));
            if (!device) break;
            auto rates = device->getAvailableSampleRates();
            double rate = rates.contains(sourceRate.load()) ? sourceRate.load() : rates.contains(48000.0) ? 48000.0 : rates.getFirst();
            if (rate <= 0) { device.reset(); return "Dispositivo sem taxa de audio disponivel."; }
            juce::BigInteger outputs; outputs.setRange(0, juce::jmin(2, device->getOutputChannelNames().size()), true);
            auto error = device->open({}, outputs, rate, device->getDefaultBufferSize());
            if (error.isNotEmpty()) { device.reset(); return error; }
            selectedType = selected.type; selectedName = selected.name;
            outputRate = device->getCurrentSampleRate();
            firstCallback = true;
            active = true;
            device->start(this);
            if (!device->isPlaying()) { stop(); return "Nao foi possivel iniciar a saida dos fones."; }
            return {};
        }
        return "Saida de audio nao encontrada. Atualize a lista de dispositivos.";
    }
    void stop() {
        active = false;
        if (device) { device->stop(); device->close(); device.reset(); }
    }
    juce::String getDeviceName() const { return selectedName; }
    juce::String getDeviceType() const { return selectedType; }
    bool isActive() const { return active.load(); }
    void setVolume(float value) { volume = juce::jlimit(0.0f, 1.0f, value); }
    float getVolume() const { return volume.load(); }
    void setSourceRate(double value) { if (value > 0) sourceRate = value; }
    void capture(const juce::AudioBuffer<float>& pads, int samples, float gain) {
        if (active.load()) queue.push(pads, samples, gain);
    }
private:
    juce::AudioDeviceManager catalog;
    juce::OwnedArray<juce::AudioIODeviceType> types;
    void ensureTypes() { if (types.isEmpty()) catalog.createAudioDeviceTypes(types); }
    std::unique_ptr<juce::AudioIODevice> device;
    SonoCraftPadMonitorQueue queue;
    std::atomic<bool> active { false };
    std::atomic<float> volume { 0.7f };
    std::atomic<double> sourceRate { 48000.0 };
    juce::String selectedType, selectedName;
    double outputRate = 48000;
    bool firstCallback = true;
    void audioDeviceIOCallbackWithContext(const float* const*, int, float* const* output, int channels, int samples, const juce::AudioIODeviceCallbackContext&) override {
        if (firstCallback) { queue.discard(); firstCallback = false; }
        queue.render(output, channels, samples, sourceRate.load(), outputRate, active.load() ? volume.load() : 0);
    }
    void audioDeviceAboutToStart(juce::AudioIODevice* output) override { outputRate = output->getCurrentSampleRate(); }
    void audioDeviceStopped() override { active = false; }
    void audioDeviceError(const juce::String&) override { active = false; }
};
