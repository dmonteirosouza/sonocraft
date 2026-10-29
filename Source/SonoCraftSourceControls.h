// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"
#include "SonoCraftFaderLookAndFeel.h"

// A short add-source dialog, launched from the mixer, not a settings tab.
class SonoCraftSourceControls final : public juce::Component {
public:
    explicit SonoCraftSourceControls(SonobusAudioProcessor& p) : processor(p) {
        for (auto* c : std::initializer_list<juce::Component*>{&close,&title,&help,&kind,&apps,&refresh,&add,&status}) addAndMakeVisible(c);
        close.setButtonText("Fechar"); close.onClick=[this]{if(onClose)onClose();};
        title.setText("Adicionar fonte",juce::dontSendNotification); title.setFont(juce::Font(20.0f,juce::Font::bold));
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
        scan();update();setSize(450,370);
    }
    std::function<void()> onClose;
    void paint(juce::Graphics& g) override {
        g.setColour(SonoCraftTheme::panel());g.fillRoundedRectangle(getLocalBounds().toFloat(),10);
        g.setColour(SonoCraftTheme::accent());g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),10,2);
    }
    void resized() override {
        auto r=getLocalBounds().reduced(18);
        auto header=r.removeFromTop(32);close.setBounds(header.removeFromRight(68));title.setBounds(header); help.setBounds(r.removeFromTop(84));
        kind.setBounds(r.removeFromTop(32));r.removeFromTop(8);
        apps.setBounds(r.removeFromTop(32)); refresh.setBounds(r.removeFromTop(30));r.removeFromTop(8);
        add.setBounds(r.removeFromTop(36));r.removeFromTop(8);status.setBounds(r);
    }
private:
    SonobusAudioProcessor& processor;
    juce::Label title,help,status;
    juce::ComboBox kind,apps;
    juce::TextButton refresh,add,close;
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

