// SonoCraft regression coverage: no audio device or remote server required.
#include "../Source/SonobusPluginProcessor.h"
#include "../Source/SoundboardChannelProcessor.h"
#include "../Source/SoundboardProcessor.h"
#include "../Source/SonoCraftBrowserAudioBridge.h"
#include <cmath>
#include <iostream>
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

class EmptyPlayHead final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override { return {}; }
};

int main()
{
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
        set(SonobusAudioProcessor::paramMeetingMode, 0);
        set(SonobusAudioProcessor::paramMetEnabled, 0);
        set(SonobusAudioProcessor::paramMainReverbEnabled, 0);
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
    double meetingRemoteEnergy = 0.0, meetingVoiceEnergy = 0.0, privateAudioLeak = 0.0;
    double listeningVoiceLeak = 0.0, listeningRemoteEnergy = 0.0;
    double receiverMeetingSum = 0.0;
    int receiverMeetingSamples = 0;
    auto setMeeting = [](SonobusAudioProcessor& processor, bool enabled) {
        auto* parameter = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMeetingMode);
        parameter->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
    };
    // Legacy remote-audio test explicitly opts into sharing general received audio.
    sender.getValueTreeState().getParameter(SonobusAudioProcessor::paramMeetingIncludeReceived)->setValueNotifyingHost(1);
    receiver.getValueTreeState().getParameter(SonobusAudioProcessor::paramMeetingIncludeReceived)->setValueNotifyingHost(1);
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
        if (block == 1200 || block == 1300) {
            setMeeting(sender, block == 1200);
            setMeeting(receiver, block == 1200);
        }
        if (block == 1230 || block == 1260)
            receiver.getValueTreeState().getParameter(SonobusAudioProcessor::paramMeetingIncludeReceived)->setValueNotifyingHost(block == 1260 ? 1 : 0);
        sender.processBlock(input, midi);
        if (block > 1220 && block < 1280)
            for (int channel = 0; channel < input.getNumChannels(); ++channel)
                for (int sample = 0; sample < samples; ++sample)
                    meetingVoiceEnergy += input.getSample(channel, sample) * input.getSample(channel, sample);
        // Windows microphone is distinct from the remote sine: a DC signal
        // lets us verify the meeting mix contains it while listening does not.
        if (block >= 1250 && block < 1300)
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < samples; ++sample) output.setSample(channel, sample, 0.02f);
        receiver.processBlock(output, midi);
        if (block > 1220 && block < 1280) {
            const float localListening = sender.getListeningMeterSource().getMaxLevel(0);
            const float remoteListening = receiver.getListeningMeterSource().getMaxLevel(0);
            if (block < 1245) listeningVoiceLeak += localListening * localListening;
            listeningRemoteEnergy += remoteListening * remoteListening;
            if (block > 1260) for (int sample = 0; sample < samples; ++sample) { receiverMeetingSum += output.getSample(0, sample); ++receiverMeetingSamples; }
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
                if (block > 1220 && block < 1280) meetingRemoteEnergy += value * value;
                if (block > 1240 && block < 1250) privateAudioLeak += value * value;
            }
        // Model an audio device clock, rather than adding a relative sleep to
        // processing time. macOS CI can coalesce nanosleep by tens of ms, which
        // otherwise simulates a device running far below its declared rate.
        // Hosted runners can suspend this process for seconds. A hardware
        // callback cannot catch up by sending hundreds of blocks instantly:
        // that starves the UDP worker and invalidates the recovery windows.
        // Rebase only after a scheduler stall; preserve real audio assertions.
        const auto callbackNow = juce::Time::getMillisecondCounterHiRes();
        if (callbackNow - nextCallbackAt > 100.0) {
            std::cout << "Audio clock rebased after scheduler stall of "
                      << callbackNow - nextCallbackAt << " ms" << std::endl;
            nextCallbackAt = callbackNow;
        }
        nextCallbackAt += samples * 1000.0 / 48000.0;
        waitForAudioDeadline(nextCallbackAt);
    }
    if (privateAudioLeak > 0.000001 || mutedEnergy > 0.000001 || resumedEnergy <= 0.001
        || meetingRemoteEnergy <= 0.001 || meetingVoiceEnergy <= 0.001
        || listeningVoiceLeak > 0.000001 || listeningRemoteEnergy <= 0.001
        || receiverMeetingSamples == 0 || std::abs(receiverMeetingSum/receiverMeetingSamples - 0.02) > 0.005) {
        std::cerr << "FAIL: mute=" << mutedEnergy << ", unmute=" << resumedEnergy
                  << ", meeting remote=" << meetingRemoteEnergy << ", meeting voice=" << meetingVoiceEnergy << ", total=" << receivedEnergy << "\n";
        for (auto& state : receiver.getMixerChannelStates())
            std::cerr << "receiver muted=" << state.muted << ", solo=" << state.soloed << ", gain=" << state.gain
                      << ", connected=" << state.connected << ", packets=" << receiver.getRemotePeerPacketsReceived(0)
                      << ", active=" << receiver.getRemotePeerRecvActive(0) << ", channels=" << receiver.getRemotePeerRecvChannelCount(0) << "\n";
        return 1;
    }
    std::cout << "PASS: immediate participant mute, unmute recovery, separate listening and meeting mixes, private remote audio blocked until opted in, local voice excluded from listening" << std::endl;
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
    // Play a real pad file into the meeting mix with no microphone signal.
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
    setMeeting(*roster, true);
    (*player)->play();
    double padEnergy = 0, listeningOnMeetingEnergy = 0, listeningOffMeetingEnergy = 0, headphonePadEnergy = 0;
    TestListeningDevice* localHeadphones = nullptr;
    for (int block = 0; block < 100; ++block) {
        juce::AudioBuffer<float> padOutput(2, 128);
        padOutput.clear();
        if (block == 30) {
            auto device = std::make_unique<TestListeningDevice>(); localHeadphones = device.get();
            if (roster->getPadMonitor().startWithDevice(std::move(device), {"Regression", "Headset fixture"}).isNotEmpty()) return 1;
        }
        if (block == 60) { roster->getPadMonitor().stop(); localHeadphones = nullptr; }
        if (block == 70) roster->setStandaloneAudioDevices("VB-Cable", "VB-Cable");
        if (block == 80) roster->setStandaloneAudioDevices("VB-Cable", "Headset");
        roster->processBlock(padOutput, midi);
        if (block >= 70 && block < 80 && padOutput.getMagnitude(0, 128) != 0) { std::cerr << "FAIL: virtual cable fed its own capture input\n"; return 1; }
        if (block > 40 && block < 55) listeningOnMeetingEnergy += padOutput.getRMSLevel(0,0,128);
        if (block > 10 && block < 25) listeningOffMeetingEnergy += padOutput.getRMSLevel(0,0,128);
        if (localHeadphones) { juce::AudioBuffer<float> ears(2,128); localHeadphones->render(ears); headphonePadEnergy += ears.getRMSLevel(0,0,128); }
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < 128; ++sample)
                padEnergy += padOutput.getSample(channel, sample) * padOutput.getSample(channel, sample);
    }
    (*player)->unload();
    if (roster->getListeningMeterSource().getMaxLevel(0) <= 0.001f) {
        std::cerr << "FAIL: local pad missing from headphone mix\n"; return 1;
    }
    if (listeningOnMeetingEnergy <= 0.001 || listeningOffMeetingEnergy <= 0.001 || headphonePadEnergy <= 0.001) { std::cerr << "FAIL: headphone toggle interrupted meeting audio\n"; return 1; }
    if (padEnergy <= 0.001) { std::cerr << "FAIL: no Soundboard in meeting output\n"; return 1; }
    std::cout << "PASS: local pad reaches meeting with headphones on/off; virtual cable loop output is blocked" << std::endl;
    // Real source pad -> UDP receiver -> meeting output and listening bus.
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
    setMeeting(receiver, true);
    setParameter(receiver, SonobusAudioProcessor::paramMeetingIncludeReceived, 0);
    SoundSample remotePad("Remote pad", juce::URL(padFile.getFile()), SoundSample::LOOP_AT_END);
    auto remotePlayer = sender.getSoundboardProcessor()->loadSample(remotePad);
    if (!remotePlayer) return 1;
    (*remotePlayer)->play();
    double remotePadEnergy = 0, remotePadMean = 0, remoteMeetingWithEars = 0, remoteMeetingWithoutEars = 0, remoteHeadphoneEnergy = 0;
    TestListeningDevice* remoteHeadphones = nullptr;
    auto padDeadline = juce::Time::getMillisecondCounterHiRes();
    for (int block = 0; block < 480; ++block) {
        juce::AudioBuffer<float> source(2,128), destination(2,128);
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < 128; ++sample) source.setSample(channel, sample, 0.6f);
        // Unmute the source voice while its pad keeps playing: the receiver
        // must still forward only pad channels, not the general input stream.
        if (block == 240) setParameter(sender, SonobusAudioProcessor::paramMainInMute, 0);
        destination.clear();
        if (block == 120) {
            auto device = std::make_unique<TestListeningDevice>(); remoteHeadphones = device.get();
            if (receiver.getPadMonitor().startWithDevice(std::move(device), {"Regression", "Headset fixture"}).isNotEmpty()) return 1;
        }
        if (block == 300) { receiver.getPadMonitor().stop(); remoteHeadphones = nullptr; }
        sender.processBlock(source, midi); receiver.processBlock(destination, midi);
        if (block > 180 && block < 230) remoteMeetingWithEars += destination.getRMSLevel(0,0,128);
        if (block > 330 && block < 380) remoteMeetingWithoutEars += destination.getRMSLevel(0,0,128);
        if (remoteHeadphones) { juce::AudioBuffer<float> ears(2,128); remoteHeadphones->render(ears); remoteHeadphoneEnergy += ears.getRMSLevel(0,0,128); }
        if (block > 300) for (int sample = 0; sample < 128; ++sample) {
            auto value = destination.getSample(0, sample);
            remotePadEnergy += value * value; remotePadMean += value;
        }
        const auto padNow = juce::Time::getMillisecondCounterHiRes();
        if (padNow - padDeadline > 100.0) padDeadline = padNow;
        padDeadline += 128 * 1000.0 / 48000.0;
        waitForAudioDeadline(padDeadline);
    }
    if (remoteMeetingWithEars <= 0.001 || remoteMeetingWithoutEars <= 0.001 || remoteHeadphoneEnergy <= 0.001) { std::cerr << "FAIL: remote pad lost when toggling headphones or restoring mono settings\n"; return 1; }
    if (remotePadEnergy <= 0.001 || std::abs(remotePadMean/(179*128)) > 0.01
        || receiver.getListeningMeterSource().getMaxLevel(0) <= 0.001f) {
        std::cerr << "FAIL: remote pad/mic-mute/listening routing: energy=" << remotePadEnergy << " mean=" << remotePadMean/(179*128) << "\n"; return 1;
    }
    (*remotePlayer)->unload();
    sender.getSoundboardProcessor()->clearPlayback(); remotePlayer.reset();
    // Browser tab IPC is tested with the same authenticated audio endpoint the
    // Chrome helper uses; no actual user window or microphone is captured.
    const juce::URL tabUrl(sender.getApplicationAudio().startBrowserTab());
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
        if (readHttpHeaders(unauthorized).contains("101 Switching") || sender.getApplicationAudio().isActive()) { std::cerr << "FAIL: cross-origin tab capture accepted\n"; return 1; }
    }
    juce::StreamingSocket browser;
    if (!browser.connect("127.0.0.1",tabUrl.getPort(),3000)) return 1;
    browser.write(request.toRawUTF8(),int(request.getNumBytesAsUTF8()));
    const auto headers=readHttpHeaders(browser);
    if (!headers.contains("101 Switching Protocols") || !headers.contains("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=")) { std::cerr << "FAIL: browser tab socket upgrade\n"; return 1; }
    const juce::String rateMessage="rate=48000";
    sendBrowserFrame(browser,1,rateMessage.toRawUTF8(),int(rateMessage.getNumBytesAsUTF8()));
    const auto activationDeadline=juce::Time::getMillisecondCounterHiRes()+3000;
    while (!sender.getApplicationAudio().isActive() && juce::Time::getMillisecondCounterHiRes()<activationDeadline) juce::Thread::sleep(1);
    if (!sender.getApplicationAudio().isActive()) { std::cerr << "FAIL: authenticated tab capture did not activate\n"; return 1; }
    setMeeting(sender,true); setMeeting(receiver,true);
    sender.setStandaloneAudioDevices("VB-Cable",{});
    sender.protectSelectedSourceInput();
    // Even an accidental manual unmute must not forward the aggregate cable
    // while a selected-source capture is requested.
    setParameter(sender,SonobusAudioProcessor::paramMainInMute,0);
    setParameter(sender,SonobusAudioProcessor::paramSendApplicationAudio,1);
    setParameter(sender,SonobusAudioProcessor::paramApplicationToMeeting,1);
    setParameter(receiver,SonobusAudioProcessor::paramReceivedApplicationsToMeeting,0);
    double privateTabLeak=0, sharedTabEnergy=0, tabInHeadphones=0, stoppedTabEnergy=0, localTabMeetingEnergy=0, aggregateLeak=0;
    auto earsDevice=std::make_unique<TestListeningDevice>(); auto* tabEars=earsDevice.get();
    receiver.getPadMonitor().startWithDevice(std::move(earsDevice),{"Regression","Headset fixture"});
    double tabPhase=0; auto tabDeadline=juce::Time::getMillisecondCounterHiRes();
    for (int block=0;block<500;++block) {
        if (block%8==0) {
            float packet[2048];
            for (int i=0;i<1024;++i) { packet[i*2]=packet[i*2+1]=float(0.12*std::sin(tabPhase)); tabPhase+=2*3.141592653589793*660/48000; }
            if (!sendBrowserFrame(browser,2,packet,sizeof(packet))) return 1;
        }
        if (block==160) setParameter(receiver,SonobusAudioProcessor::paramReceivedApplicationsToMeeting,1);
        if (block==320) setParameter(sender,SonobusAudioProcessor::paramSendApplicationAudio,0);
        juce::AudioBuffer<float> local(2,128), remote(2,128), ears(2,128); local.clear(); remote.clear();
        for (int ch=0;ch<2;++ch) for (int sample=0;sample<128;++sample) local.setSample(ch,sample,0.4f);
        sender.processBlock(local,midi); receiver.processBlock(remote,midi); tabEars->render(ears);
        if (block>80 && block<130) { privateTabLeak+=remote.getRMSLevel(0,0,128); tabInHeadphones+=ears.getRMSLevel(0,0,128); }
        if (block>230 && block<280) { sharedTabEnergy+=remote.getRMSLevel(0,0,128); localTabMeetingEnergy+=local.getRMSLevel(0,0,128); for (int sample=0;sample<128;++sample) aggregateLeak+=local.getSample(0,sample)/(49*128); }
        if (block>440) stoppedTabEnergy+=remote.getRMSLevel(0,0,128);
        const auto tabNow=juce::Time::getMillisecondCounterHiRes(); if (tabNow-tabDeadline>100) tabDeadline=tabNow;
        tabDeadline+=128*1000.0/48000.0; waitForAudioDeadline(tabDeadline);
    }
    receiver.getPadMonitor().stop(); browser.close(); sender.getApplicationAudio().stop();
    {
        juce::AudioBuffer<float> aggregate(2,128);
        for (int ch=0;ch<2;++ch) for (int i=0;i<128;++i) aggregate.setSample(ch,i,0.4f);
        sender.processBlock(aggregate,midi);
        if (!sender.isAggregateInputProtected() || aggregate.getMagnitude(0,128)>0.00001f) { std::cerr << "FAIL: stopping selected capture re-enabled aggregate audio\n"; return 1; }
        sender.setStandaloneAudioDevices("Headset microphone","CABLE Input");
        for (int ch=0;ch<2;++ch) for (int i=0;i<128;++i) aggregate.setSample(ch,i,0.4f);
        sender.processBlock(aggregate,midi);
        if (aggregate.getRMSLevel(0,0,128)<0.1f) { std::cerr << "FAIL: virtual-input protection muted a physical microphone\n"; return 1; }
        sender.allowAggregateInputAfterExplicitSetup();
        if (sender.isAggregateInputProtected()) { std::cerr << "FAIL: explicit aggregate setup did not release protection\n"; return 1; }
    }
    if (privateTabLeak>0.00001 || sharedTabEnergy<=0.01 || tabInHeadphones<=0.01 || stoppedTabEnergy>0.00001 || localTabMeetingEnergy<=0.01 || std::abs(aggregateLeak)>0.01) {
        std::cerr << "FAIL: selected tab destination routing: private=" << privateTabLeak << " meeting=" << sharedTabEnergy << " headphones=" << tabInHeadphones << " stopped=" << stoppedTabEnergy << " local=" << localTabMeetingEnergy << "\n"; return 1;
    }
    std::cout << "PASS: authenticated browser audio, private listening, opt-in local/remote meeting, independent network stop" << std::endl;
    sender.releaseResources(); receiver.releaseResources();
    std::cout << "PASS: remote pads reach meeting and headphones while unmuted remote voice stays private" << std::endl;
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
