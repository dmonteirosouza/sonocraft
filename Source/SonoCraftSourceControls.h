// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#pragma push_macro("Component")
#undef Component
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"

class SonoCraftSourceControls final : public juce::Component, private juce::Timer {
public:
    explicit SonoCraftSourceControls(SonobusAudioProcessor& value) : processor(value) {
        for (auto* item : std::initializer_list<juce::Component*> { &intro,&mode,&applications,&refresh,&begin,&end,&network,&meeting,&received,&status,&explanation,&inputNotice }) addAndMakeVisible(item);
        intro.setText(juce::String::fromUTF8("Escolha o som que deseja compartilhar. No sistema, mantenha o fone/alto-falante como saída dos aplicativos. Use o cabo virtual somente na saída do SonoCraft para a reunião."), juce::dontSendNotification);
        mode.addItem("Uma aba do navegador (outras abas privadas)",1);
        mode.addItem("Um aplicativo inteiro (todas as janelas dele)",2);
        mode.setSelectedId(1,juce::dontSendNotification);
        mode.onChange = [this] { processor.getApplicationAudio().stop(); actionError.clear(); inputNotice.setText({},juce::dontSendNotification); updateMode(); scan(); };
        refresh.setButtonText("Atualizar aplicativos"); refresh.onClick = [this] { scan(); };
        end.setButtonText("Parar captura"); end.onClick = [this] { actionError.clear(); processor.getApplicationAudio().stop(); };
        network.setButtonText(juce::String::fromUTF8("Enviar esta fonte ao grupo / conexão"));
        meeting.setButtonText(juce::String::fromUTF8("Enviar esta fonte à reunião (microfone virtual)"));
        received.setButtonText(juce::String::fromUTF8("Enviar abas e aplicativos recebidos à reunião"));
        networkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(),SonobusAudioProcessor::paramSendApplicationAudio,network);
        meetingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(),SonobusAudioProcessor::paramApplicationToMeeting,meeting);
        receivedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(),SonobusAudioProcessor::paramReceivedApplicationsToMeeting,received);
        begin.onClick = [this] {
            actionError.clear();
            if (!network.getToggleState() && !meeting.getToggleState()) {
                showError(juce::String::fromUTF8("Marque primeiro onde enviar esta fonte: grupo / conexão ou reunião.")); return;
            }
            if (mode.getSelectedId()==2 && !juce::isPositiveAndBelow(applications.getSelectedId()-1,apps.size())) {
                showError("Escolha um aplicativo na lista."); return;
            }
            if (network.getToggleState()) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->setValueNotifyingHost(0);
            // Old virtual-input setups copy every application into the input.
            // Silence that aggregate before promising a private tab selection.
            if (SonoCraftAudioRouting::isVirtualCable(processor.getStandaloneInputName())) {
                processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainInMute)->setValueNotifyingHost(1);
                inputNotice.setText(juce::String::fromUTF8("A entrada do cabo virtual foi silenciada para não enviar outras abas junto. Os pads e esta captura continuam funcionando. No sistema, reproduza os aplicativos no fone ou alto-falante físico."),juce::dontSendNotification);
            }
            if (mode.getSelectedId()==1) {
                const auto url=processor.getApplicationAudio().startBrowserTab();
                if (url.isNotEmpty() && !juce::URL(url).launchInDefaultBrowser()) showError("Abra o Chrome ou Edge e tente novamente.");
            } else {
                const int index=applications.getSelectedId()-1;
                if (!juce::isPositiveAndBelow(index,apps.size())) { showError("Escolha um aplicativo na lista."); return; }
                processor.getApplicationAudio().start(apps[index]);
            }
        };
        for (auto* label : {&intro,&status,&explanation,&inputNotice}) {
            label->setFont(14.0f); label->setMinimumHorizontalScale(1); label->setJustificationType(juce::Justification::topLeft);
        }
        inputNotice.setFont(12.5f); status.setColour(juce::Label::textColourId,SonoCraftTheme::accent());
        updateMode(); scan(); startTimer(250);
    }
    ~SonoCraftSourceControls() override { stopTimer(); }
    void resized() override {
        auto box=getLocalBounds().reduced(14);
        intro.setBounds(box.removeFromTop(92)); mode.setBounds(box.removeFromTop(34)); box.removeFromTop(12);
        if (mode.getSelectedId()==2) { applications.setBounds(box.removeFromTop(34)); refresh.setBounds(box.removeFromTop(30)); box.removeFromTop(8); }
        auto row=box.removeFromTop(36); end.setBounds(row.removeFromRight(124)); row.removeFromRight(8); begin.setBounds(row);
        box.removeFromTop(12); explanation.setBounds(box.removeFromTop(108));
        network.setBounds(box.removeFromTop(32)); meeting.setBounds(box.removeFromTop(32)); received.setBounds(box.removeFromTop(46));
        box.removeFromTop(12); status.setBounds(box.removeFromTop(96)); inputNotice.setBounds(box.removeFromTop(100));
    }
private:
    SonobusAudioProcessor& processor;
    juce::Label intro,status,explanation,inputNotice;
    juce::ComboBox mode,applications;
    juce::TextButton refresh,begin,end;
    juce::ToggleButton network,meeting,received;
    juce::Array<SonoCraftApplicationAudio::Application> apps;
    juce::String actionError;
    void showError(const juce::String& text) { actionError=text; status.setText(text,juce::dontSendNotification); }
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> networkAttachment,meetingAttachment,receivedAttachment;
    void updateMode() {
        const bool tab=mode.getSelectedId()==1; applications.setVisible(!tab); refresh.setVisible(!tab); resized();
        begin.setButtonText(tab ? "Escolher uma aba no navegador" : "Capturar aplicativo selecionado");
        explanation.setText(juce::String::fromUTF8(tab
            ? "Uma página auxiliar abrirá no Chrome ou Edge. Escolha a aba e marque Compartilhar áudio. Mantenha a página auxiliar aberta. Você ouve o som original normalmente; nenhuma outra aba é capturada."
            : "Captura todas as janelas do aplicativo escolhido. Para manter uma aba privada, use Uma aba do navegador. O sistema pode solicitar permissão de gravação de áudio/tela. Não selecione o aplicativo da própria reunião, para evitar retorno."),juce::dontSendNotification);
    }
    void scan() {
        const auto previous=applications.getText(); apps=SonoCraftApplicationAudio::applications(); applications.clear(juce::dontSendNotification);
        for (int i=0;i<apps.size();++i) { applications.addItem(apps[i].name,i+1); if (apps[i].name==previous) applications.setSelectedId(i+1,juce::dontSendNotification); }
        applications.setTextWhenNothingSelected("Escolha um aplicativo aberto");
    }
    void timerCallback() override {
        if (!isShowing()) return;
        auto& audio=processor.getApplicationAudio();
        auto text=audio.status();
        if (actionError.isNotEmpty()) { status.setText(actionError,juce::dontSendNotification); return; }
        if (audio.isActive()) text=audio.sourceName()+" · "+(audio.level()>0.001f ? juce::String::fromUTF8("recebendo áudio") : juce::String::fromUTF8("aguardando som"))+"\n"+text;
        if ((meeting.getToggleState()||received.getToggleState()) && processor.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMeetingMode)->load()<0.5f)
            text+=juce::String::fromUTF8("\nPara enviar à reunião, aplique Participo da reunião aqui na aba Reunião e fones.");
        status.setText(text,juce::dontSendNotification);
    }
};
#pragma pop_macro("Component")
