#pragma once
#pragma push_macro("Component")
#undef Component
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"

// Headphone playback is an additional destination. It never changes the
// processor's meeting mix or takes ownership of the primary device.
class SonoCraftListeningControls final : public juce::Component, private juce::Timer {
public:
    using DeviceManagerProvider = std::function<juce::AudioDeviceManager*()>;
    SonoCraftListeningControls(SonobusAudioProcessor& p, bool fullPage = false, DeviceManagerProvider provider = {})
        : processor(p), full(fullPage), managerProvider(std::move(provider)) {
        for (auto* item : std::initializer_list<juce::Component*> { &enabled, &devices, &refresh, &volume, &hint,
                &instructions, &role, &roleLabel, &inputDevices, &inputLabel, &mainDevices, &mainLabel,
                &apply, &routingStatus, &configure, &receive, &micMute, &includeReceived, &headphonesLabel })
            addAndMakeVisible(item);
        instructions.setText(juce::String::fromUTF8("Escolha a função deste computador e clique em Aplicar. O áudio para a reunião e o áudio dos seus fones têm destinos separados. Ouvir nos fones não deve interromper o envio à reunião."), juce::dontSendNotification);
        roleLabel.setText(juce::String::fromUTF8("1. O que este computador faz?"), juce::dontSendNotification);
        role.addItem(juce::String::fromUTF8("Participo da reunião aqui (voz + pads)"), 1);
        role.addItem("Envio sons para outro computador", 2);
        role.setSelectedId(processor.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMeetingMode)->load() > 0 ? 1 : 2, juce::dontSendNotification);
        role.onChange = [this] { updateRoleLabels(); };
        apply.setButtonText(juce::String::fromUTF8("Aplicar configuração neste computador"));
        apply.onClick = [this] { applySetup(); };
        configure.setButtonText(juce::String::fromUTF8("Dispositivos e opções avançadas…"));
        configure.onClick = [this] { if (onChooseAudio) onChooseAudio(); };
        receive.setButtonText(juce::String::fromUTF8("Receber sons dos outros computadores"));
        receive.onClick = [this] { setParameter(SonobusAudioProcessor::paramMainRecvMute, receive.getToggleState() ? 0 : 1); };
        micMute.setButtonText("Silenciar minha entrada (mantendo os pads)");
        micMuteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(), SonobusAudioProcessor::paramMainInMute, micMute);
        includeReceived.setButtonText(juce::String::fromUTF8("Incluir também músicas, vídeos e voz recebidos na reunião"));
        includeReceived.setTooltip(juce::String::fromUTF8("Desmarcado: sua voz e os pads identificados. Marcado: inclui todos os sons recebidos dos outros computadores. Os fones continuam ouvindo tudo."));
        includeReceivedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(), SonobusAudioProcessor::paramMeetingIncludeReceived, includeReceived);
        headphonesLabel.setText(juce::String::fromUTF8("4. Onde eu ouço (sem retorno da minha voz)"), juce::dontSendNotification);
        enabled.setButtonText("Ouvir sons recebidos e pads nos meus fones");
        devices.setTextWhenNothingSelected("Escolha o headset ou alto-falante");
        devices.setTitle(juce::String::fromUTF8("Fones físicos, sem minha voz"));
        refresh.setButtonText("Atualizar"); refresh.onClick = [this] { scan(); scanMainDevices(); };
        volume.setRange(0, 1, 0.01); volume.setSliderStyle(juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 24);
        volume.setTitle("Volume somente nos meus fones");
        volume.setValue(processor.getPadMonitor().getVolume(), juce::dontSendNotification);
        volume.onValueChange = [this] { processor.getPadMonitor().setVolume(float(volume.getValue())); };
        for (auto* label : { &instructions, &roleLabel, &inputLabel, &mainLabel, &headphonesLabel, &routingStatus, &hint }) {
            label->setMinimumHorizontalScale(1); label->setFont(13.0f);
            label->setJustificationType(juce::Justification::topLeft);
        }
        hint.setFont(12.0f); hint.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        enabled.onClick = [this] {
            if (!enabled.getToggleState()) { processor.getPadMonitor().stop(); showHint(); return; }
            const int index = devices.getSelectedId()-1;
            if (!juce::isPositiveAndBelow(index, outputs.size())) { enabled.setToggleState(false, juce::dontSendNotification); hint.setText("Escolha primeiro seus fones na lista.", juce::dontSendNotification); return; }
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
        for (auto* item : std::initializer_list<juce::Component*> { &instructions, &role, &roleLabel, &inputDevices,
                &inputLabel, &mainDevices, &mainLabel, &apply, &routingStatus, &configure, &receive, &micMute,
                &includeReceived, &headphonesLabel }) item->setVisible(full);
        if (full) {
            instructions.setBounds(box.removeFromTop(76));
            roleLabel.setBounds(box.removeFromTop(24)); role.setBounds(box.removeFromTop(32)); box.removeFromTop(12);
            inputLabel.setBounds(box.removeFromTop(24)); inputDevices.setBounds(box.removeFromTop(32)); box.removeFromTop(12);
            mainLabel.setBounds(box.removeFromTop(24)); mainDevices.setBounds(box.removeFromTop(32)); box.removeFromTop(12);
            apply.setBounds(box.removeFromTop(36)); box.removeFromTop(8);
            routingStatus.setBounds(box.removeFromTop(94));
            configure.setBounds(box.removeFromTop(32)); box.removeFromTop(12);
            receive.setBounds(box.removeFromTop(28)); micMute.setBounds(box.removeFromTop(28));
            includeReceived.setBounds(box.removeFromTop(38)); box.removeFromTop(12);
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
    juce::ToggleButton enabled, receive, micMute, includeReceived;
    juce::ComboBox role, inputDevices, mainDevices, devices;
    juce::TextButton apply, refresh, configure;
    juce::Slider volume;
    juce::Label instructions, roleLabel, inputLabel, mainLabel, routingStatus, headphonesLabel, hint;
    juce::Array<SonoCraftPadMonitor::Device> outputs;
    juce::StringArray inputs, mainOutputs;
    juce::String applyError;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> micMuteAttachment, includeReceivedAttachment;
    void setParameter(const juce::String& id, float value) {
        auto* parameter = processor.getValueTreeState().getParameter(id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
    void updateRoleLabels() {
        const bool meeting = role.getSelectedId() == 1;
        inputLabel.setText(juce::String::fromUTF8(meeting ? "2. Minha voz: microfone do headset (não CABLE Output)" : "2. Som que envio: VB-Cable ou um microfone"), juce::dontSendNotification);
        mainLabel.setText(juce::String::fromUTF8(meeting ? "3. Para a reunião: CABLE Input (não o headset)" : "3. Onde ouço aqui: fone ou alto-falante físico"), juce::dontSendNotification);
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
        mainDevices.addItem(juce::String::fromUTF8("Sem saída (apenas enviar)"), 1);
        inputDevices.addItem("Sem entrada (somente pads)", 1);
        for (int i = 0; i < inputs.size(); ++i) inputDevices.addItem(inputs[i], i+2);
        inputDevices.setSelectedId(inputs.indexOf(setup.inputDeviceName)+2, juce::dontSendNotification);
        if (setup.inputDeviceName.isEmpty()) inputDevices.setSelectedId(1, juce::dontSendNotification);
        for (int i = 0; i < mainOutputs.size(); ++i) mainDevices.addItem(mainOutputs[i], i+2);
        mainDevices.setSelectedId(mainOutputs.indexOf(setup.outputDeviceName)+2, juce::dontSendNotification);
        if (setup.outputDeviceName.isEmpty()) mainDevices.setSelectedId(1, juce::dontSendNotification);
    }
    void applySetup() {
        auto* manager = managerProvider ? managerProvider() : nullptr;
        if (!manager) { applyError = "Abra este painel no aplicativo SonoCraft."; updateStatus(); return; }
        const int inputIndex = inputDevices.getSelectedId()-2, outputIndex = mainDevices.getSelectedId()-2;
        if (mainDevices.getSelectedId() == 0) { applyError = juce::String::fromUTF8("Escolha a saída principal antes de aplicar."); updateStatus(); return; }
        const auto input = juce::isPositiveAndBelow(inputIndex, inputs.size()) ? inputs[inputIndex] : juce::String();
        const auto output = juce::isPositiveAndBelow(outputIndex, mainOutputs.size()) ? mainOutputs[outputIndex] : juce::String();
        const bool meeting = role.getSelectedId() == 1;
        if (input.isEmpty() && output.isEmpty()) applyError = juce::String::fromUTF8("Escolha uma entrada ou saída para manter o áudio dos pads em funcionamento.");
        else if (SonoCraftAudioRouting::feedsBack(input, output)) applyError = juce::String::fromUTF8("Entrada e saída estão no mesmo cabo virtual: isso devolve o som para a entrada e causa repetição. Use a entrada do microfone no computador da reunião; no computador que envia, escolha uma saída física.");
        else if (meeting && SonoCraftAudioRouting::isVirtualCable(input)) applyError = juce::String::fromUTF8("CABLE Output é o microfone do Teams. Aqui no SonoCraft, escolha o microfone físico do headset ou Sem entrada.");
        else if (meeting && !SonoCraftAudioRouting::isVirtualCable(output)) applyError = juce::String::fromUTF8("Para a reunião, escolha CABLE Input como saída principal. O headset fica no campo Onde eu ouço abaixo. Se o cabo não aparece, instale o VB-CABLE e atualize a lista.");
        else if (!meeting && SonoCraftAudioRouting::isVirtualCable(output)) applyError = juce::String::fromUTF8("Neste computador que envia sons, escolha um fone ou alto-falante como saída. O cabo virtual pode ficar somente na entrada para capturar músicas e vídeos.");
        else applyError.clear();
        if (applyError.isNotEmpty()) { updateStatus(); return; }
        if (!meeting || output.equalsIgnoreCase(processor.getPadMonitor().getDeviceName())) processor.getPadMonitor().stop();
        auto setup = manager->getAudioDeviceSetup();
        setup.inputDeviceName = input; setup.outputDeviceName = output;
        setup.useDefaultInputChannels = input.isNotEmpty(); if (input.isEmpty()) setup.inputChannels.clear();
        setup.useDefaultOutputChannels = output.isNotEmpty(); if (output.isEmpty()) setup.outputChannels.clear();
        applyError = manager->setAudioDeviceSetup(setup, true);
        if (applyError.isNotEmpty()) { updateStatus(); return; }
        processor.setStandaloneAudioDevices(input, output);
        setParameter(SonobusAudioProcessor::paramMeetingMode, meeting ? 1 : 0);
        setParameter(SonobusAudioProcessor::paramMainRecvMute, 0);
        setParameter(SonobusAudioProcessor::paramDry, 0);
        setParameter(SonobusAudioProcessor::paramSendSoundboardAudio, 1);
        setParameter(SonobusAudioProcessor::paramMainSendMute, 0);
        if (!meeting && SonoCraftAudioRouting::isVirtualCable(input)) setParameter(SonobusAudioProcessor::paramMainInMute, 0);
        updateStatus(); showHint();
    }
    void updateStatus() {
        if (!full) return;
        const bool blocked = processor.isStandaloneOutputBlocked();
        const auto text = applyError.isNotEmpty() ? applyError : blocked
            ? juce::String::fromUTF8("Saída principal pausada para evitar repetição: entrada e saída usam o mesmo cabo virtual. Corrija os destinos acima e clique em Aplicar. O envio pela rede continua ativo.")
            : processor.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMeetingMode)->load() > 0
                ? juce::String::fromUTF8("Reunião: sua voz + pads locais e recebidos. No Teams, microfone = CABLE Output; alto-falante = headset. Ative a escuta abaixo para ouvir os pads também.")
                : juce::String::fromUTF8("Envio: entrada e pads vão aos conectados. Os sons recebidos tocam na saída física escolhida. Sua própria entrada não retorna para você.");
        routingStatus.setText(text, juce::dontSendNotification);
        routingStatus.setColour(juce::Label::textColourId, applyError.isNotEmpty() || blocked ? juce::Colour(0xffa02a23) : SonoCraftTheme::secondary());
    }
    void showHint() {
        hint.setText(juce::String::fromUTF8(processor.getPadMonitor().isActive()
            ? "Escuta ligada: recebidos + pads. O envio para a reunião continua independente, sem retorno da sua voz."
            : "Esta opção só liga seus fones. Ela não liga nem desliga o envio à reunião. Use fones físicos, nunca o cabo virtual."), juce::dontSendNotification);
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