class SonoCraftSourceStrip final : public juce::Component {
public:
    SonoCraftSourceStrip(SonobusAudioProcessor& p,const juce::String& sourceId) : processor(p),id(sourceId) {
        for(auto* c:std::initializer_list<juce::Component*>{&name,&type,&status,&level,&send,&mute,&capture,&remove})addAndMakeVisible(c);
        name.setEditable(false,true,false);name.setTooltip("Duplo clique para renomear este canal");
        name.onTextChange=[this]{processor.getSourceMixer().rename(id,name.getText());};
        name.setFont(juce::Font(15.0f,juce::Font::bold));
        type.setFont(11.0f);type.setColour(juce::Label::textColourId,SonoCraftTheme::secondary());
        status.setFont(12.0f);status.setMinimumHorizontalScale(1);status.setJustificationType(juce::Justification::topLeft);
        level.setLookAndFeel(&faderLookAndFeel);level.setDoubleClickReturnValue(true,100);
        level.setRange(0,200,1);level.setSliderStyle(juce::Slider::LinearVertical);
        level.setTextBoxStyle(juce::Slider::TextBoxBelow,false,76,24);level.setTextValueSuffix("%");level.setTitle("Volume de envio da fonte");
        level.onValueChange=[this]{processor.getSourceMixer().setGain(id,float(level.getValue())/100.0f);};
        send.setButtonText("Enviar");send.setTooltip("Enviar este canal aos conectados no SonoCraft");
        send.onClick=[this]{
            actionError.clear();
            if(send.getToggleState() && !requested && !beginCapture()) {send.setToggleState(false,juce::dontSendNotification);return;}
            processor.getSourceMixer().setSend(id,send.getToggleState());
        };
        mute.setButtonText("Silenciar");mute.onClick=[this]{processor.getSourceMixer().setMute(id,mute.getToggleState());};
        remove.setButtonText("Remover");remove.onClick=[this]{processor.getSourceMixer().remove(id);};
        capture.onClick=[this]{
            actionError.clear();
            if(permissionRequired){
                if(SonoCraftApplicationAudio::requestCapturePermission())beginCapture();
                else actionError=juce::String::fromUTF8("Autorize esta instalação nos Ajustes do sistema e reabra o SonoCraft se solicitado.");
            } else if(requested) processor.getSourceMixer().stop(id);
            else beginCapture();
        };
    }
    ~SonoCraftSourceStrip() override {level.setLookAndFeel(nullptr);}
    const juce::String& sourceId() const {return id;}
    void sync(const SonoCraftSourceMixer::View& s) {
        if(!name.isBeingEdited())name.setText(s.name,juce::dontSendNotification);
        type.setText(s.tab?juce::String("ABA DO NAVEGADOR"):juce::String::fromUTF8(s.producing?"APLICATIVO · COM ÁUDIO":"APLICATIVO"),juce::dontSendNotification);
        requested=s.requested;permissionRequired=s.permissionRequired;capture.setButtonText(permissionRequired?"Autorizar captura":requested?"Parar captura":"Iniciar captura");
        capture.setEnabled(!s.blockedByTabs);send.setEnabled(!s.blockedByTabs);
        status.setText(s.blockedByTabs ? juce::String::fromUTF8("Abas individuais em uso. Remova as abas para enviar o navegador inteiro.") : actionError.isNotEmpty() ? actionError : s.automatic && !s.requested ? juce::String::fromUTF8(s.producing?"Áudio detectado. Ative Enviar para compartilhar.":"Sem áudio agora. Seus ajustes foram mantidos.") : s.active ? (s.sending ? (s.muted ? juce::String("Envio silenciado") : juce::String("Enviando aos conectados")) : juce::String::fromUTF8("Capturando · envio desligado")) : s.status,juce::dontSendNotification);
        status.setTooltip(s.status);
        if(!level.isMouseButtonDown())level.setValue(s.gain*100,juce::dontSendNotification);
        send.setToggleState(s.sending,juce::dontSendNotification);mute.setToggleState(s.muted,juce::dontSendNotification);
        peak=s.level;repaint();
    }
    void resized() override {
        auto r=getLocalBounds().reduced(10);name.setBounds(r.removeFromTop(30));type.setBounds(r.removeFromTop(20));status.setBounds(r.removeFromTop(60));
        remove.setBounds(r.removeFromBottom(28));r.removeFromBottom(5);capture.setBounds(r.removeFromBottom(28));
        auto toggles=r.removeFromBottom(30);send.setBounds(toggles.removeFromLeft(toggles.getWidth()/2));mute.setBounds(toggles);
        meter=r.removeFromRight(12).reduced(2,12).toFloat();level.setBounds(r);
    }
    void paint(juce::Graphics& g) override {
        g.setColour(SonoCraftTheme::panel());g.fillRoundedRectangle(getLocalBounds().toFloat(),8);
        g.setColour(SonoCraftTheme::border());g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),8,1);
        g.fillRoundedRectangle(meter,3);g.setColour(peak>1?SonoCraftTheme::danger():SonoCraftTheme::accent());
        auto fill=meter;fill.removeFromTop(fill.getHeight()*(1-juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(peak,-60.0f)+60)/60)));
        g.fillRoundedRectangle(fill,3);
    }
private:
    SonobusAudioProcessor& processor;
    juce::String id,actionError;
    juce::Label name,type,status;
    SonoCraftFaderLookAndFeel faderLookAndFeel;
    juce::Slider level;
    juce::ToggleButton send,mute;
    juce::TextButton capture,remove;
    juce::Rectangle<float> meter;
    float peak=0;bool requested=false,permissionRequired=false;
    bool beginCapture(){
        const auto result=processor.getSourceMixer().restart(id);actionError=result.error;
        if(result.url.isNotEmpty())juce::URL(result.url).launchInDefaultBrowser();
        return result.error.isEmpty();
    }
};
