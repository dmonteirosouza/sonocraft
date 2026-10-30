#pragma once
#pragma push_macro("Component")
#undef Component
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"

// Headphone playback is an additional destination. It never changes the
// network send or takes ownership of the primary device.
class SonoCraftListeningControls final : public juce::Component, private juce::Timer, private juce::ChangeListener {
public:
    using DeviceManagerProvider = std::function<juce::AudioDeviceManager*()>;
    SonoCraftListeningControls(SonobusAudioProcessor& p, DeviceManagerProvider provider = {})
        : processor(p), managerProvider(std::move(provider)) {
        for (auto* item : std::initializer_list<juce::Component*> { &enabled, &devices, &refresh, &volume, &hint }) addAndMakeVisible(item);
        enabled.setButtonText(juce::String::fromUTF8("Usar uma segunda saída de áudio"));
        devices.setTextWhenNothingSelected("Escolha o headset ou alto-falante");
        devices.setTitle(juce::String::fromUTF8("Dispositivo da segunda saída"));
        refresh.setButtonText("Atualizar"); refresh.onClick = [this] { scan(); };
        volume.setRange(0, 1, 0.01); volume.setSliderStyle(juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 26);
        volume.setTitle(String::fromUTF8("Volume da segunda saída (dB)"));
        volume.textFromValueFunction = [](double gain) { return gain <= 0 ? String::fromUTF8("−∞ dB") : String(Decibels::gainToDecibels(gain), 1) + " dB"; };
        volume.valueFromTextFunction = [](const String& text) { return text.contains(String::fromUTF8("∞")) ? 0.0 : Decibels::decibelsToGain(text.replaceCharacter(',', '.').getDoubleValue()); };
        volume.setDoubleClickReturnValue(true, 1.0);
        volume.setValue(processor.getPadMonitor().getVolume(), juce::dontSendNotification);
        volume.onValueChange = [this] { processor.getPadMonitor().setVolume(float(volume.getValue())); };
        for (auto* label : { &hint }) {
            label->setMinimumHorizontalScale(1); label->setFont(13.0f);
            label->setJustificationType(juce::Justification::topLeft);
        }
        hint.setFont(12.0f); hint.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        enabled.onClick = [this] {
            if (!enabled.getToggleState()) { processor.getPadMonitor().stop(); showHint(); return; }
            const int index = devices.getSelectedId()-1;
            if (!juce::isPositiveAndBelow(index, outputs.size())) { enabled.setToggleState(false, juce::dontSendNotification); hint.setText(juce::String::fromUTF8("Escolha primeiro o dispositivo da segunda saída."), juce::dontSendNotification); return; }
            const auto error = processor.startListeningOutput(outputs[index]);
            if (error.isNotEmpty()) { enabled.setToggleState(false, juce::dontSendNotification); hint.setText(error, juce::dontSendNotification); return; }
            auto& state = processor.getValueTreeState().state;
            state.setProperty("listeningOutputName", outputs[index].name, nullptr);
            state.setProperty("listeningOutputType", outputs[index].type, nullptr); showHint();
        };
        devices.onChange = [this] {
            if (processor.getPadMonitor().isActive()) processor.getPadMonitor().stop();
            enabled.setToggleState(false, juce::dontSendNotification); showHint();
        };
        if (auto* manager = managerProvider ? managerProvider() : nullptr) manager->addChangeListener(this);
        scan(); startTimer(250);
    }
    ~SonoCraftListeningControls() override {
        stopTimer();
        if (auto* manager = managerProvider ? managerProvider() : nullptr) manager->removeChangeListener(this);
    }
    void resized() override {
        auto box = getLocalBounds();
        enabled.setBounds(box.removeFromTop(28));
        auto row = box.removeFromTop(32); refresh.setBounds(row.removeFromRight(76)); row.removeFromRight(6); devices.setBounds(row);
        volume.setBounds(box.removeFromTop(32)); hint.setBounds(box);
    }
    juce::String audioDiagnostic() {
        juce::String text = "SonoCraft " JucePlugin_VersionString " / audio diagnostic 5\n";
        const auto line = [&text](const char* key, const juce::String& value) { text += juce::String(key) + ": " + value + "\n"; };
        const auto quoted = [](const juce::String& value) { return juce::JSON::toString(juce::var(value), true); };
        const auto flag = [this,&line](const char* key, const juce::String& parameter) {
            line(key, processor.getValueTreeState().getRawParameterValue(parameter)->load()>0.5f ? "on" : "off");
        };
        line("time",juce::Time::getCurrentTime().toISO8601(true));
        if (auto* manager = managerProvider ? managerProvider() : nullptr) {
            const auto setup = manager->getAudioDeviceSetup();
            line("configured_input",setup.inputDeviceName);
            line("configured_output",setup.outputDeviceName);
            line("audio_backend",manager->getCurrentAudioDeviceType());
            if (auto* device = manager->getCurrentAudioDevice()) {
                line("device_playing",device->isPlaying() ? "yes" : "no");
                line("sample_rate",juce::String(device->getCurrentSampleRate()));
                line("buffer_samples",juce::String(device->getCurrentBufferSizeSamples()));
                line("input_channels",juce::String(device->getActiveInputChannels().countNumberOfSetBits()));
                line("output_channels",juce::String(device->getActiveOutputChannels().countNumberOfSetBits()));
            } else line("device_playing","no device");
        }
        line("engine_input",processor.getStandaloneInputName());
        line("engine_output",processor.getStandaloneOutputName());
        line("virtual_output_protection",processor.isStandaloneOutputBlocked() ? "blocked" : "clear");
        flag("input_muted",SonobusAudioProcessor::paramMainInMute);
        flag("network_send_muted",SonobusAudioProcessor::paramMainSendMute);
        flag("network_receive_muted",SonobusAudioProcessor::paramMainRecvMute);
        flag("pads_to_connections",SonobusAudioProcessor::paramSendSoundboardAudio);
        const auto sources = processor.getSourceMixer().views();
        line("source_mixer_channels", juce::String(sources.size()));
        line("source_mode", processor.getSourceMixer().usesIndividualSources() ? "individual" : "complete_input");
        for (int i=0;i<sources.size();++i) {
            const auto& s=sources[i];
            text += "source_"+juce::String(i+1)+": type="+(s.tab?"tab":"app")
                +" name="+quoted(s.name)+" application="+quoted(s.applicationName)
                +" active="+(s.active?"yes":"no")+" send="+(s.sending?"on":"off")
                +" capture_requested="+(s.requested?"yes":"no")
                +" meter_origin="+(s.systemMeter?"application_playback":s.active?"captured_audio":"none")
                +" muted="+(s.muted?"yes":"no")+" gain="+juce::String(s.gain,2)
                +" source_level="+juce::String(juce::Decibels::gainToDecibels(s.level,-100.0f),1)+" dBFS"
                +" status="+quoted(s.status)+" meter_status="+quoted(s.meterStatus)+"\n";
        }
        line("system_input_suppressed", processor.isSystemInputSuppressed() ? "yes" : "no");
        line("local_capture_active",processor.getSourceMixer().anyActive() ? "yes" : "no");
        auto& monitor = processor.getPadMonitor();
        line("listening_active",monitor.isActive() ? "yes" : "no");
        line("listening_device",monitor.getDeviceName());
        line("listening_backend",monitor.getDeviceType());
        const auto peak = [](foleys::LevelMeterSource& meter) {
            float value = 0;
            for (int ch=0;ch<meter.getNumChannels();++ch) value=juce::jmax(value,meter.getMaxLevel(ch));
            return juce::String(juce::Decibels::gainToDecibels(value,-100.0f),1)+" dBFS";
        };
        line("input_meter",peak(processor.getInputMeterSource()));
        line("network_send_meter",peak(processor.getSendMeterSource()));
        line("output_meter",peak(processor.getOutputMeterSource()));
        line("listening_meter",peak(processor.getListeningMeterSource()));
        const auto peers = processor.getMixerChannelStates();
        line("connections",juce::String(peers.size()));
        for (int i=0;i<peers.size();++i) {
            text += "connection_"+juce::String(i+1)+": muted="+(peers[i].muted ? "yes" : "no")
                +" name="+quoted(peers[i].name)
                +" receiving="+(processor.getRemotePeerRecvActive(i) ? "yes" : "no")
                +" channels="+juce::String(processor.getRemotePeerRecvChannelCount(i))
                +" send_enabled="+(processor.getRemotePeerSendAllow(i)?"yes":"no")
                +" send_active="+(processor.getRemotePeerSendActive(i)?"yes":"no")
                +" packets_sent="+juce::String(processor.getRemotePeerPacketsSent(i))
                +" packets_received="+juce::String(processor.getRemotePeerPacketsReceived(i))+"\n";
        }
        return text;
    }
private:
    SonobusAudioProcessor& processor;
    DeviceManagerProvider managerProvider;
    juce::ToggleButton enabled;
    juce::ComboBox devices;
    juce::TextButton refresh;
    juce::Slider volume;
    juce::Label hint;
    juce::Array<SonoCraftPadMonitor::Device> outputs;
    void showHint() {
        hint.setText(juce::String::fromUTF8(processor.getPadMonitor().isActive()
            ? "Segunda saída ligada: reproduz os participantes e os pads, sem sua entrada. O envio aos conectados não muda."
            : "Opcional: use outro fone ou alto-falante. A saída principal já reproduz os participantes e os pads. Escolha dispositivos diferentes para não ouvir o som duas vezes."), juce::dontSendNotification);
    }
    void scan() {
        const auto previous = devices.getText(); outputs = processor.getPadMonitor().devices(); devices.clear(juce::dontSendNotification);
        const auto savedName = processor.getValueTreeState().state.getProperty("listeningOutputName").toString();
        const auto savedType = processor.getValueTreeState().state.getProperty("listeningOutputType").toString();
        for (int i = 0; i < outputs.size(); ++i) {
            const auto label = outputs[i].name + " (" + outputs[i].type + ")"; devices.addItem(label, i+1);
            if (label == previous || (previous.isEmpty() && outputs[i].name == savedName && outputs[i].type == savedType)) devices.setSelectedId(i+1, juce::dontSendNotification);
        }
        showHint();
    }
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        auto& monitor = processor.getPadMonitor();
        if (auto* manager = managerProvider ? managerProvider() : nullptr) {
            if (monitor.isActive() && monitor.getDeviceName().equalsIgnoreCase(manager->getAudioDeviceSetup().outputDeviceName)) {
                monitor.stop();
                hint.setText(juce::String::fromUTF8("Este dispositivo passou a ser a saída principal. A segunda saída foi desligada para evitar som duplicado."), juce::dontSendNotification);
            }
        }
    }
    void timerCallback() override {
        if (!isShowing()) return;
        auto& monitor = processor.getPadMonitor(); enabled.setToggleState(monitor.isActive(), juce::dontSendNotification);
        if (!volume.isMouseButtonDown()) volume.setValue(monitor.getVolume(), juce::dontSendNotification);
        if (monitor.isActive()) for (int i = 0; i < outputs.size(); ++i)
            if (outputs[i].name == monitor.getDeviceName() && outputs[i].type == monitor.getDeviceType()) devices.setSelectedId(i+1, juce::dontSendNotification);
    }
};
#pragma pop_macro("Component")
