// SonoCraft regression coverage: no audio device or remote server required.
#include "../Source/SonobusPluginProcessor.h"
#include "../Source/SoundboardChannelProcessor.h"
#include "../Source/SoundboardProcessor.h"
#include "../Source/SonoCraftBrowserAudioBridge.h"
#include <cmath>
#include <iostream>
#include "MacAudioTestActivity.h"
#if JUCE_MAC
#include <mach/mach_time.h>
#endif

static void waitForAudioDeadline(double deadline)
{
#if JUCE_MAC
    // Use an absolute kernel wait instead of spinning on a hosted runner's
    // limited CPUs or requesting a coalesced nanosleep. Leave CPU time for the
    // UDP workers, as a real hardware-driven audio callback does.
    const auto remaining = deadline - juce::Time::getMillisecondCounterHiRes();
    if (remaining > 0.3) {
        mach_timebase_info_data_t timebase;
        mach_timebase_info(&timebase);
        const auto ticks = static_cast<uint64_t>((remaining - 0.1) * 1.0e6
                                                * timebase.denom / timebase.numer);
        mach_wait_until(mach_absolute_time() + ticks);
    }
#endif
    while (juce::Time::getMillisecondCounterHiRes() < deadline)
        juce::Thread::yield();
}

static void advanceAudioClock(double& deadline, int samples)
{
    const double period = samples * 1000.0 / 48000.0;
    const double now = juce::Time::getMillisecondCounterHiRes();
    // A suspended device callback cannot replay its missed ticks in a burst.
    // Even 20 ms of catch-up overruns a small queue and starves the socket
    // workers. Apply this to every fixture, including browser capture and pads.
    if (now - deadline > period) deadline = now;
    deadline += period;
    waitForAudioDeadline(deadline);
}

// A callback-driven physical-output substitute: the processor still runs its
// real FIFO/open/start/stop path while tests observe both destinations.
class TestListeningDevice final : public juce::AudioIODevice {
public:
    TestListeningDevice() : AudioIODevice("Headset fixture", "Regression") {}
    juce::StringArray getOutputChannelNames() override { return { "Left", "Right" }; }
    juce::StringArray getInputChannelNames() override { return {}; }
    juce::Array<double> getAvailableSampleRates() override { return { 48000.0 }; }
    juce::Array<int> getAvailableBufferSizes() override { return { 128 }; }
    int getDefaultBufferSize() override { return 128; }
    juce::String open(const juce::BigInteger&, const juce::BigInteger&, double, int) override { opened = true; return {}; }
    void close() override { stop(); opened = false; }
    bool isOpen() override { return opened; }
    void start(juce::AudioIODeviceCallback* value) override { callback = value; callback->audioDeviceAboutToStart(this); juce::AudioBuffer<float> initial(2,128); render(initial); }
    void stop() override { if (callback) callback->audioDeviceStopped(); callback = nullptr; }
    bool isPlaying() override { return callback != nullptr; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return 128; }
    double getCurrentSampleRate() override { return 48000; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override { juce::BigInteger channels; channels.setRange(0, 2, true); return channels; }
    juce::BigInteger getActiveInputChannels() const override { return {}; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }
    void render(juce::AudioBuffer<float>& buffer) { if (callback) callback->audioDeviceIOCallbackWithContext(nullptr, 0, buffer.getArrayOfWritePointers(), 2, buffer.getNumSamples(), {}); }
private:
    juce::AudioIODeviceCallback* callback = nullptr;
    bool opened = false;
};

static bool sendBrowserFrame(juce::StreamingSocket& socket, int opcode, const void* data, int length) {
    juce::MemoryOutputStream frame;
    frame.writeByte(char(0x80|opcode));
    if (length<126) frame.writeByte(char(0x80|length));
    else { frame.writeByte(char(0x80|126)); frame.writeShortBigEndian(short(length)); }
    const uint8_t mask[] {0x13,0x2a,0x48,0x7f}; frame.write(mask,4);
    const auto* bytes=static_cast<const uint8_t*>(data);
    for (int i=0;i<length;++i) frame.writeByte(char(bytes[i]^mask[i%4]));
    return socket.write(frame.getData(),int(frame.getDataSize()))==int(frame.getDataSize());
}
static juce::String readHttpHeaders(juce::StreamingSocket& socket) {
    juce::String text; const auto deadline=juce::Time::getMillisecondCounterHiRes()+3000;
    while (!text.endsWith("\r\n\r\n") && text.length()<8192 && juce::Time::getMillisecondCounterHiRes()<deadline) {
        if (socket.waitUntilReady(true,50)<=0) continue;
        char byte; if (socket.read(&byte,1,false)!=1) break; text+=juce::String::charToString(byte);
    }
    return text;
}

#include "SourceMixerRegression.h"

class EmptyPlayHead final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override { return {}; }
};

