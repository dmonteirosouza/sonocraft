// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#pragma push_macro("Component")
#undef Component
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"

class SonoCraftSourceControls final : public juce::Component, private juce::Timer {
public:
    explicit SonoCraftSourceControls(SonobusAudioProcessor& value) : processor(value) {
        for (auto* item : std::initializer_list<juce::Component*> { &intro,&mode,&applications,&refresh,&begin,&end,&network,&meeting,&received,&status,&explanation,&inputNotice,&sourceHeading,&destinationsHeading,&receivedHeading }) addAndMakeVisible(item);
        intro.setText(juce::String::fromUTF8("O áudio completo da entrada continua chegando aos conectados. Escolha abaixo uma fonte adicional para usar na saída para aplicativos, sem mudar o que você escuta."), juce::dontSendNotification);
        mode.addItem("Uma aba do navegador",1);
        mode.addItem("Um aplicativo inteiro (todas as janelas dele)",2);
        mode.setSelectedId(1,juce::dontSendNotification);
        mode.onChange = [this] { processor.getApplicationAudio().stop(); actionError.clear(); inputNotice.setText({},juce::dontSendNotification); updateMode(); scan(); };
        refresh.setButtonText("Atualizar aplicativos"); refresh.onClick = [this] { scan(); };
        end.setButtonText("Parar captura"); end.onClick = [this] { actionError.clear(); processor.getApplicationAudio().stop(); };
        network.setButtonText(juce::String::fromUTF8("Enviar esta fonte aos conectados"));
        meeting.setButtonText(juce::String::fromUTF8("Usar esta fonte na saída para aplicativos"));
        received.setButtonText(juce::String::fromUTF8("Usar fontes recebidas na saída para aplicativos"));
        sourceHeading.setText(juce::String::fromUTF8("1. Fonte local (uma seleção por vez)"),juce::dontSendNotification);
        destinationsHeading.setText(juce::String::fromUTF8("2. Destinos desta fonte"),juce::dontSendNotification);
        receivedHeading.setText(juce::String::fromUTF8("3. Fontes selecionadas pelos conectados"),juce::dontSendNotification);
        network.setTooltip(juce::String::fromUTF8("Envia esta seleção em faixa separada, além da entrada completa e dos pads. Cada destino decide se a inclui na saída para aplicativos."));
        meeting.setTooltip(juce::String::fromUTF8("Inclui a seleção local no cabo virtual de saída. Sua reprodução original continua no dispositivo de escuta do sistema."));
        received.setTooltip(juce::String::fromUTF8("Inclui as seleções enviadas pelos conectados no cabo virtual de saída. O restante do áudio recebido continua apenas na escuta."));
        for (auto* label : {&sourceHeading,&destinationsHeading,&receivedHeading}) {
            label->setFont(juce::Font(14.0f,juce::Font::bold));
            label->setMinimumHorizontalScale(1);
        }
        networkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(),SonobusAudioProcessor::paramSendApplicationAudio,network);
        meetingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(),SonobusAudioProcessor::paramApplicationToMeeting,meeting);
        receivedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(),SonobusAudioProcessor::paramReceivedApplicationsToMeeting,received);
        begin.onClick = [this] {
            actionError.clear();
            if (!network.getToggleState() && !meeting.getToggleState()) {
                showError(juce::String::fromUTF8("Marque primeiro um destino: conectados ou saída para aplicativos.")); return;
            }
            if (mode.getSelectedId()==2 && !juce::isPositiveAndBelow(applications.getSelectedId()-1,apps.size())) {
                showError("Escolha um aplicativo na lista."); return;
            }
            if (network.getToggleState()) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->setValueNotifyingHost(0);
            if (SonoCraftAudioRouting::isVirtualCable(processor.getStandaloneInputName())) {
                const bool inputMuted = processor.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMainInMute)->load() > 0.5f;
                inputNotice.setText(juce::String::fromUTF8(inputMuted
                    ? "Sua entrada continua silenciada. A fonte selecionada e os pads são independentes desse controle. Para voltar a enviar o áudio completo da entrada, desative Silenciar minha entrada."
                    : "A entrada virtual continua enviando o áudio completo. Esta fonte segue separadamente e só entra na saída para aplicativos quando autorizada no destino."),juce::dontSendNotification);
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
        intro.setBounds(box.removeFromTop(92)); sourceHeading.setBounds(box.removeFromTop(30));
        mode.setBounds(box.removeFromTop(34)); box.removeFromTop(12);
        if (mode.getSelectedId()==2) { applications.setBounds(box.removeFromTop(34)); refresh.setBounds(box.removeFromTop(30)); box.removeFromTop(8); }
        auto row=box.removeFromTop(36); end.setBounds(row.removeFromRight(124)); row.removeFromRight(8); begin.setBounds(row);
        box.removeFromTop(12); explanation.setBounds(box.removeFromTop(108));
        destinationsHeading.setBounds(box.removeFromTop(30));
        network.setBounds(box.removeFromTop(32)); meeting.setBounds(box.removeFromTop(32));
        box.removeFromTop(12); receivedHeading.setBounds(box.removeFromTop(30)); received.setBounds(box.removeFromTop(38));
        box.removeFromTop(12); status.setBounds(box.removeFromTop(96)); inputNotice.setBounds(box.removeFromTop(100));
    }
private:
    SonobusAudioProcessor& processor;
    juce::Label intro,status,explanation,inputNotice,sourceHeading,destinationsHeading,receivedHeading;
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
            ? "Abra a página auxiliar no Chrome ou Edge, escolha uma aba com áudio e mantenha a página aberta. Só essa aba entra na seleção; as outras continuam na escuta completa se estiverem chegando pela entrada."
            : "Inclui todas as janelas e abas do aplicativo. Para compartilhar somente uma aba, use Uma aba do navegador. O sistema pode pedir permissão de áudio/tela. Evite capturar o aplicativo que recebe esta mesma saída, para não criar retorno."),juce::dontSendNotification);
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
            text+=juce::String::fromUTF8("\nPara usar estas fontes na saída local, ative Saída para aplicativos em Escuta e saídas.");
        status.setText(text,juce::dontSendNotification);
    }
};
#pragma pop_macro("Component")
