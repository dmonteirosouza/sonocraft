// SonoCraft regression coverage: no audio device or remote server required.
#include "../Source/SonobusPluginProcessor.h"
#include "../Source/SoundboardChannelProcessor.h"
#include "../Source/SoundboardProcessor.h"
#include <cmath>
#include <iostream>

class EmptyPlayHead final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override { return {}; }
};

int main()
{
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
    double meetingRemoteEnergy = 0.0, meetingVoiceEnergy = 0.0;
    auto setMeeting = [](SonobusAudioProcessor& processor, bool enabled) {
        auto* parameter = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMeetingMode);
        parameter->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
    };
    const int blockSizes[] = {128, 1024, 64, 256, 512, 128};
    const auto startedAt = juce::Time::getMillisecondCounterHiRes();
    auto nextCallbackAt = startedAt;
    for (int block = 0; block < 1400; ++block)
    {
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
        sender.processBlock(input, midi);
        if (block > 1220 && block < 1280)
            for (int channel = 0; channel < input.getNumChannels(); ++channel)
                for (int sample = 0; sample < samples; ++sample)
                    meetingVoiceEnergy += input.getSample(channel, sample) * input.getSample(channel, sample);
        receiver.processBlock(output, midi);
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
            }
        // Model an audio device clock, rather than adding a relative sleep to
        // processing time. macOS CI can coalesce nanosleep by tens of ms, which
        // otherwise simulates a device running far below its declared rate.
        nextCallbackAt += samples * 1000.0 / 48000.0;
        while (juce::Time::getMillisecondCounterHiRes() < nextCallbackAt)
            juce::Thread::yield();
    }
    if (mutedEnergy > 0.000001 || resumedEnergy <= 0.001
        || meetingRemoteEnergy > 0.000001 || meetingVoiceEnergy <= 0.001) {
        std::cerr << "FAIL: mute=" << mutedEnergy << ", unmute=" << resumedEnergy
                  << ", meeting remote=" << meetingRemoteEnergy << ", meeting voice=" << meetingVoiceEnergy << ", total=" << receivedEnergy << "\n";
        for (auto& state : receiver.getMixerChannelStates())
            std::cerr << "receiver muted=" << state.muted << ", solo=" << state.soloed << ", gain=" << state.gain
                      << ", connected=" << state.connected << ", packets=" << receiver.getRemotePeerPacketsReceived(0)
                      << ", active=" << receiver.getRemotePeerRecvActive(0) << ", channels=" << receiver.getRemotePeerRecvChannelCount(0) << "\n";
        return 1;
    }
    std::cout << "PASS: immediate participant mute, unmute recovery, meeting voice without remote return" << std::endl;
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
    double padEnergy = 0;
    for (int block = 0; block < 100; ++block) {
        juce::AudioBuffer<float> padOutput(2, 128);
        padOutput.clear();
        roster->processBlock(padOutput, midi);
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < 128; ++sample)
                padEnergy += padOutput.getSample(channel, sample) * padOutput.getSample(channel, sample);
    }
    (*player)->unload();
    if (padEnergy <= 0.001) { std::cerr << "FAIL: no Soundboard in meeting output\n"; return 1; }
    std::cout << "PASS: real Soundboard pad reaches meeting output" << std::endl;
    roster->releaseResources();
    roster.reset(); // A caller may still retain the playback manager.
    player.reset(); // Its file reader must already be detached from the disk thread.
    std::cout << "PASS: stable removal after list reorder and stale action rejection" << std::endl;

    std::cout << "PASS: missing host position, changing audio blocks, finite output, pending invitation status, local UDP audio" << std::endl;
    return 0;
}
