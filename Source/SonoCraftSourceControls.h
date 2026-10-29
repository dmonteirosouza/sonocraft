// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"
#include "SonoCraftStrip.h"

// A short add-source dialog, launched from the mixer, not a settings tab.
class SonoCraftSourceControls final : public juce::Component {
public:
    explicit SonoCraftSourceControls(SonobusAudioProcessor& p) : processor(p) {
        for (auto* c : std::initializer_list<juce::Component*>{&close,&title,&help,&kind,&apps,&refresh,&add,&status,&mode,&modeHelp}) addAndMakeVisible(c);
        close.setButtonText("Fechar"); close.onClick=[this]{if(onClose)onClose();};
        title.setText(juce::String::fromUTF8("Fontes de áudio"),juce::dontSendNotification); title.setFont(juce::Font(20.0f,juce::Font::bold));
        help.setText(juce::String::fromUTF8("Cada fonte ganha um canal com volume e envio próprios. O envio começa desligado. O volume original do aplicativo não muda."),juce::dontSendNotification);
        kind.addItem("Aplicativo inteiro",1); kind.addItem("Uma aba do navegador",2); kind.setSelectedId(1,juce::dontSendNotification);
        kind.onChange=[this]{update();}; refresh.setButtonText("Atualizar aplicativos"); refresh.onClick=[this]{scan();};
        add.onClick=[this]{
            SonoCraftSourceMixer::Added result;
            if (kind.getSelectedId()==2) result=processor.getSourceMixer().addTab();
            else {
                const int i=apps.getSelectedId()-1;
                if (!juce::isPositiveAndBelow(i,available.size())) { status.setText("Escolha um aplicativo aberto.",juce::dontSendNotification); return; }
                result=processor.getSourceMixer().addApplication(available[i]);
            }
            if (result.error.isNotEmpty()) { status.setText(result.error,juce::dontSendNotification); return; }
            if (result.url.isNotEmpty() && !juce::URL(result.url).launchInDefaultBrowser()) {
                status.setText("Abra o Chrome ou Edge e inicie a captura pelo canal criado.",juce::dontSendNotification); return;
            }
            if (onClose) onClose();
        };
        for (auto* l:{&help,&status}) { l->setMinimumHorizontalScale(1); l->setJustificationType(juce::Justification::topLeft); }
        status.setColour(juce::Label::textColourId,SonoCraftTheme::warning());
        mode.onClick=[this]{
            auto& mixer=processor.getSourceMixer();
            if(mixer.usesIndividualSources())mixer.useCompleteInput();else mixer.useIndividualSources();
            refreshMode();
        };
        modeHelp.setFont(12.0f);modeHelp.setColour(juce::Label::textColourId,SonoCraftTheme::secondary());
        scan();update();refreshMode();setSize(470,470);
    }
    void refreshMode() {
        const bool visible=SonoCraftAudioRouting::isVirtualCable(processor.getStandaloneInputName());
        mode.setVisible(visible);modeHelp.setVisible(visible);
        const bool individual=processor.getSourceMixer().usesIndividualSources();
        modeHelp.setText(juce::String::fromUTF8(individual?"Modo atual: aplicativos e abas selecionados.":"Modo atual: entrada completa do sistema."),juce::dontSendNotification);
        mode.setButtonText(individual?"Usar entrada completa":"Usar fontes individuais");
        mode.setTooltip(juce::String::fromUTF8("A entrada completa e as fontes individuais não são enviadas ao mesmo tempo, para evitar áudio duplicado."));
        resized();
    }
    std::function<void()> onClose;
    void paint(juce::Graphics& g) override {
        g.setColour(SonoCraftTheme::panel());g.fillRoundedRectangle(getLocalBounds().toFloat(),10);
        g.setColour(SonoCraftTheme::accent());g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),10,2);
    }
    void resized() override {
        auto r=getLocalBounds().reduced(18);
        auto header=r.removeFromTop(32);close.setBounds(header.removeFromRight(68));title.setBounds(header); help.setBounds(r.removeFromTop(72));
        if(mode.isVisible()){modeHelp.setBounds(r.removeFromTop(36));mode.setBounds(r.removeFromTop(30));r.removeFromTop(10);}
        kind.setBounds(r.removeFromTop(32));r.removeFromTop(8);
        apps.setBounds(r.removeFromTop(32)); r.removeFromTop(8);
        refresh.setBounds(r.removeFromTop(30));r.removeFromTop(8);
        add.setBounds(r.removeFromTop(36));r.removeFromTop(8);status.setBounds(r);
    }
