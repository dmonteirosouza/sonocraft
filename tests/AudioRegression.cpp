// SonoCraft regression coverage: no audio device or remote server required.
#include "../Source/SonobusPluginProcessor.h"
#include <cmath>
#include <iostream>

class EmptyPlayHead final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override { return {}; }
};

int main()
{
    std::cout << "Starting audio regression" << std::endl;
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    // Audio processors contain large DSP buffers. Windows console programs
    // have a smaller default stack than macOS, so mirror the application's
    // factory ownership and keep the two processors on the heap.
    auto senderOwner = std::make_unique<SonobusAudioProcessor>();
    auto receiverOwner = std::make_unique<SonobusAudioProcessor>();
    auto& sender = *senderOwner;
    auto& receiver = *receiverOwner;
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
    double receivedEnergy = 0.0;
    const int blockSizes[] = {128, 1024, 64, 256, 512, 128};
    const auto startedAt = juce::Time::getMillisecondCounterHiRes();
    auto nextCallbackAt = startedAt;
    for (int block = 0; block < 1000; ++block)
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
        sender.processBlock(input, midi);
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
            }
        // Model an audio device clock, rather than adding a relative sleep to
        // processing time. macOS CI can coalesce nanosleep by tens of ms, which
        // otherwise simulates a device running far below its declared rate.
        nextCallbackAt += samples * 1000.0 / 48000.0;
        while (juce::Time::getMillisecondCounterHiRes() < nextCallbackAt)
            juce::Thread::yield();
    }
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
    std::cout << "PASS: missing host position, changing audio blocks, finite output, pending invitation status, local UDP audio" << std::endl;
    return 0;
}