int main()
{
#if JUCE_MAC
    SonoCraftScopedAudioTestActivity activity;
#endif
    if (sonoCraftWebSocketAccept("dGhlIHNhbXBsZSBub25jZQ==") != "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") {
        std::cerr << "FAIL: RFC 6455 handshake\n"; return 1;
    }
    if (!SonoCraftAudioRouting::feedsBack("VB-Cable", "VB-Cable")
        || !SonoCraftAudioRouting::feedsBack("CABLE Output (VB-Audio Virtual Cable)", "CABLE Input (VB-Audio Virtual Cable)")
        || SonoCraftAudioRouting::feedsBack("Headset", "Headset")
        || SonoCraftAudioRouting::feedsBack("CABLE-A Output (VB-Audio Cable A)", "CABLE-B Input (VB-Audio Cable B)")
        || SonoCraftAudioRouting::listeningError("Headset", "CABLE Input").isNotEmpty()
        || SonoCraftAudioRouting::listeningError("Headset", "Headset").isEmpty()) {
        std::cerr << "FAIL: audio destination validation\n"; return 1;
    }
    // Independent device queue: only pad samples enter, channels remain
    // isolated, gain applies locally, and underflow always produces silence.
    auto monitorQueue = std::make_unique<SonoCraftPadMonitorQueue>();
    juce::AudioBuffer<float> padQueueInput(2, 4096), headphones(3, 256);
    for (int i = 0; i < 4096; ++i) { padQueueInput.setSample(0, i, 0.25f); padQueueInput.setSample(1, i, 0.5f); }
    headphones.clear();
    monitorQueue->render(headphones.getArrayOfWritePointers(), 3, 256, 48000, 48000, 1);
    if (headphones.getMagnitude(0, 256) != 0) return 1;
    for (double rate : { 48000.0, 44100.0, 96000.0 }) {
        monitorQueue->discard();
        monitorQueue->push(padQueueInput, 4096, 1);
        monitorQueue->render(headphones.getArrayOfWritePointers(), 3, 256, rate, 48000, 0.5f);
        if (std::abs(headphones.getSample(0, 100) - 0.125f) > 0.00001f
            || std::abs(headphones.getSample(1, 100) - 0.25f) > 0.00001f
            || headphones.getMagnitude(2, 0, 256) != 0) {
            std::cerr << "FAIL: independent pad monitor channel/rate/gain routing\n"; return 1;
        }
    }
    monitorQueue.reset();
    std::cout << "Starting audio regression" << std::endl;
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    if (!testSourceMixer()) return 1;
    // Audio processors contain large DSP buffers. Windows console programs
    // have a smaller default stack than macOS, so mirror the application's
    // factory ownership and keep the two processors on the heap.
    auto senderOwner = std::make_unique<SonobusAudioProcessor>();
    auto receiverOwner = std::make_unique<SonobusAudioProcessor>();
    auto& sender = *senderOwner;
    auto& receiver = *receiverOwner;
    auto resetTestParameters = [](SonobusAudioProcessor& processor) {
        // User-saved application defaults must not change regression behavior.
        for (auto* parameter : processor.getParameters())
            parameter->setValueNotifyingHost(parameter->getDefaultValue());
        auto set = [&](const juce::String& id, float value) {
            auto* parameter = processor.getValueTreeState().getParameter(id);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        set(SonobusAudioProcessor::paramDry, 0);
        set(SonobusAudioProcessor::paramWet, 1);
        set(SonobusAudioProcessor::paramMainSendMute, 0);
        set(SonobusAudioProcessor::paramMainRecvMute, 0);
        set(SonobusAudioProcessor::paramMetEnabled, 0);
        set(SonobusAudioProcessor::paramMainReverbEnabled, 0);
        processor.getSourceMixer().restore({});
        processor.setInputGroupCount(1);
        processor.setInputGroupChannelStartAndCount(0, 0, 2);
        processor.setInputGroupChannelDestStartAndCount(0, 0, 2);
    };
    resetTestParameters(sender);
    resetTestParameters(receiver);
    std::cout << "Audio processors initialized" << std::endl;
    EmptyPlayHead emptyPlayHead;
    sender.setPlayHead(nullptr);
    receiver.setPlayHead(&emptyPlayHead);
    sender.setRateAndBufferSizeDetails(48000.0, 128);
    receiver.setRateAndBufferSizeDetails(48000.0, 128);
    // Use a fixed 40 ms jitter buffer to isolate transport from adaptation policy.
    for (auto* processor : { &sender, &receiver })
    {
        processor->setDefaultAutoresizeBufferMode(SonobusAudioProcessor::AutoNetBufferModeOff);
        auto* bufferTime = processor->getValueTreeState().getParameter(SonobusAudioProcessor::paramDefaultNetbufMs);
        bufferTime->setValueNotifyingHost(bufferTime->convertTo0to1(0.040f));
    }
    sender.setInputGroupCount(1);
    sender.setInputGroupChannelStartAndCount(0, 0, 2);
    sender.setInputGroupChannelDestStartAndCount(0, 0, 2);
    sender.prepareToPlay(48000.0, 128);
    receiver.prepareToPlay(48000.0, 128);

    if (sender.getUdpLocalPort() <= 0 || receiver.getUdpLocalPort() <= 0
        || sender.getUdpLocalPort() == receiver.getUdpLocalPort())
    {
        std::cerr << "FAIL: two independent UDP sockets required\n";
        return 1;
    }

    juce::DatagramSocket silentEndpoint;
    if (!silentEndpoint.bindToPort(0)
        || !sender.connectRemotePeer("127.0.0.1", silentEndpoint.getBoundPort(), "", "", true)
        || sender.getRemotePeerConnected(0))
    {
        std::cerr << "FAIL: an unanswered invitation must not be marked connected\n";
        return 1;
    }
    sender.removeAllRemotePeers();

    if (!sender.connectRemotePeer("127.0.0.1", receiver.getUdpLocalPort(), "receiver", "", true))
    {
        std::cerr << "FAIL: could not queue local peer invitation\n";
        return 1;
    }

    juce::MidiBuffer midi;
    double phase = 0.0;
    double receivedEnergy = 0.0, mutedEnergy = 0.0, resumedEnergy = 0.0;
    double playbackRemoteEnergy = 0.0;
    double listeningVoiceLeak = 0.0, listeningRemoteEnergy = 0.0;
    double receiverPlaybackSum = 0.0;
    int receiverPlaybackSamples = 0;
    const int blockSizes[] = {128, 1024, 64, 256, 512, 128};
    const auto startedAt = juce::Time::getMillisecondCounterHiRes();
    auto nextCallbackAt = startedAt;
    for (int block = 0; block < 1400; ++block)
    {
        if (block == 950) {
            const auto stall = juce::SystemStats::getEnvironmentVariable("SONOCRAFT_TEST_SCHEDULER_STALL_MS", "0").getIntValue();
            if (stall > 0) juce::Thread::sleep(juce::jmin(stall, 5000));
        }
        const auto samples = block < 300 || block >= 700 ? 256 : blockSizes[(block / 70) % 6];
        juce::AudioBuffer<float> input(2, samples), output(2, samples);
        output.clear();
        for (int sample = 0; sample < samples; ++sample)
        {
            const auto value = static_cast<float>(0.1 * std::sin(phase));
            phase += 2.0 * 3.141592653589793 * 440.0 / 48000.0;
            input.setSample(0, sample, value);
            input.setSample(1, sample, value);
        }
        if (block % 100 == 0)
            std::cout << "Processing block " << block << ", samples=" << samples
                      << ", elapsed=" << (juce::Time::getMillisecondCounterHiRes() - startedAt) << " ms" << std::endl;
        if (block == 930) {
            const bool activeBefore = receiver.getRemotePeerRecvActive(0);
            const int channelsBefore = receiver.getRemotePeerRecvChannelCount(0);
            for (int32_t staleId : { 0, 99999 }) {
                aoo_source_state_event staleStop {};
                staleStop.type = AOO_SOURCE_STATE_EVENT;
                staleStop.endpoint = receiver.findOrAddEndpoint("127.0.0.1",sender.getUdpLocalPort());
                staleStop.id = staleId; staleStop.state = AOO_SOURCE_STATE_STOP;
                aoo_source_event staleFormat {};
                staleFormat.type = AOO_SOURCE_FORMAT_EVENT; staleFormat.endpoint = staleStop.endpoint; staleFormat.id = staleId;
                const aoo_event* staleEvents[] { reinterpret_cast<const aoo_event*>(&staleStop), reinterpret_cast<const aoo_event*>(&staleFormat) };
                receiver.handleSinkEvents(staleEvents,2,1);
                if ((activeBefore && !receiver.getRemotePeerRecvActive(0)) || receiver.getRemotePeerRecvChannelCount(0)!=channelsBefore) {
                    std::cerr << "FAIL: stale sink event changed live receive state, source=" << staleId << " activeBefore=" << activeBefore << " activeAfter=" << receiver.getRemotePeerRecvActive(0) << "\n"; return 1;
                }
            }
        }
        if (block == 950) {
            // Each fresh test peer owns sink 1. Simulate a delayed teardown of
            // source 0 (bootstrap), targeting that same remote sink. It must
            // not stop the negotiated source or subsequent unmute audio.
            aoo_sink_event bootstrapClose {};
            bootstrapClose.type = AOO_UNINVITE_EVENT;
            bootstrapClose.endpoint = sender.findOrAddEndpoint("127.0.0.1", receiver.getUdpLocalPort());
            bootstrapClose.id = 1;
            const aoo_event* events[] = { reinterpret_cast<const aoo_event*>(&bootstrapClose) };
            sender.handleSourceEvents(events, 1, 0);
        }
        if (block == 1000 || block == 1100) {
            const auto id = receiver.getRemotePeerConnectionId(0);
            receiver.setMixerChannelMute(id, block == 1000);
            const auto states = receiver.getMixerChannelStates();
            if (states.isEmpty() || states[0].muted != (block == 1000)) {
                std::cerr << "FAIL: participant mute state did not persist at " << block
                          << ", peers=" << states.size() << ", allow=" << receiver.getRemotePeerRecvAllow(0)
                          << ", cached=" << receiver.getRemotePeerRecvAllow(0, true)
                          << ", masterMute=" << receiver.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMainRecvMute)->load() << "\n";
                return 1;
            }
        }
        sender.processBlock(input, midi);
        // Physical input is transmitted, but does not enter local listening
        // with input monitoring disabled.
        if (block >= 1250 && block < 1300)
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < samples; ++sample) output.setSample(channel, sample, 0.02f);
        receiver.processBlock(output, midi);
        if (block > 1220 && block < 1280) {
            const float localListening = sender.getListeningMeterSource().getMaxLevel(0);
            const float remoteListening = receiver.getListeningMeterSource().getMaxLevel(0);
            if (block < 1245) listeningVoiceLeak += localListening * localListening;
            listeningRemoteEnergy += remoteListening * remoteListening;
            if (block > 1260) for (int sample = 0; sample < samples; ++sample) { receiverPlaybackSum += output.getSample(0, sample); ++receiverPlaybackSamples; }
        }
        for (int channel = 0; channel < output.getNumChannels(); ++channel)
            for (int sample = 0; sample < samples; ++sample)
            {
                const auto value = output.getSample(channel, sample);
                if (!std::isfinite(value))
                {
                    std::cerr << "FAIL: non-finite audio output\n";
                    return 1;
                }
                receivedEnergy += value * value;
                if (block > 1020 && block < 1080) mutedEnergy += value * value;
                if (block > 1120 && block < 1180) resumedEnergy += value * value;
                if (block > 1220 && block < 1280) playbackRemoteEnergy += value * value;
            }
        // Model an audio device clock, rather than adding a relative sleep to
        // processing time. macOS CI can coalesce nanosleep by tens of ms, which
        // otherwise simulates a device running far below its declared rate.
        // Hosted runners can suspend this process for seconds. A hardware
        // callback cannot catch up by sending hundreds of blocks instantly:
        // that starves the UDP worker and invalidates the recovery windows.
        // Rebase only after a scheduler stall; preserve real audio assertions.
        advanceAudioClock(nextCallbackAt, samples);
    }
    if (mutedEnergy > 0.000001 || resumedEnergy <= 0.001 || playbackRemoteEnergy <= 0.001
        || listeningVoiceLeak > 0.000001 || listeningRemoteEnergy <= 0.001
        || receiverPlaybackSamples == 0 || std::abs(receiverPlaybackSum/receiverPlaybackSamples) > 0.005) {
        std::cerr << "FAIL: mute=" << mutedEnergy << ", unmute=" << resumedEnergy
                  << ", playback=" << playbackRemoteEnergy << ", total=" << receivedEnergy << "\n";
        for (auto& state : receiver.getMixerChannelStates())
            std::cerr << "receiver muted=" << state.muted << ", solo=" << state.soloed << ", gain=" << state.gain
                      << ", connected=" << state.connected << ", packets=" << receiver.getRemotePeerPacketsReceived(0)
                      << ", active=" << receiver.getRemotePeerRecvActive(0) << ", channels=" << receiver.getRemotePeerRecvChannelCount(0) << "\n";
        return 1;
    }
    std::cout << "PASS: immediate participant mute, unmute recovery, remote playback and local voice excluded from listening" << std::endl;
    std::cout << "Audio loop completed; releasing resources" << std::endl;
    sender.releaseResources();
    receiver.releaseResources();
    receiver.setPlayHead(nullptr);
    if (receivedEnergy <= 0.001)
    {
        std::cerr << "FAIL: no audio received over local UDP; sender peers=" << sender.getNumberRemotePeers()
                  << ", receiver peers=" << receiver.getNumberRemotePeers() << "\n";
        if (receiver.getNumberRemotePeers() > 0)
            std::cerr << "Receiver packets=" << receiver.getRemotePeerPacketsReceived(0)
                      << ", active=" << receiver.getRemotePeerRecvActive(0)
                      << ", channels=" << receiver.getRemotePeerRecvChannelCount(0) << "\n";
        return 1;
    }
    // Exercise removal with a roster that cannot receive asynchronous replies.
    auto roster = std::make_unique<SonobusAudioProcessor>();
    resetTestParameters(*roster);
    roster->setRateAndBufferSizeDetails(48000.0, 128);
    roster->prepareToPlay(48000.0, 128);
    juce::DatagramSocket secondSilentEndpoint;
    if (!secondSilentEndpoint.bindToPort(0)
        || !roster->connectRemotePeer("127.0.0.1", silentEndpoint.getBoundPort())
        || !roster->connectRemotePeer("127.0.0.1", secondSilentEndpoint.getBoundPort()))
        { std::cerr << "FAIL: could not create silent roster endpoints\n"; return 1; }
    const auto removedId = roster->getRemotePeerConnectionId(0);
    const auto retainedId = roster->getRemotePeerConnectionId(1);
    roster->setMixerChannelGain(retainedId, 0.5f);
    roster->setMixerChannelMute(retainedId, true);
    roster->setMixerChannelSolo(retainedId, true);
    roster->setMixerChannelBuffer(retainedId, 40.0f, false);
    const auto mixerStates = roster->getMixerChannelStates();
    if (mixerStates.size() != 2 || mixerStates[1].id != retainedId
        || std::abs(mixerStates[1].gain - 0.5f) > 0.001f
        || !mixerStates[1].muted || !mixerStates[1].soloed
        || std::abs(mixerStates[1].bufferMs - 40.0f) > 0.01f)
    {
        std::cerr << "FAIL: mixer controls did not update their selected channel\n";
        return 1;
    }

    if (removedId.isEmpty() || retainedId.isEmpty() || removedId == retainedId
        || roster->removeRemotePeer(-1)
        || !roster->removeRemotePeerByConnectionId(removedId)
        || roster->findRemotePeerConnectionId(retainedId) != 0
        || roster->removeRemotePeerByConnectionId(removedId)
        || roster->getNumberRemotePeers() != 1
        || !roster->removeRemotePeerByConnectionId(retainedId)
        || roster->getNumberRemotePeers() != 0)
    {
        std::cerr << "FAIL: connection removal selected a stale or wrong peer\n";
        return 1;
    }
    roster->setMixerChannelGain(removedId, 0.9f);
    if (!roster->getMixerChannelStates().isEmpty()) {
        std::cerr << "FAIL: stale mixer control recreated a removed channel\n";
        return 1;
    }
    // Play a real pad through local playback, then reject a saved virtual output.
    // This exercises the Soundboard path rather than synthesising its output.
    auto fixturePath = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("sonocraft-pad-test", ".wav");
    juce::TemporaryFile padFile(fixturePath.withFileExtension("unused"), fixturePath);
    {
        juce::WavAudioFormat wav;
        auto stream = padFile.getFile().createOutputStream();
        std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(), 48000, 2, 16, {}, 0));
        if (!writer) { std::cerr << "FAIL: could not create WAV fixture at " << padFile.getFile().getFullPathName() << "\n"; return 1; }
        juce::AudioBuffer<float> tone(2, 48000);
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < tone.getNumSamples(); ++sample)
                tone.setSample(channel, sample, 0.1f * std::sin(float(sample) * 0.0576f));
        if (!writer->writeFromAudioSampleBuffer(tone, 0, tone.getNumSamples())) { std::cerr << "FAIL: could not write WAV fixture\n"; return 1; }
    }
    SoundSample pad("Regression pad", juce::URL(padFile.getFile()));
    auto player = roster->getSoundboardProcessor()->loadSample(pad);
    if (!player) { std::cerr << "FAIL: could not load pad fixture\n"; return 1; }
    // Reload a preset from a version that supported virtual microphone mixing.
    auto legacy = roster->getValueTreeState().copyState();
    for (const auto* id : { "meetingmode", "meetingincludereceived", "applicationtomeeting", "receivedapplicationstomeeting", "sendapplicationaudio" }) {
        juce::ValueTree parameter("PARAM"); parameter.setProperty("id", id, nullptr); parameter.setProperty("value", 1.0f, nullptr);
        legacy.addChild(parameter, -1, nullptr); legacy.setProperty(id, true, nullptr);
    }
    const auto legacyXml = legacy.toXmlString();
    roster->setStateInformationWithOptions(legacyXml.toRawUTF8(), int(legacyXml.getNumBytesAsUTF8()), false, false, true);
    const auto restoredState = roster->getValueTreeState().copyState();
    for (const auto* id : { "meetingmode", "meetingincludereceived", "applicationtomeeting", "receivedapplicationstomeeting", "sendapplicationaudio" }) {
        if (roster->getValueTreeState().getParameter(id) != nullptr || restoredState.hasProperty(id)
            || restoredState.getChildWithProperty("id", id).isValid()) { std::cerr << "FAIL: removed microphone routing survived preset reload\n"; return 1; }
    }
    std::cout << "PASS: legacy virtual microphone parameters removed on preset reload" << std::endl;
    roster->getSoundboardProcessor()->setMonitorGain(1.0f);
    (*player)->play();
    double padEnergy = 0, listeningOnPlaybackEnergy = 0, listeningOffPlaybackEnergy = 0, headphonePadEnergy = 0;
    TestListeningDevice* localHeadphones = nullptr;
    for (int block = 0; block < 100; ++block) {
        juce::AudioBuffer<float> padOutput(2, 128);
        padOutput.clear();
        if (block == 30) {
            auto device = std::make_unique<TestListeningDevice>(); localHeadphones = device.get();
            if (roster->getPadMonitor().startWithDevice(std::move(device), {"Regression", "Headset fixture"}).isNotEmpty()) return 1;
        }
        if (block == 60) { roster->getPadMonitor().stop(); localHeadphones = nullptr; }
        if (block == 70) roster->setStandaloneAudioDevices("Headset microphone", "CABLE Input (VB-Audio Virtual Cable)");
        if (block == 80) roster->setStandaloneAudioDevices("VB-Cable", "Headset");
        roster->processBlock(padOutput, midi);
        if (block >= 70 && block < 80 && padOutput.getMagnitude(0, 128) != 0) { std::cerr << "FAIL: pad entered a virtual microphone output\n"; return 1; }
        if (block > 40 && block < 55) listeningOnPlaybackEnergy += padOutput.getRMSLevel(0,0,128);
        if (block > 10 && block < 25) listeningOffPlaybackEnergy += padOutput.getRMSLevel(0,0,128);
        if (localHeadphones) { juce::AudioBuffer<float> ears(2,128); localHeadphones->render(ears); headphonePadEnergy += ears.getRMSLevel(0,0,128); }
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < 128; ++sample)
                padEnergy += padOutput.getSample(channel, sample) * padOutput.getSample(channel, sample);
    }
    (*player)->unload();
    if (roster->getListeningMeterSource().getMaxLevel(0) <= 0.001f) {
        std::cerr << "FAIL: local pad missing from headphone mix\n"; return 1;
    }
    if (listeningOnPlaybackEnergy <= 0.001 || listeningOffPlaybackEnergy <= 0.001 || headphonePadEnergy <= 0.001) { std::cerr << "FAIL: headphone toggle interrupted local playback\n"; return 1; }
    if (padEnergy <= 0.001) { std::cerr << "FAIL: no Soundboard in local playback\n"; return 1; }
    std::cout << "PASS: local pads play with headphones on/off; physical input cannot feed a virtual output" << std::endl;
    // Real source pad -> UDP receiver -> physical output and listening bus.
    // Muting the source microphone must not suppress its Soundboard stream.
    sender.prepareToPlay(48000, 128); receiver.prepareToPlay(48000, 128);
    auto setParameter = [](SonobusAudioProcessor& processor, const juce::String& id, float value) {
        auto* parameter = processor.getValueTreeState().getParameter(id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setParameter(sender, SonobusAudioProcessor::paramMainInMute, 1);
    setParameter(sender, SonobusAudioProcessor::paramMainSendMute, 0);
    // Reproduce saved mono/peer override settings from earlier versions.
    setParameter(sender, SonobusAudioProcessor::paramSendChannels, 1);
    sender.setRemotePeerOverrideSendChannelCount(-1, 1);
    setParameter(sender, SonobusAudioProcessor::paramSendSoundboardAudio, 1);
    SoundSample remotePad("Remote pad", juce::URL(padFile.getFile()), SoundSample::LOOP_AT_END);
    auto remotePlayer = sender.getSoundboardProcessor()->loadSample(remotePad);
    if (!remotePlayer) return 1;
    (*remotePlayer)->play();
    // Reopening the audio engine after the roster/preset phase requires a
    // real format handshake. Keep processing on an audio clock until audio
    // returns, with a hard bound; never reconnect or bypass routing checks.
    const auto resumeLimit=juce::Time::getMillisecondCounterHiRes()+5000;
    auto resumeClock=juce::Time::getMillisecondCounterHiRes();int audibleBlocks=0;
    while(audibleBlocks<24 && juce::Time::getMillisecondCounterHiRes()<resumeLimit){
        juce::AudioBuffer<float> source(2,128),destination(2,128);source.clear();destination.clear();
        sender.processBlock(source,midi);receiver.processBlock(destination,midi);
        audibleBlocks=receiver.getRemotePeerRecvActive(0) && destination.getMagnitude(0,128)>0.001f ? audibleBlocks+1 : 0;
        advanceAudioClock(resumeClock,128);
    }
    if(audibleBlocks<24){std::cerr<<"FAIL: UDP audio did not recover after reopening within 5 seconds\n";return 1;}
    std::cout<<"PASS: UDP audio recovered after engine reopen before headphone-toggle measurements"<<std::endl;
    double remotePadEnergy = 0, remotePlaybackWithEars = 0, remotePlaybackWithoutEars = 0, remoteHeadphoneEnergy = 0;
    TestListeningDevice* remoteHeadphones = nullptr;
    auto padDeadline = juce::Time::getMillisecondCounterHiRes();
    for (int block = 0; block < 480; ++block) {
        juce::AudioBuffer<float> source(2,128), destination(2,128);
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < 128; ++sample) source.setSample(channel, sample, 0.6f);
        // All source audio remains available to the physical listening output.
        if (block == 240) setParameter(sender, SonobusAudioProcessor::paramMainInMute, 0);
        destination.clear();
        if (block == 120) {
            auto device = std::make_unique<TestListeningDevice>(); remoteHeadphones = device.get();
            if (receiver.getPadMonitor().startWithDevice(std::move(device), {"Regression", "Headset fixture"}).isNotEmpty()) return 1;
        }
        if (block == 300) { receiver.getPadMonitor().stop(); remoteHeadphones = nullptr; }
        sender.processBlock(source, midi); receiver.processBlock(destination, midi);
        if (block > 180 && block < 230) remotePlaybackWithEars += destination.getRMSLevel(0,0,128);
        if (block > 330 && block < 380) remotePlaybackWithoutEars += destination.getRMSLevel(0,0,128);
        if (remoteHeadphones) { juce::AudioBuffer<float> ears(2,128); remoteHeadphones->render(ears); remoteHeadphoneEnergy += ears.getRMSLevel(0,0,128); }
        if (block > 300) for (int sample = 0; sample < 128; ++sample) {
            auto value = destination.getSample(0, sample);
            remotePadEnergy += value * value;
        }
        advanceAudioClock(padDeadline,128);
    }
    if (remotePlaybackWithEars <= 0.001 || remotePlaybackWithoutEars <= 0.001 || remoteHeadphoneEnergy <= 0.001) { std::cerr << "FAIL: remote pad lost when toggling headphones or restoring mono settings: on=" << remotePlaybackWithEars << " off=" << remotePlaybackWithoutEars << " ears=" << remoteHeadphoneEnergy << " recvChannels=" << receiver.getRemotePeerRecvChannelCount(0) << " sendChannels=" << sender.getRemotePeerActualSendChannelCount(0) << " active=" << receiver.getRemotePeerRecvActive(0) << "\n"; return 1; }
    if (remotePadEnergy <= 0.001
        || receiver.getListeningMeterSource().getMaxLevel(0) <= 0.001f) {
        std::cerr << "FAIL: remote pad/mic-mute/listening routing: energy=" << remotePadEnergy << "\n"; return 1;
    }
    (*remotePlayer)->unload();
    sender.getSoundboardProcessor()->clearPlayback(); remotePlayer.reset();
    // Browser tab IPC is tested with the same authenticated audio endpoint the
    // Chrome helper uses; no actual user window or microphone is captured.
    const auto tabChannel=sender.getSourceMixer().addTab();
    const juce::URL tabUrl(tabChannel.url);
    if (tabUrl.getPort()<=0 || tabUrl.getParameterValues().isEmpty()) return 1;
    const auto tabHost="127.0.0.1:"+juce::String(tabUrl.getPort());
    const auto token=tabUrl.getParameterValues()[0];
    const auto request="GET /stream?key="+token+" HTTP/1.1\r\nHost: "+tabHost
        +"\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\nOrigin: http://"+tabHost+"\r\n\r\n";
    {
        juce::StreamingSocket unauthorized;
        if (!unauthorized.connect("127.0.0.1",tabUrl.getPort(),3000)) return 1;
        const auto badRequest=request.replace("Origin: http://"+tabHost,"Origin: https://unrelated.invalid");
        unauthorized.write(badRequest.toRawUTF8(),int(badRequest.getNumBytesAsUTF8()));
        if (readHttpHeaders(unauthorized).contains("101 Switching") || sender.getSourceMixer().anyActive()) { std::cerr << "FAIL: cross-origin tab capture accepted\n"; return 1; }
    }
    juce::StreamingSocket browser;
    if (!browser.connect("127.0.0.1",tabUrl.getPort(),3000)) return 1;
    browser.write(request.toRawUTF8(),int(request.getNumBytesAsUTF8()));
    const auto headers=readHttpHeaders(browser);
    if (!headers.contains("101 Switching Protocols") || !headers.contains("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=")) { std::cerr << "FAIL: browser tab socket upgrade\n"; return 1; }
    const juce::String rateMessage="rate=48000";
    sendBrowserFrame(browser,1,rateMessage.toRawUTF8(),int(rateMessage.getNumBytesAsUTF8()));
    const auto activationDeadline=juce::Time::getMillisecondCounterHiRes()+3000;
    while (!sender.getSourceMixer().anyActive() && juce::Time::getMillisecondCounterHiRes()<activationDeadline) juce::Thread::sleep(1);
    if (!sender.getSourceMixer().anyActive()) { std::cerr << "FAIL: authenticated tab capture did not activate\n"; return 1; }
    sender.setStandaloneAudioDevices("VB-Cable",{});
    // Individual capture suppresses the virtual full-system input; disabling
    // the last send must not unexpectedly transmit the complete system.
    setParameter(sender,SonobusAudioProcessor::paramMainInMute,0);
    sender.getSourceMixer().setSend(tabChannel.id,true);
    double completeFeedPlayback=0, tabInHeadphones=0, fullPlaybackAfterStop=0, fullListeningAfterStop=0, duplicateSelectedAudio=0;
    double selectedReal[2]{}, selectedImag[2]{}, completeReal[2]{}, completeImag[2]{};
    int selectedSamples=0,completeSamples=0;
    auto earsDevice=std::make_unique<TestListeningDevice>(); auto* tabEars=earsDevice.get();
    receiver.getPadMonitor().startWithDevice(std::move(earsDevice),{"Regression","Headset fixture"});
    double tabPhase=0, fullPhase=0; auto tabDeadline=juce::Time::getMillisecondCounterHiRes();
    for (int block=0;block<500;++block) {
        if (block%8==0) {
            float packet[2048];
            for (int i=0;i<1024;++i) { packet[i*2]=packet[i*2+1]=float(0.12*std::sin(tabPhase)); tabPhase+=2*3.141592653589793*660/48000; }
            if (!sendBrowserFrame(browser,2,packet,sizeof(packet))) return 1;
        }
        if (block==180) sender.getSourceMixer().useCompleteInput();
        if (block==270) sender.getSourceMixer().setSend(tabChannel.id,true);
        if (block==320) sender.getSourceMixer().setSend(tabChannel.id,false);
        juce::AudioBuffer<float> local(2,128), remote(2,128), ears(2,128); local.clear(); remote.clear();
        for (int sample=0;sample<128;++sample) {
            // A different full-feed tone must never leak through individual mode.
            const auto full=float(0.1*std::sin(fullPhase));
            fullPhase+=2*3.141592653589793*440/48000;
            for (int ch=0;ch<2;++ch) local.setSample(ch,sample,full);
        }
        sender.processBlock(local,midi); receiver.processBlock(remote,midi); tabEars->render(ears);
        if (block>80 && block<130) { completeFeedPlayback+=remote.getRMSLevel(0,0,128); tabInHeadphones+=ears.getRMSLevel(0,0,128); }
        if (block>230 && block<260) duplicateSelectedAudio+=ears.getRMSLevel(0,0,128);
        if ((block>80 && block<130) || (block>230 && block<260)) {
            const bool selected=block<130; auto* real=selected?selectedReal:completeReal; auto* imag=selected?selectedImag:completeImag;
            for(int sample=0;sample<128;++sample) for(int tone=0;tone<2;++tone) {
                const auto angle=2*3.141592653589793*(tone==0?440:660)*(block*128+sample)/48000;
                real[tone]+=remote.getSample(0,sample)*std::cos(angle); imag[tone]+=remote.getSample(0,sample)*std::sin(angle);
            }
            (selected?selectedSamples:completeSamples)+=128;
        }
        if (block>440) { fullPlaybackAfterStop+=remote.getRMSLevel(0,0,128); fullListeningAfterStop+=ears.getRMSLevel(0,0,128); }
        const auto tabNow=juce::Time::getMillisecondCounterHiRes(); if (tabNow-tabDeadline>100) tabDeadline=tabNow;
        advanceAudioClock(tabDeadline,128);
    }
    const auto selectedLeak=2*std::hypot(selectedReal[0],selectedImag[0])/selectedSamples;
    const auto selectedTone=2*std::hypot(selectedReal[1],selectedImag[1])/selectedSamples;
    const auto completeTone=2*std::hypot(completeReal[0],completeImag[0])/completeSamples;
    const auto completeLeak=2*std::hypot(completeReal[1],completeImag[1])/completeSamples;
    if(selectedLeak>0.015 || selectedTone<0.025 || completeTone<0.025 || completeLeak>0.015) {
        std::cerr << "FAIL: overlapping full/selected sources: selected=" << selectedTone << " leak=" << selectedLeak
                  << " full=" << completeTone << " leak=" << completeLeak << "\n";return 1;
    }
    // Keep a remote pad playing while a live tab is repeatedly enabled/disabled
    // from the transmitted channels. The receiver's physical input is muted,
    // headphones stay open, and only the selected source may disappear.
    receiver.setStandaloneAudioDevices("Headset microphone", "CABLE Input");
    setParameter(receiver,SonobusAudioProcessor::paramMainInMute,1);
    auto padDuringTabSwitch = sender.getSoundboardProcessor()->loadSample(remotePad);
    if (!padDuringTabSwitch) return 1;
    (*padDuringTabSwitch)->play();
    double switchListeningEnergy[5] {};
    tabDeadline=juce::Time::getMillisecondCounterHiRes();
    for (int block=0;block<900;++block) {
        const int phase=block/180;
        if (block%180==0) sender.getSourceMixer().setSend(tabChannel.id,phase%2!=0);
        if (block%8==0) {
            float packet[2048];
            for (int i=0;i<1024;++i) { packet[i*2]=packet[i*2+1]=float(0.12*std::sin(tabPhase)); tabPhase+=2*3.141592653589793*660/48000; }
            if (!sendBrowserFrame(browser,2,packet,sizeof(packet))) return 1;
        }
        juce::AudioBuffer<float> local(2,128), remote(2,128), ears(2,128);
        for (int ch=0;ch<2;++ch) for (int sample=0;sample<128;++sample) {
            local.setSample(ch,sample,0.2f); remote.setSample(ch,sample,0.3f);
        }
        sender.processBlock(local,midi); receiver.processBlock(remote,midi); tabEars->render(ears);
        if (block%180>120) {
            if (remote.getMagnitude(0,128) != 0) { std::cerr << "FAIL: network audio entered a blocked virtual output\n"; return 1; }
            switchListeningEnergy[phase]+=ears.getRMSLevel(0,0,128);
        }
        const auto now=juce::Time::getMillisecondCounterHiRes(); if (now-tabDeadline>100) tabDeadline=now;
        advanceAudioClock(tabDeadline,128);
    }
    for (int phase=0;phase<5;++phase) {
        if (switchListeningEnergy[phase]<0.05
            || receiver.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMainInMute)->load()<0.5f) {
            std::cerr << "FAIL: tab send toggle interrupted pads or unmuted input: phase=" << phase
                      << " ears=" << switchListeningEnergy[phase] << "\n"; return 1;
        }
    }
    (*padDuringTabSwitch)->unload(); sender.getSoundboardProcessor()->clearPlayback(); padDuringTabSwitch.reset();
    std::cout << "PASS: tab send toggles preserve physical listening while virtual output stays silent" << std::endl;
    // A selected tab on its own still reaches normal SonoCraft playback.
    sender.setStandaloneAudioDevices("Headset microphone", "Headset");
    receiver.setStandaloneAudioDevices("Headset microphone", "Headset");
    setParameter(sender,SonobusAudioProcessor::paramMainInMute,1);
    sender.getSourceMixer().setSend(tabChannel.id,true);
    double selectedOnlyPlayback=0;
    for (int block=0;block<240;++block) {
        if (block%8==0) {
            float packet[2048];
            for (int i=0;i<1024;++i) { packet[i*2]=packet[i*2+1]=float(0.12*std::sin(tabPhase)); tabPhase+=2*3.141592653589793*660/48000; }
            if (!sendBrowserFrame(browser,2,packet,sizeof(packet))) return 1;
        }
        juce::AudioBuffer<float> local(2,128), remote(2,128); local.clear(); remote.clear();
        sender.processBlock(local,midi); receiver.processBlock(remote,midi);
        if (block>150) selectedOnlyPlayback+=remote.getRMSLevel(0,0,128);
        const auto now=juce::Time::getMillisecondCounterHiRes(); if (now-tabDeadline>100) tabDeadline=now;
        advanceAudioClock(tabDeadline,128);
    }
    receiver.getPadMonitor().stop(); browser.close(); sender.getSourceMixer().stop(tabChannel.id);
    // No supported virtual output can receive physical input or any mix.
    setParameter(sender,SonobusAudioProcessor::paramDry,1);
    setParameter(sender,SonobusAudioProcessor::paramMainInMute,0);
    for (const auto* name : { "CABLE Input (VB-Audio Virtual Cable)", "BlackHole 2ch", "VB-Cable", "Voicemeeter Input", "Loopback Audio" }) {
        sender.setStandaloneAudioDevices("Headset microphone",name);
        juce::AudioBuffer<float> output(2,128);
        for (int ch=0;ch<2;++ch) for (int i=0;i<128;++i) output.setSample(ch,i,0.4f);
        sender.processBlock(output,midi);
        if (!sender.isStandaloneOutputBlocked() || output.getMagnitude(0,128)!=0) { std::cerr << "FAIL: audio reached virtual output " << name << "\n"; return 1; }
        if (sender.getPadMonitor().startWithDevice(std::make_unique<TestListeningDevice>(),{"Regression",name}).isEmpty()) {
            std::cerr << "FAIL: virtual independent listening accepted\n"; return 1;
        }
    }
    if (completeFeedPlayback<=0.01 || tabInHeadphones<=0.01 || fullPlaybackAfterStop>0.005 || fullListeningAfterStop>0.005 || duplicateSelectedAudio<=0.01 || selectedOnlyPlayback<=0.01) {
        std::cerr << "FAIL: network source listening: full=" << completeFeedPlayback << " ears=" << tabInHeadphones
                  << " afterStop=" << fullPlaybackAfterStop << " duplicate=" << duplicateSelectedAudio << " selectedOnly=" << selectedOnlyPlayback << "\n"; return 1;
    }
    std::cout << "PASS: explicit complete/individual modes, no full-feed fallback after disabling a source, virtual outputs reject audio" << std::endl;
    sender.releaseResources(); receiver.releaseResources();
    std::cout << "PASS: remote pads and audio remain available to physical playback" << std::endl;
    const auto reuseDir = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("pad-reuse-test", "", false);
    if (reuseDir.createDirectory().failed()) return 1;
    {
        SoundboardProcessor boards(roster->getSoundboardProcessor(), reuseDir);
        boards.addSoundboard("A original", true);
        auto* source = boards.addSoundSample("Abertura", padFile.getFile().getFullPathName());
        source->setGain(0.4f);
        source->setButtonColour(0x3366cc);
        boards.editSoundSample(*source);
        boards.addSoundboard("B destino", true);
        if (&boards.getSoundboard(0).getSamples()[0] != source) {
            std::cerr << "FAIL: adding a board invalidated existing sound pointers\n"; return 1;
        }
        if (!boards.addExistingSample(*source, 1) || boards.addExistingSample(*source, 1)
            || boards.addExistingSample(*source, -1) || boards.addExistingSample(*source, 99)) {
            std::cerr << "FAIL: reuse destination/duplicate validation\n"; return 1;
        }
        auto& reused = boards.getSoundboard(1).getSamples()[0];
        if (reused.getFileURL() != source->getFileURL() || reused.getButtonColour() != source->getButtonColour()
            || std::abs(reused.getGain() - 0.4f) > 0.001f) return 1;
        reused.setGain(0.7f); boards.editSoundSample(reused);
        if (std::abs(source->getGain() - 0.4f) > 0.001f) return 1;
    }
    {
        SoundboardProcessor restored(roster->getSoundboardProcessor(), reuseDir);
        if (restored.getNumberOfSoundboards() != 2 || restored.getSoundboard(1).getSamples().size() != 1
            || restored.getSoundboard(1).getSamples()[0].getName() != "Abertura") {
            std::cerr << "FAIL: reused pad did not survive reload\n"; return 1;
        }
    }
    reuseDir.deleteRecursively();
    std::cout << "PASS: reuse pad in another board, independent settings and persistence" << std::endl;
    roster->releaseResources();
    roster.reset(); // A caller may still retain the playback manager.
    player.reset(); // Its file reader must already be detached from the disk thread.
    std::cout << "PASS: stable removal after list reorder and stale action rejection" << std::endl;

    std::cout << "PASS: missing host position, changing audio blocks, finite output, pending invitation status, local UDP audio" << std::endl;
    return 0;
}
