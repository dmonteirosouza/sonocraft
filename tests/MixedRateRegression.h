#include "../deps/aoo/lib/src/common.hpp"

static bool testResamplerCapacity()
{
    for (const auto rates : { std::pair<int,int>{48000,16000}, {48000,8000}, {96000,16000},
                             {16000,48000}, {44100,48000}, {48000,44100}, {48000,48000} }) {
        for (const int channels : { 1, 2, 3, 6 }) {
            aoo::dynamic_resampler resampler;
            constexpr int inputFrames = 127, outputFrames = 480;
            resampler.setup(inputFrames, outputFrames, rates.first, rates.second, channels);
            resampler.update(rates.first, rates.second);
            std::vector<aoo_sample> input(inputFrames * channels), output(outputFrames * channels);
            for (int i=0; i<inputFrames; ++i) for (int ch=0; ch<channels; ++ch)
                input[i*channels+ch] = aoo_sample(0.1 * (ch+1));
            for (int block=0; block<200; ++block) {
                // Keep one interpolation frame beyond the requested output.
                while (resampler.read_available() < int(output.size()) + channels) {
                    if (resampler.write_available() < int(input.size())) {
                        std::cerr << "FAIL: resampler cannot buffer a complete output callback\n";
                        return false;
                    }
                    resampler.write(input.data(), int(input.size()));
                }
                resampler.read(output.data(), int(output.size()));
                for (int i=0; i<outputFrames; ++i) for (int ch=0; ch<channels; ++ch)
                    if (!std::isfinite(output[i*channels+ch])
                        || std::abs(output[i*channels+ch] - aoo_sample(0.1*(ch+1))) > 0.00001) {
                        std::cerr << "FAIL: resampler lost channel alignment across wrap\n";
                        return false;
                    }
            }
        }
    }
    std::cout << "PASS: resampler capacity and channel alignment across seven rate pairs" << std::endl;
    return true;
}

// Exercise the separate source lanes with a 48 kHz sender and a 16 kHz
// receiver, using independent callback schedules (as Bluetooth headsets do).
static bool testMixedRateSource(bool equalRate = false)
{
    const int receiverRate = equalRate ? 48000 : 16000;
    auto txOwner = std::make_unique<SonobusAudioProcessor>();
    auto rxOwner = std::make_unique<SonobusAudioProcessor>();
    auto& tx = *txOwner; auto& rx = *rxOwner;
    for (auto* p : { &tx, &rx }) {
        for (auto* parameter : p->getParameters())
            parameter->setValueNotifyingHost(parameter->getDefaultValue());
        auto set = [p](const juce::String& id, float value) {
            auto* parameter = p->getValueTreeState().getParameter(id);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        set(SonobusAudioProcessor::paramDry, 0);
        set(SonobusAudioProcessor::paramWet, 1);
        set(SonobusAudioProcessor::paramMainSendMute, 0);
        set(SonobusAudioProcessor::paramMainRecvMute, 0);
        set(SonobusAudioProcessor::paramMetEnabled, 0);
        set(SonobusAudioProcessor::paramMainReverbEnabled, 0);
        set(SonobusAudioProcessor::paramSendSoundboardAudio, 1);
        set(SonobusAudioProcessor::paramDefaultNetbufMs, 0.080f);
        p->setDefaultAutoresizeBufferMode(SonobusAudioProcessor::AutoNetBufferModeOff);
        p->getSourceMixer().restore({});
        p->setInputGroupCount(1);
        p->setInputGroupChannelStartAndCount(0, 0, 2);
        p->setInputGroupChannelDestStartAndCount(0, 0, 2);
    }
    tx.setStandaloneAudioDevices("", "Speakers fixture");
    rx.setStandaloneAudioDevices("Headset microphone", "Headphones fixture");
    tx.setPlayConfigDetails(0, 2, 48000, 480);
    rx.setPlayConfigDetails(1, 2, receiverRate, 480);
    tx.prepareToPlay(48000, 480); rx.prepareToPlay(receiverRate, 480);
    const auto source = tx.getSourceMixer().addTab();
    const juce::URL url(source.url);
    juce::StreamingSocket socket;
    const auto host = "127.0.0.1:" + juce::String(url.getPort());
    if (!socket.connect("127.0.0.1", url.getPort(), 3000)) return false;
    const auto request = "GET /stream?key=" + url.getParameterValues()[0]
        + " HTTP/1.1\r\nHost: " + host
        + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ=="
          "\r\nSec-WebSocket-Version: 13\r\nOrigin: http://" + host + "\r\n\r\n";
    socket.write(request.toRawUTF8(), int(request.getNumBytesAsUTF8()));
    if (!readHttpHeaders(socket).contains("101 Switching")) return false;
    const juce::String rate = "rate=48000";
    if (!sendBrowserFrame(socket, 1, rate.toRawUTF8(), int(rate.getNumBytesAsUTF8()))) return false;
    tx.getSourceMixer().setSend(source.id, true);
    if (!tx.connectRemotePeer("127.0.0.1", rx.getUdpLocalPort(), "mixed-rate", "", true)) return false;
    juce::MidiBuffer midi;
    double energy = 0; int audible = 0, measured = 0;
    auto deadline = juce::Time::getMillisecondCounterHiRes();
    for (int block = 0; block < 900; ++block) {
        float packet[960];
        for (int i = 0; i < 480; ++i)
            packet[2*i] = packet[2*i+1] = float(0.2 * std::sin(2 * 3.141592653589793 * 660 * (block*480+i) / 48000.0));
        if (!sendBrowserFrame(socket, 2, packet, sizeof(packet))) return false;
        juce::AudioBuffer<float> input(2, 480); input.clear();
        tx.processBlock(input, midi);
        if (equalRate || block % 3 == 2) {
            juce::AudioBuffer<float> output(2, 480); output.clear();
            rx.processBlock(output, midi);
            if (block > 600) {
                const float rms = output.getRMSLevel(0, 0, 480);
                if (!std::isfinite(rms)) return false;
                energy += rms; ++measured; if (rms > 0.02f) ++audible;
            }
        }
        advanceAudioClock(deadline, 480);
    }
    const bool ok = measured > 0 && energy / measured > 0.04 && audible > measured * 0.8;
    float sentPeak = 0;
    for (int ch=0; ch<tx.getSendMeterSource().getNumChannels(); ++ch)
        sentPeak = juce::jmax(sentPeak, tx.getSendMeterSource().getMaxLevel(ch));
    const auto peers = rx.getMixerChannelStates();
    std::cout << (ok ? "PASS" : "FAIL") << ": 48000 -> " << receiverRate << "; rms="
              << energy / juce::jmax(1, measured) << " audible=" << audible << "/" << measured
              << " channels=" << rx.getRemotePeerRecvChannelCount(0)
              << " packets=" << rx.getRemotePeerPacketsReceived(0)
              << " capture=" << tx.getSourceMixer().views()[0].level << " send=" << sentPeak
              << " peer=" << (peers.isEmpty() ? -1 : peers[0].left)
              << " gain=" << (peers.isEmpty() ? -1 : peers[0].gain)
              << " safety=" << (!peers.isEmpty() && peers[0].safetyMuted) << std::endl;
    socket.close(); tx.getSourceMixer().remove(source.id);
    tx.releaseResources(); rx.releaseResources();
    return ok;
}
