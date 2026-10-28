#pragma once
#pragma push_macro("Component")
#undef Component
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"

// Shared controls in device settings and the Soundboard; both views drive the
// same independent listening output, and never open a microphone themselves.
class SonoCraftListeningControls final : public juce::Component, private juce::Timer {
public:
    SonoCraftListeningControls(SonobusAudioProcessor& p, bool fullPage = false) : processor(p), full(fullPage) {
        for (auto* component : std::initializer_list<juce::Component*> { &enabled, &devices, &refresh, &volume, &hint, &instructions, &mode, &configure, &summary, &receive, &micMute }) addAndMakeVisible(component);
        enabled.setButtonText("Ouvir recebidos e pads nos fones");
        devices.setTextWhenNothingSelected("Escolha seu headset ou fone");
        devices.setTitle(juce::String::fromUTF8("Saída dos fones, sem minha voz"));
        refresh.setButtonText("Atualizar");
        refresh.onClick = [this] { scan(); };
        volume.setRange(0, 1, 0.01);
        volume.setSliderStyle(juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 24);
        volume.setTitle("Volume nos meus fones");
        volume.setValue(processor.getPadMonitor().getVolume(), juce::dontSendNotification);
        volume.onValueChange = [this] { processor.getPadMonitor().setVolume(float(volume.getValue())); };
        receive.setButtonText(juce::String::fromUTF8("Receber áudio dos conectados"));
        receive.onClick = [this] { processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainRecvMute)->setValueNotifyingHost(receive.getToggleState() ? 0 : 1); };
        micMute.setButtonText("Silenciar meu microfone");
        micMuteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(), SonobusAudioProcessor::paramMainInMute, micMute);
        mode.setButtonText(juce::String::fromUTF8("Ativar mistura para reunião"));
        modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(), SonobusAudioProcessor::paramMeetingMode, mode);
        mode.onClick = [this] {
            if (mode.getToggleState()) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainRecvMute)->setValueNotifyingHost(0);
        };
        configure.setButtonText(juce::String::fromUTF8("Escolher microfone e saída para a reunião"));
        configure.onClick = [this] { if (onChooseAudio) onChooseAudio(); };
        instructions.setText(juce::String::fromUTF8("1. Microfone: escolha o headset. Saída principal: escolha CABLE Input.\n2. Ative a mistura e a recepção dos conectados.\n3. Abaixo, escolha seus fones: recebidos + pads, sem sua voz.\n4. No Teams: microfone CABLE Output e alto-falante headset."), juce::dontSendNotification);
        instructions.setFont(14.0f);
        instructions.setJustificationType(juce::Justification::topLeft);
        instructions.setMinimumHorizontalScale(1);
        summary.setFont(12.5f); summary.setMinimumHorizontalScale(1);
        summary.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        hint.setFont(12.0f); hint.setMinimumHorizontalScale(1);
        hint.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        enabled.onClick = [this] {
            if (!enabled.getToggleState()) { processor.getPadMonitor().stop(); showHint(); return; }
            const int index = devices.getSelectedId()-1;
            if (!juce::isPositiveAndBelow(index, outputs.size())) { enabled.setToggleState(false, juce::dontSendNotification); hint.setText("Escolha primeiro seus fones na lista.", juce::dontSendNotification); return; }
            auto error = processor.getPadMonitor().start(outputs[index]);
            if (error.isNotEmpty()) { enabled.setToggleState(false, juce::dontSendNotification); hint.setText(error, juce::dontSendNotification); return; }
            auto& state = processor.getValueTreeState().state;
            state.setProperty("listeningOutputName", outputs[index].name, nullptr);
            state.setProperty("listeningOutputType", outputs[index].type, nullptr);
            showHint();
        };
        devices.onChange = [this] {
            if (processor.getPadMonitor().isActive()) processor.getPadMonitor().stop();
            enabled.setToggleState(false, juce::dontSendNotification); showHint();
        };
        scan(); startTimer(250);
    }
    ~SonoCraftListeningControls() override { stopTimer(); }
    std::function<void()> onChooseAudio;
    std::function<juce::String()> configurationSummary;
    void resized() override {
        auto box = getLocalBounds().reduced(full ? 14 : 0);
        for (auto* item : std::initializer_list<juce::Component*> { &instructions, &mode, &configure, &summary, &receive, &micMute }) item->setVisible(full);
        if (full) {
            instructions.setBounds(box.removeFromTop(getWidth() >= 440 ? 112 : 140));
            configure.setBounds(box.removeFromTop(36)); box.removeFromTop(8);
            summary.setBounds(box.removeFromTop(44));
            mode.setBounds(box.removeFromTop(30));
            receive.setBounds(box.removeFromTop(30));
            micMute.setBounds(box.removeFromTop(30)); box.removeFromTop(12);
        }
        enabled.setBounds(box.removeFromTop(28));
        auto row = box.removeFromTop(30);
        refresh.setBounds(row.removeFromRight(76)); row.removeFromRight(6); devices.setBounds(row);
        volume.setBounds(box.removeFromTop(32));
        hint.setBounds(box.removeFromTop(46));
    }
private:
    SonobusAudioProcessor& processor;
    const bool full;
    juce::ToggleButton enabled, mode, receive, micMute;
    juce::ComboBox devices;
    juce::TextButton refresh, configure;
    juce::Slider volume;
    juce::Label hint, instructions, summary;
    juce::Array<SonoCraftPadMonitor::Device> outputs;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeAttachment, micMuteAttachment;
    void showHint() { hint.setText(juce::String::fromUTF8(processor.getPadMonitor().isActive()
        ? "Nos fones: áudio recebido e pads locais. Sua voz fica só na saída da reunião."
        : "Escolha os fones físicos, não o cabo virtual. Desative 'Escutar este dispositivo' no Windows."), juce::dontSendNotification); }
    void scan() {
        auto previous = devices.getText();
        outputs = processor.getPadMonitor().devices(); devices.clear(juce::dontSendNotification);
        const auto savedName = processor.getValueTreeState().state.getProperty("listeningOutputName").toString();
        const auto savedType = processor.getValueTreeState().state.getProperty("listeningOutputType").toString();
        for (int i = 0; i < outputs.size(); ++i) {
            auto label = outputs[i].name + " (" + outputs[i].type + ")";
            devices.addItem(label, i+1);
            if (label == previous || (previous.isEmpty() && outputs[i].name == savedName && outputs[i].type == savedType)) devices.setSelectedId(i+1, juce::dontSendNotification);
        }
        showHint();
        if (outputs.isEmpty()) hint.setText(juce::String::fromUTF8("Nenhuma saída encontrada. Conecte seus fones e clique em Atualizar."), juce::dontSendNotification);
    }
    void timerCallback() override {
        if (!isShowing()) return;
        receive.setToggleState(processor.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMainRecvMute)->load() == 0, juce::dontSendNotification);
        auto& monitor = processor.getPadMonitor();
        enabled.setToggleState(monitor.isActive(), juce::dontSendNotification);
        if (!volume.isMouseButtonDown()) volume.setValue(monitor.getVolume(), juce::dontSendNotification);
        if (monitor.isActive()) for (int i = 0; i < outputs.size(); ++i)
            if (outputs[i].name == monitor.getDeviceName() && outputs[i].type == monitor.getDeviceType()) devices.setSelectedId(i+1, juce::dontSendNotification);
        if (configurationSummary) summary.setText(configurationSummary(), juce::dontSendNotification);
    }
};

#pragma pop_macro("Component")