private:
    SonobusAudioProcessor& processor;
    juce::Label title,help,status,modeHelp;
    juce::ComboBox kind,apps;
    juce::TextButton refresh,add,close,mode;
    juce::Array<SonoCraftApplicationAudio::Application> available;
    void scan() {
        const auto previous=apps.getText();available=SonoCraftApplicationAudio::applications();apps.clear(juce::dontSendNotification);
        for (int i=0;i<available.size();++i) { apps.addItem(available[i].name,i+1);if(available[i].name==previous)apps.setSelectedId(i+1,juce::dontSendNotification); }
        apps.setTextWhenNothingSelected("Escolha um aplicativo aberto");
    }
    void update() {
        const bool tab=kind.getSelectedId()==2; apps.setVisible(!tab); refresh.setVisible(!tab);
        add.setButtonText(tab?"Adicionar aba e escolher no navegador":"Adicionar aplicativo");
        help.setText(juce::String::fromUTF8(tab
            ? "Escolha a aba no Chrome ou Edge e mantenha a página auxiliar aberta. Adicione cada aba só uma vez. O envio do navegador inteiro será desligado para evitar áudio duplicado."
            : "Cada aplicativo ganha um canal com volume e envio próprios. O envio começa desligado. O volume original do aplicativo não muda."),juce::dontSendNotification);
    }
};

class SonoCraftSourceStrip final : public SonoCraftStrip {
public:
    SonoCraftSourceStrip(SonobusAudioProcessor& p,const juce::String& sourceId) : processor(p),id(sourceId) {
        name.setEditable(false,true,false);name.setTooltip("Duplo clique para renomear este canal");
        name.onTextChange=[this]{processor.getSourceMixer().rename(id,name.getText());};
        fader.setTitle("Volume de envio da fonte (dB)");
        fader.onValueChange=[this]{processor.getSourceMixer().setGain(id,gainForDb(fader.getValue()));};
        secondary.setButtonText("Enviar");secondary.setTooltip("Enviar este canal aos conectados no SonoCraft");
        secondary.onClick=[this]{
            actionError.clear();
            if(secondary.getToggleState() && !requested && !beginCapture()) {secondary.setToggleState(false,juce::dontSendNotification);return;}
            processor.getSourceMixer().setSend(id,secondary.getToggleState());
        };
        mute.setTooltip("Silenciar o envio desta fonte, mantendo o volume escolhido");mute.onClick=[this]{processor.getSourceMixer().setMute(id,mute.getToggleState());};
        remove.setButtonText("Remover");remove.onClick=[this]{processor.getSourceMixer().remove(id);};
        details.onClick=[this]{
            actionError.clear();
            if(permissionRequired){
                if(SonoCraftApplicationAudio::requestCapturePermission())beginCapture();
                else actionError=juce::String::fromUTF8("Autorize esta instalação nos Ajustes do sistema e reabra o SonoCraft se solicitado.");
            } else if(requested) processor.getSourceMixer().stop(id);
            else beginCapture();
        };
    }
    const juce::String& sourceId() const {return id;}
    void sync(const SonoCraftSourceMixer::View& s, bool hasRecipients, bool sendPaused) {
        if(!name.isBeingEdited())name.setText(s.name,juce::dontSendNotification);
        subtitle.setText(s.tab ? "ABA DO NAVEGADOR" : "APLICATIVO", juce::dontSendNotification);
        requested=s.requested;permissionRequired=s.permissionRequired;details.setButtonText(permissionRequired?"Autorizar captura":requested?"Parar captura":"Iniciar captura");
        details.setEnabled(!s.blockedByTabs);secondary.setEnabled(!s.blockedByTabs);
        const auto presentation = SonoCraftPresentation::describeSource(s, hasRecipients, sendPaused, actionError);
        setStatus(presentation.state, presentation.detail, presentation.tone);
        setRoutes(presentation.listening, presentation.sending, presentation.detail);
        syncGain(s.gain);
        secondary.setToggleState(s.sending,juce::dontSendNotification);mute.setToggleState(s.muted,juce::dontSendNotification);
        meterLeft=s.leftLevel;meterRight=s.rightLevel;
        setTooltip(juce::String::fromUTF8(s.systemMeter
            ? "Atividade do aplicativo no sistema. Este medidor não confirma o envio; confira o estado do canal."
            : "Nível capturado antes do volume e do silêncio de envio. Barras em movimento não significam que o áudio está sendo enviado."));
        repaint();
    }
private:
    SonobusAudioProcessor& processor;
    juce::String id,actionError;
    bool requested=false,permissionRequired=false;
    bool beginCapture(){
        const auto result=processor.getSourceMixer().restart(id);actionError=result.error;
        if(result.url.isNotEmpty())juce::URL(result.url).launchInDefaultBrowser();
        return result.error.isEmpty();
    }
};
