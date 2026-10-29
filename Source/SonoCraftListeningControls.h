#pragma once
#pragma push_macro("Component")
#undef Component
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"

// Headphone playback is an additional destination. It never changes the
// network send or takes ownership of the primary device.
class SonoCraftListeningControls final : public juce::Component, private juce::Timer {
public:
    using DeviceManagerProvider = std::function<juce::AudioDeviceManager*()>;
    SonoCraftListeningControls(SonobusAudioProcessor& p, bool fullPage = false, DeviceManagerProvider provider = {})
        : processor(p), full(fullPage), managerProvider(std::move(provider)) {
        for (auto* item : std::initializer_list<juce::Component*> { &enabled, &devices, &refresh, &volume, &hint,
                &instructions, &inputDevices, &inputLabel, &mainDevices, &mainLabel,
                &apply, &routingStatus, &configure, &diagnostic, &receive, &micMute, &headphonesLabel })
            addAndMakeVisible(item);
        instructions.setText(juce::String::fromUTF8("Escolha a entrada para transmitir aos conectados e uma saída física para ouvir. Pads e fontes são compartilhados somente entre usuários do SonoCraft."), juce::dontSendNotification);
        apply.setButtonText(juce::String::fromUTF8("Aplicar dispositivos e saída"));
        apply.onClick = [this] { applySetup(); };
        configure.setButtonText(juce::String::fromUTF8("Dispositivos e opções avançadas…"));
        configure.onClick = [this] { if (onChooseAudio) onChooseAudio(); };
        diagnostic.setButtonText(juce::String::fromUTF8("Copiar diagnóstico de áudio"));
        diagnostic.setTooltip(juce::String::fromUTF8("Copia dispositivos, controles e níveis atuais para você compartilhar no suporte. Não grava áudio nem copia nomes de abas ou endereços das conexões."));
        diagnostic.onClick = [this] {
            juce::SystemClipboard::copyTextToClipboard(audioDiagnostic());
            diagnostic.setButtonText(juce::String::fromUTF8("Diagnóstico copiado"));
        };
        receive.setButtonText(juce::String::fromUTF8("Receber áudio dos conectados"));
        receive.onClick = [this] { setParameter(SonobusAudioProcessor::paramMainRecvMute, receive.getToggleState() ? 0 : 1); };
        micMute.setButtonText("Silenciar minha entrada (mantendo os pads)");
        micMuteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(), SonobusAudioProcessor::paramMainInMute, micMute);
        headphonesLabel.setText(juce::String::fromUTF8("3. Escuta adicional (opcional, sem sua própria voz)"), juce::dontSendNotification);
        enabled.setButtonText(juce::String::fromUTF8("Ouvir áudio recebido e pads"));
        devices.setTextWhenNothingSelected("Escolha o headset ou alto-falante");
        devices.setTitle(juce::String::fromUTF8("Dispositivo de escuta"));
        refresh.setButtonText("Atualizar"); refresh.onClick = [this] { scan(); scanMainDevices(); };
        volume.setRange(0, 1, 0.01); volume.setSliderStyle(juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 24);
        volume.setTitle("Volume da escuta");
        volume.setValue(processor.getPadMonitor().getVolume(), juce::dontSendNotification);
        volume.onValueChange = [this] { processor.getPadMonitor().setVolume(float(volume.getValue())); };
        for (auto* label : { &instructions, &inputLabel, &mainLabel, &headphonesLabel, &routingStatus, &hint }) {
            label->setMinimumHorizontalScale(1); label->setFont(13.0f);
            label->setJustificationType(juce::Justification::topLeft);
        }
        hint.setFont(12.0f); hint.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        enabled.onClick = [this] {
            if (!enabled.getToggleState()) { processor.getPadMonitor().stop(); showHint(); return; }
            const int index = devices.getSelectedId()-1;
            if (!juce::isPositiveAndBelow(index, outputs.size())) { enabled.setToggleState(false, juce::dontSendNotification); hint.setText("Escolha primeiro o dispositivo de escuta.", juce::dontSendNotification); return; }
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
        updateRoleLabels(); scan(); scanMainDevices(); startTimer(250);
    }
    ~SonoCraftListeningControls() override { stopTimer(); }
    std::function<void()> onChooseAudio;
    void resized() override {
        auto box = getLocalBounds().reduced(full ? 14 : 0);
        for (auto* item : std::initializer_list<juce::Component*> { &instructions, &inputDevices,
                &inputLabel, &mainDevices, &mainLabel, &apply, &routingStatus, &configure, &receive, &micMute,
                &headphonesLabel, &diagnostic }) item->setVisible(full);
        if (full) {
            instructions.setBounds(box.removeFromTop(76));
            inputLabel.setBounds(box.removeFromTop(24)); inputDevices.setBounds(box.removeFromTop(32)); box.removeFromTop(12);
            mainLabel.setBounds(box.removeFromTop(24)); mainDevices.setBounds(box.removeFromTop(32)); box.removeFromTop(12);
            apply.setBounds(box.removeFromTop(36)); box.removeFromTop(8);
            routingStatus.setBounds(box.removeFromTop(94));
            configure.setBounds(box.removeFromTop(32)); box.removeFromTop(6);
            diagnostic.setBounds(box.removeFromTop(32)); box.removeFromTop(12);
            receive.setBounds(box.removeFromTop(28)); micMute.setBounds(box.removeFromTop(28));
            headphonesLabel.setBounds(box.removeFromTop(24));
        }
        enabled.setBounds(box.removeFromTop(28));
        auto row = box.removeFromTop(32); refresh.setBounds(row.removeFromRight(76)); row.removeFromRight(6); devices.setBounds(row);
        volume.setBounds(box.removeFromTop(32)); hint.setBounds(box.removeFromTop(62));
    }
private:
    SonobusAudioProcessor& processor;
    const bool full;
    DeviceManagerProvider managerProvider;
    juce::ToggleButton enabled, receive, micMute;
    juce::ComboBox inputDevices, mainDevices, devices;
    juce::TextButton apply, refresh, configure, diagnostic;
    juce::Slider volume;
    juce::Label instructions, inputLabel, mainLabel, routingStatus, headphonesLabel, hint;
    juce::Array<SonoCraftPadMonitor::Device> outputs;
    juce::StringArray inputs, mainOutputs;
    juce::String applyError;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> micMuteAttachment;
    juce::String audioDiagnostic() {
        juce::String text = "SonoCraft " JucePlugin_VersionString " / audio diagnostic 3\n";
        const auto line = [&text](const char* key, const juce::String& value) { text += juce::String(key) + ": " + value + "\n"; };
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
                +" active="+(s.active?"yes":"no")+" send="+(s.sending?"on":"off")
                +" muted="+(s.muted?"yes":"no")+" gain="+juce::String(s.gain,2)+"\n";
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
        line("output_meter",peak(processor.getOutputMeterSource()));
        line("listening_meter",peak(processor.getListeningMeterSource()));
        const auto peers = processor.getMixerChannelStates();
        line("connections",juce::String(peers.size()));
        for (int i=0;i<peers.size();++i) {
            text += "connection_"+juce::String(i+1)+": muted="+(peers[i].muted ? "yes" : "no")
                +" receiving="+(processor.getRemotePeerRecvActive(i) ? "yes" : "no")
                +" channels="+juce::String(processor.getRemotePeerRecvChannelCount(i))+"\n";
        }
        return text;
    }
    void setParameter(const juce::String& id, float value) {
        auto* parameter = processor.getValueTreeState().getParameter(id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
    void updateRoleLabels() {
        inputLabel.setText(juce::String::fromUTF8("1. Entrada (microfone ou captura do sistema)"), juce::dontSendNotification);
        mainLabel.setText(juce::String::fromUTF8("2. Saída local (fone, alto-falante ou Sem saída)"), juce::dontSendNotification);
        applyError.clear(); updateStatus();
    }
    void scanMainDevices() {
        if (!full) return;
        auto* manager = managerProvider ? managerProvider() : nullptr;
        if (!manager || !manager->getCurrentDeviceTypeObject()) return;
        auto* type = manager->getCurrentDeviceTypeObject(); type->scanForDevices();
        const auto setup = manager->getAudioDeviceSetup();
        inputs = type->getDeviceNames(true); mainOutputs = type->getDeviceNames(false);
        inputDevices.clear(juce::dontSendNotification); mainDevices.clear(juce::dontSendNotification);
        mainDevices.addItem(juce::String::fromUTF8("Sem saída principal (só transmitir)"), 1);
        inputDevices.addItem("Sem entrada (pads e fontes selecionadas)", 1);
        for (int i = 0; i < inputs.size(); ++i) inputDevices.addItem(inputs[i], i+2);
        inputDevices.setSelectedId(inputs.indexOf(setup.inputDeviceName)+2, juce::dontSendNotification);
        if (setup.inputDeviceName.isEmpty()) inputDevices.setSelectedId(1, juce::dontSendNotification);
        for (int i = mainOutputs.size(); --i >= 0;)
            if (SonoCraftAudioRouting::isVirtualCable(mainOutputs[i])) mainOutputs.remove(i);
        for (int i = 0; i < mainOutputs.size(); ++i) mainDevices.addItem(mainOutputs[i], i+2);
        mainDevices.setSelectedId(mainOutputs.contains(setup.outputDeviceName) ? mainOutputs.indexOf(setup.outputDeviceName)+2 : 0, juce::dontSendNotification);
        if (setup.outputDeviceName.isEmpty()) mainDevices.setSelectedId(1, juce::dontSendNotification);
    }
    void applySetup() {
        auto* manager = managerProvider ? managerProvider() : nullptr;
        if (!manager) { applyError = "Abra este painel no aplicativo SonoCraft."; updateStatus(); return; }
        const int inputIndex = inputDevices.getSelectedId()-2, outputIndex = mainDevices.getSelectedId()-2;
        if (mainDevices.getSelectedId() == 0) { applyError = juce::String::fromUTF8("Escolha a saída principal antes de aplicar."); updateStatus(); return; }
        const auto input = juce::isPositiveAndBelow(inputIndex, inputs.size()) ? inputs[inputIndex] : juce::String();
        const auto output = juce::isPositiveAndBelow(outputIndex, mainOutputs.size()) ? mainOutputs[outputIndex] : juce::String();
        if (input.isEmpty() && output.isEmpty()) applyError = juce::String::fromUTF8("Escolha uma entrada ou saída para manter o áudio dos pads em funcionamento.");
        else if (SonoCraftAudioRouting::isVirtualCable(output)) applyError = juce::String::fromUTF8("Escolha fones, alto-falantes ou Sem saída. O SonoCraft não envia áudio para microfones virtuais.");
        else applyError.clear();
        if (applyError.isNotEmpty()) { updateStatus(); return; }
        if (output.equalsIgnoreCase(processor.getPadMonitor().getDeviceName())) processor.getPadMonitor().stop();
        auto setup = manager->getAudioDeviceSetup();
        setup.inputDeviceName = input; setup.outputDeviceName = output;
        setup.useDefaultInputChannels = input.isNotEmpty(); if (input.isEmpty()) setup.inputChannels.clear();
        setup.useDefaultOutputChannels = output.isNotEmpty(); if (output.isEmpty()) setup.outputChannels.clear();
        applyError = manager->setAudioDeviceSetup(setup, true);
        if (applyError.isNotEmpty()) { updateStatus(); return; }
        processor.setStandaloneAudioDevices(input, output);
        setParameter(SonobusAudioProcessor::paramMainRecvMute, 0);
        setParameter(SonobusAudioProcessor::paramDry, 0);
        setParameter(SonobusAudioProcessor::paramSendSoundboardAudio, 1);
        setParameter(SonobusAudioProcessor::paramMainSendMute, 0);
        updateStatus(); showHint();
    }
    void updateStatus() {
        if (!full) return;
        const bool blocked = processor.isStandaloneOutputBlocked();
        const auto text = applyError.isNotEmpty() ? applyError : blocked
            ? juce::String::fromUTF8("A saída virtual salva foi bloqueada. Escolha fones, alto-falantes ou Sem saída e clique em Aplicar. A transmissão entre usuários do SonoCraft continua disponível.")
            : juce::String::fromUTF8("A entrada, os pads e as fontes habilitadas são enviados aos conectados no SonoCraft. A saída local reproduz os sons para você.");
        routingStatus.setText(text, juce::dontSendNotification);
        routingStatus.setColour(juce::Label::textColourId, applyError.isNotEmpty() || blocked ? juce::Colour(0xffa02a23) : SonoCraftTheme::secondary());
    }
    void showHint() {
        hint.setText(juce::String::fromUTF8(processor.getPadMonitor().isActive()
            ? "Escuta ligada: áudio recebido + pads, sem sua própria voz. O envio aos conectados continua independente."
            : "A escuta não altera o envio aos conectados. Escolha uma saída física diferente da saída principal para evitar reprodução duplicada."), juce::dontSendNotification);
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
    void timerCallback() override {
        if (!isShowing()) return;
        receive.setToggleState(processor.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMainRecvMute)->load() == 0, juce::dontSendNotification);
        auto& monitor = processor.getPadMonitor(); enabled.setToggleState(monitor.isActive(), juce::dontSendNotification);
        if (!volume.isMouseButtonDown()) volume.setValue(monitor.getVolume(), juce::dontSendNotification);
        if (monitor.isActive()) for (int i = 0; i < outputs.size(); ++i)
            if (outputs[i].name == monitor.getDeviceName() && outputs[i].type == monitor.getDeviceType()) devices.setSelectedId(i+1, juce::dontSendNotification);
        updateStatus();
    }
};
#pragma pop_macro("Component")
