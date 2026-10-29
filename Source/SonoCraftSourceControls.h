// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#pragma push_macro("Component")
#undef Component
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"

class SonoCraftSourceControls final : public juce::Component, private juce::Timer {
public:
    explicit SonoCraftSourceControls(SonobusAudioProcessor& value) : processor(value) {
        for (auto* item : std::initializer_list<juce::Component*> { &intro,&mode,&applications,&refresh,&begin,&end,&network,&status,&explanation,&inputNotice,&sourceHeading,&destinationsHeading }) addAndMakeVisible(item);
        intro.setText(juce::String::fromUTF8("Escolha uma aba ou aplicativo para enviar aos conectados no SonoCraft. A entrada e os pads continuam com seus próprios controles de envio."), juce::dontSendNotification);
        mode.addItem("Uma aba do navegador",1);
        mode.addItem("Um aplicativo inteiro (todas as janelas dele)",2);
        mode.setSelectedId(1,juce::dontSendNotification);
        mode.onChange = [this] { processor.getApplicationAudio().stop(); actionError.clear(); inputNotice.setText({},juce::dontSendNotification); updateMode(); scan(); };
        refresh.setButtonText("Atualizar aplicativos"); refresh.onClick = [this] { scan(); };
        end.setButtonText("Parar captura"); end.onClick = [this] { actionError.clear(); processor.getApplicationAudio().stop(); };
        network.setButtonText(juce::String::fromUTF8("Enviar esta fonte aos conectados"));
        sourceHeading.setText(juce::String::fromUTF8("1. Fonte local (uma seleção por vez)"),juce::dontSendNotification);
        destinationsHeading.setText(juce::String::fromUTF8("2. Envio desta fonte"),juce::dontSendNotification);
        network.setTooltip(juce::String::fromUTF8("Envia esta seleção aos participantes da conexão ou grupo do SonoCraft, além da entrada e dos pads habilitados."));
        for (auto* label : {&sourceHeading,&destinationsHeading}) {
            label->setFont(juce::Font(14.0f,juce::Font::bold));
            label->setMinimumHorizontalScale(1);
        }
        networkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(),SonobusAudioProcessor::paramSendApplicationAudio,network);
        begin.onClick = [this] {
            actionError.clear();
            if (!network.getToggleState()) {
                showError(juce::String::fromUTF8("Marque Enviar esta fonte aos conectados antes de iniciar.")); return;
            }
            if (mode.getSelectedId()==2 && !juce::isPositiveAndBelow(applications.getSelectedId()-1,apps.size())) {
                showError("Escolha um aplicativo na lista."); return;
            }
            if (network.getToggleState()) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->setValueNotifyingHost(0);
            if (SonoCraftAudioRouting::isVirtualCable(processor.getStandaloneInputName())) {
                const bool inputMuted = processor.getValueTreeState().getRawParameterValue(SonobusAudioProcessor::paramMainInMute)->load() > 0.5f;
                inputNotice.setText(juce::String::fromUTF8(inputMuted
                    ? "Sua entrada continua silenciada. A fonte selecionada e os pads são independentes desse controle. Para voltar a enviar o áudio completo da entrada, desative Silenciar minha entrada."
                    : "A entrada virtual continua enviando o áudio completo. Esta fonte segue separadamente para os conectados no SonoCraft."),juce::dontSendNotification);
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
        network.setBounds(box.removeFromTop(32));
        box.removeFromTop(12); status.setBounds(box.removeFromTop(96)); inputNotice.setBounds(box.removeFromTop(100));
    }
private:
    SonobusAudioProcessor& processor;
    juce::Label intro,status,explanation,inputNotice,sourceHeading,destinationsHeading;
    juce::ComboBox mode,applications;
    juce::TextButton refresh,begin,end;
    juce::ToggleButton network;
    juce::Array<SonoCraftApplicationAudio::Application> apps;
    juce::String actionError;
    void showError(const juce::String& text) { actionError=text; status.setText(text,juce::dontSendNotification); }
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> networkAttachment;
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
        status.setText(text,juce::dontSendNotification);
    }
};
#pragma pop_macro("Component")
