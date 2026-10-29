// SonoCraft's desktop mixing workspace. All meters are GUI snapshots;
// audio processing remains owned by SonobusAudioProcessor.
#pragma once
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"
#include "SonoCraftSourceControls.h"
#include "SonoCraftStrip.h"

class SonoCraftChannelStrip final : public SonoCraftStrip {
public:
    enum Role { input, peer, master };
    SonoCraftChannelStrip(SonobusAudioProcessor& p, Role r, juce::String connection = {})
        : processor(p), role(r), id(std::move(connection))
    {
        setGainLimit(role == input ? 4.0f : 2.0f);
        fader.onValueChange = [this] {
            const auto gain = gainForDb(fader.getValue());
            if (role == peer) processor.setMixerChannelGain(id, gain);
            else writeParameter(role == input ? SonobusAudioProcessor::paramInGain : SonobusAudioProcessor::paramWet, gain);
        };
        fader.onDragStart = [this] { if (auto* parameter = gainParameter()) parameter->beginChangeGesture(); };
        fader.onDragEnd = [this] { if (auto* parameter = gainParameter()) parameter->endChangeGesture(); };
        // This existing switch affects received participants, not local pads.
        // Keep that scope explicit instead of presenting it as a master mute.
        if (role == master) mute.setButtonText("Silenciar recebidos");
        secondary.setButtonText(role == input ? "Ouvir entrada" : juce::String::fromUTF8("Ouvir só este"));
        mute.setTooltip(role == input ? juce::String::fromUTF8("Silenciar a entrada de áudio, mantendo os pads e fontes selecionadas")
            : juce::String::fromUTF8("Silenciar o áudio recebido, sem interromper a conexão"));
        secondary.setTooltip(role == input ? juce::String::fromUTF8("Ouvir sua entrada na saída local. Não altera o envio aos conectados.")
            : juce::String::fromUTF8("Ouvir somente este participante e os outros marcados como Ouvir só este. Não altera o envio."));
        mute.onClick = [this] {
            if (role == peer) processor.setMixerChannelMute(id, mute.getToggleState());
            else writeParameter(role == input ? SonobusAudioProcessor::paramMainInMute : SonobusAudioProcessor::paramMainRecvMute, mute.getToggleState() ? 1.0f : 0.0f);
        };
        secondary.onClick = [this] {
            if (role == peer) processor.setMixerChannelSolo(id, secondary.getToggleState());
            else if (role == input) writeParameter(SonobusAudioProcessor::paramDry, secondary.getToggleState() ? 1.0f : 0.0f);
        };
        details.setButtonText(role == peer ? "Detalhes" : "Dispositivo");
        details.onClick = [this] { if (onDetails) onDetails(id); };
        remove.setTooltip(juce::String::fromUTF8("Remover somente esta conexão da sua sessão"));
        remove.onClick = [this] { processor.removeRemotePeerByConnectionId(id); };
        remove.setVisible(role == peer);
        secondary.setVisible(role != master);
        fader.setTitle(role == input ? "Volume da entrada (dB)" : role == master ? juce::String::fromUTF8("Volume da saída principal (dB)") : "Volume do participante (dB)");
    }
    void setState(const SonobusAudioProcessor::MixerChannelState& state) {
        name.setText(state.name, juce::dontSendNotification);
        subtitle.setText("PARTICIPANTE", juce::dontSendNotification);
        name.setTooltip(state.name + juce::String::fromUTF8(" · ") + state.address);
        setStatus(state.connected ? (state.muted ? juce::String::fromUTF8("Recepção silenciada") : juce::String::fromUTF8("Conectado · ") + state.address) : juce::String::fromUTF8("Aguardando conexão"));
        meterLeft = state.left; meterRight = state.right;
        mute.setToggleState(state.muted, juce::dontSendNotification);
        secondary.setToggleState(state.soloed, juce::dontSendNotification);
        syncGain(state.gain); repaint();
    }
    void syncLocal() {
        name.setText(role == input ? "Sua entrada" : juce::String::fromUTF8("Saída principal"), juce::dontSendNotification);
        subtitle.setText(role == input ? "ENTRADA" : juce::String::fromUTF8("SAÍDA"), juce::dontSendNotification);
        auto& source = role == input ? processor.getInputMeterSource() : processor.getOutputMeterSource();
        source.decayIfNeeded();
        meterLeft = source.getNumChannels() > 0 ? source.getMaxLevel(0) : 0;
        meterRight = source.getNumChannels() > 1 ? source.getMaxLevel(1) : meterLeft;
        syncGain(readParameter(role == input ? SonobusAudioProcessor::paramInGain : SonobusAudioProcessor::paramWet));
        const bool muted = readParameter(role == input ? SonobusAudioProcessor::paramMainInMute : SonobusAudioProcessor::paramMainRecvMute) > 0;
        mute.setToggleState(muted, juce::dontSendNotification);
        setStatus(role == input ? juce::String::fromUTF8(processor.isSystemInputSuppressed() ? "Fora do envio: fontes individuais em uso." : muted ? "Entrada silenciada. Pads e fontes continuam." : "Entrada disponível para os conectados.")
            : juce::String::fromUTF8(muted ? "Recebidos silenciados. Pads locais continuam." : "Volume dos sons na saída principal."));
        if (role == input) secondary.setToggleState(readParameter(SonobusAudioProcessor::paramDry) > 0, juce::dontSendNotification);
        repaint();
    }
    void setSelected(bool yes) { selected = yes; repaint(); }
    const juce::String& connectionId() const { return id; }
    std::function<void(const juce::String&)> onDetails;
private:
    SonobusAudioProcessor& processor;
    Role role;
    juce::String id;
    float readParameter(const juce::String& key) const { return processor.getValueTreeState().getRawParameterValue(key)->load(); }
    void writeParameter(const juce::String& key, float value) {
        auto* parameter = processor.getValueTreeState().getParameter(key);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
    juce::RangedAudioParameter* gainParameter() {
        return role == peer ? nullptr : processor.getValueTreeState().getParameter(role == input ? SonobusAudioProcessor::paramInGain : SonobusAudioProcessor::paramWet);
    }
};

class SonoCraftMixerView final : public juce::Component, private juce::Timer {
public:
    explicit SonoCraftMixerView(SonobusAudioProcessor& p) : processor(p), input(p, SonoCraftChannelStrip::input), master(p, SonoCraftChannelStrip::master)
    {
        for (auto* c : std::initializer_list<juce::Component*> { &input, &master, &viewport, &session,
                         &sourceMode, &sourceRouting, &brand, &connectIP, &connectGroup, &audio, &settings, &record, &metronome, &metEnabled, &metTempo, &metLevel, &metSend,
                         &file, &routing, &effects, &sources, &chat, &soundboard, &play, &stop, &disconnect, &inspector, &bufferMode, &bufferTime })
            addAndMakeVisible(c);
        brand.setImage(juce::ImageCache::getFromMemory(BinaryData::sonocraft_icon_256_png, BinaryData::sonocraft_icon_256_pngSize));
        viewport.setViewedComponent(&channels, false);
        viewport.setScrollBarsShown(true, true);
        session.setFont(juce::Font(13.0f));
        session.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        connectIP.setButtonText("Conectar por IP"); connectGroup.setButtonText("Entrar em grupo");
        connectIP.setColour(juce::TextButton::buttonColourId, SonoCraftTheme::selection());
        audio.setButtonText(juce::String::fromUTF8("Áudio e saídas")); settings.setButtonText("Ajustes");
        play.setButtonText("Reproduzir"); stop.setButtonText("Parar");
        play.onClick = [this] { if (onPlay) onPlay(); };
        stop.onClick = [this] { processor.getTransportSource().stop(); processor.getTransportSource().setPosition(0.0); };
        record.setButtonText("Gravar"); metronome.setButtonText(juce::String::fromUTF8("Metrônomo")); file.setButtonText("Arquivo");
        routing.setButtonText("Roteamento"); effects.setButtonText("Efeitos");
        sources.setButtonText("Adicionar fonte"); sources.setTooltip(juce::String::fromUTF8("Adicionar um canal de aplicativo ou aba à mesa"));
        sourceRouting.setFont(12.0f); sourceRouting.setMinimumHorizontalScale(1);
        sourceRouting.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        sourceMode.onClick = [this] {
            auto& mixer=processor.getSourceMixer();
            if (mixer.usesIndividualSources()) mixer.useCompleteInput(); else mixer.useIndividualSources();
            refresh();
        };
        chat.setButtonText("Chat"); soundboard.setButtonText("Soundboard"); disconnect.setButtonText("Desconectar");
        connectIP.onClick = [this] { if (onConnectIP) onConnectIP(); };
        connectGroup.onClick = [this] { if (onConnectGroup) onConnectGroup(); };
        audio.onClick = [this] { if (onAudio) onAudio(); };
        settings.onClick = [this] { if (onSettings) onSettings(); };
        record.onClick = [this] { if (onRecord) onRecord(); };
        metronome.setTooltip(juce::String::fromUTF8("Abrir controles de tempo, volume e envio do metrônomo"));
        metronome.onClick = [this] { metControlsVisible = !metControlsVisible; resized(); };
        metEnabled.setButtonText("Ativar"); metSend.setButtonText("Enviar ao grupo");
        metTempo.setSliderStyle(juce::Slider::LinearHorizontal);
        metTempo.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 26);
        metTempo.setTitle("Tempo do metronomo (BPM)");
        metLevel.setSliderStyle(juce::Slider::LinearHorizontal);
        metLevel.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 26);
        metLevel.setTitle("Volume do metronomo");
        auto& state = processor.getValueTreeState();
        metTempoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, SonobusAudioProcessor::paramMetTempo, metTempo);
        metLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, SonobusAudioProcessor::paramMetGain, metLevel);
        metEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state, SonobusAudioProcessor::paramMetEnabled, metEnabled);
        metSendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state, SonobusAudioProcessor::paramSendMetAudio, metSend);
        metLevel.setTooltip(juce::String::fromUTF8("Volume do metrônomo, de 0 a 1"));
        soundboard.setTooltip(juce::String::fromUTF8("Abrir pads para carregar e disparar vinhetas, músicas e efeitos"));
        file.onClick = [this] { if (onFile) onFile(); };
        routing.onClick = [this] { if (onRouting) onRouting(); };
        effects.onClick = [this] { if (onEffects) onEffects(); };
        sources.onClick = [this] {
            if (!sourceDialog) {
                sourceDialog=std::make_unique<SonoCraftSourceControls>(processor);
                addChildComponent(sourceDialog.get());
                sourceDialog->onClose=[this]{sourceDialog->setVisible(false);};
            }
            sourceDialog->setVisible(true); sourceDialog->toFront(true); resized();
        };
        chat.onClick = [this] { if (onChat) onChat(); };
        soundboard.onClick = [this] { if (onSoundboard) onSoundboard(); };
        disconnect.onClick = [this] { processor.disconnectFromServer(); };
        input.onDetails = master.onDetails = [this](const juce::String&) { if (onAudio) onAudio(); };
        bufferMode.addItem(juce::String::fromUTF8("Automático"), 1); bufferMode.addItem("Fixo", 2);
        bufferMode.onChange = [this] { applyBuffer(); };
        bufferTime.setRange(0, 1000, 1); bufferTime.setTextValueSuffix(" ms");
        bufferTime.setSliderStyle(juce::Slider::LinearHorizontal);
        bufferTime.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 24);
        bufferTime.onValueChange = [this] { applyBuffer(); };
        inspector.setFont(juce::Font(13.0f));
        processor.getSourceMixer().startDiscovery();
        refresh(); startTimerHz(24);
    }
    ~SonoCraftMixerView() override { stopTimer(); viewport.setViewedComponent(nullptr, false); }
    std::function<void()> onConnectIP, onConnectGroup, onAudio, onSettings, onRecord, onMetronome, onFile, onRouting, onEffects, onChat, onSoundboard, onPlay;
    juce::Component* settingsAnchor() { return &settings; }
    juce::Component* connectionAnchor() { return &connectIP; }

    void resized() override
    {
        if (sourceDialog) sourceDialog->setBounds(getLocalBounds().withSizeKeepingCentre(juce::jmin(450,getWidth()-24),juce::jmin(370,getHeight()-24)));
        auto bounds = getLocalBounds().reduced(18, 12);
        const bool compact = getWidth() < 840;
        auto header = bounds.removeFromTop(compact ? 82 : 46);
        auto top = compact ? header.removeFromTop(42) : header;
        brand.setBounds(top.removeFromLeft(38).reduced(2)); top.removeFromLeft(164);
        settings.setBounds(top.removeFromRight(74).reduced(2, 6));
        audio.setBounds(top.removeFromRight(144).reduced(2, 6)); top.removeFromRight(10);
        auto connections = compact ? header : top;
        connectGroup.setBounds(connections.removeFromRight(132).reduced(2, compact ? 2 : 6));
        connectIP.setBounds(connections.removeFromRight(132).reduced(2, compact ? 2 : 6));
        bounds.removeFromTop(10);
        auto sessionBar = bounds.removeFromTop(34);
        disconnect.setBounds(sessionBar.removeFromRight(112).reduced(0, 3));
        session.setBounds(sessionBar);
        auto sourceBar = bounds.removeFromTop(46);
        if (sourceMode.isVisible()) sourceMode.setBounds(sourceBar.removeFromRight(180).reduced(2, 7));
        sourceRouting.setBounds(sourceBar);
        bounds.removeFromTop(10);
        const bool wrapTransport = bounds.getWidth() < 1050;
        auto transportArea = bounds.removeFromBottom(wrapTransport ? 82 : 44);
        auto transport = wrapTransport ? transportArea.removeFromTop(40) : transportArea;
        for (auto* button : { &play, &stop, &record, &metronome, &file, &routing, &effects, &sources, &chat, &soundboard }) {
            const auto width = button == &sources ? 130 : button == &soundboard ? 104 : button == &metronome || button == &routing ? 105 : 78;
            if (wrapTransport && transport.getWidth() < width) transport = transportArea;
            button->setBounds(transport.removeFromLeft(width).reduced(3, 6));
        }
        bounds.removeFromBottom(10);
        for (auto* control : std::initializer_list<juce::Component*> { &metEnabled, &metTempo, &metLevel, &metSend }) control->setVisible(metControlsVisible);
        if (metControlsVisible) {
            auto row = bounds.removeFromBottom(44).reduced(4, 6);
            metEnabled.setBounds(row.removeFromLeft(78));
            metSend.setBounds(row.removeFromRight(140));
            const int half = row.getWidth() / 2;
            metTempo.setBounds(row.removeFromLeft(half).reduced(6, 0));
            metLevel.setBounds(row.reduced(6, 0));
            bounds.removeFromBottom(8);
        }
        const bool showInspector = selectedId.isNotEmpty();
        inspector.setVisible(showInspector); bufferMode.setVisible(showInspector); bufferTime.setVisible(showInspector);
        if (showInspector) {
            auto details = bounds.removeFromBottom(94).reduced(12, 6);
            inspector.setBounds(details.removeFromTop(34));
            bufferMode.setBounds(details.removeFromLeft(140).withHeight(28)); details.removeFromLeft(16);
            bufferTime.setBounds(details.removeFromLeft(juce::jmin(380, details.getWidth())).withHeight(28));
            bounds.removeFromBottom(12);
        }
        const auto busWidth = getWidth() < 700 ? 156 : SonoCraftStrip::preferredWidth;
        const int channelHeight = juce::jmax(100, bounds.getHeight() - viewport.getScrollBarThickness());
        input.setBounds(bounds.removeFromLeft(busWidth).withHeight(channelHeight)); bounds.removeFromLeft(14);
        master.setBounds(bounds.removeFromRight(busWidth).withHeight(channelHeight)); bounds.removeFromRight(14);
        viewport.setBounds(bounds);
        const auto width = juce::jmax(bounds.getWidth(), (strips.size() + sourceStrips.size()) * (busWidth + 12));
        channels.setSize(width, channelHeight);
        int x = 0;
        for (auto* strip : sourceStrips) { strip->setBounds(x, 0, busWidth, channels.getHeight()); x += busWidth + 12; }
        for (auto* strip : strips) { strip->setBounds(x, 0, busWidth, channels.getHeight()); x += busWidth + 12; }
    }
    void paint(juce::Graphics& g) override
    {
        using namespace SonoCraftTheme;
        g.fillAll(canvas());
        g.setColour(text()); g.setFont(juce::Font(19.0f, juce::Font::bold));
        g.drawText("SonoCraft", 62, 13, 150, 26, juce::Justification::centredLeft);
        g.setFont(11.5f); g.setColour(secondary());
        g.drawText(juce::String::fromUTF8("MESA DE ÁUDIO"), 63, 39, 150, 16, juce::Justification::centredLeft);
        g.setColour(border()); g.drawHorizontalLine(getWidth() < 840 ? 102 : 66, 18, float(getWidth() - 18));
        if (strips.isEmpty() && sourceStrips.isEmpty()) {
            auto empty = viewport.getBounds().reduced(12);
            g.setColour(panel().withAlpha(0.7f)); g.fillRoundedRectangle(empty.toFloat(), 8);
            g.setColour(text()); g.setFont(juce::Font(empty.getWidth() < 250 ? 14.0f : 18.0f, juce::Font::bold));
            g.drawText(juce::String::fromUTF8("Reproduza um som para começar"), empty.withHeight(30).withY(empty.getCentreY() - 40), juce::Justification::centred);
            g.setColour(secondary()); g.setFont(13.0f);
            g.drawText(juce::String::fromUTF8("Os aplicativos com áudio aparecerão aqui."), empty.withHeight(24).withY(empty.getCentreY()), juce::Justification::centred);
            g.drawText(juce::String::fromUTF8("Use Adicionar fonte para escolher uma aba."), empty.withHeight(24).withY(empty.getCentreY() + 24), juce::Justification::centred);
        }
    }
private:
    SonobusAudioProcessor& processor;
    SonoCraftChannelStrip input, master;
    juce::Viewport viewport;
    juce::Component channels;
    juce::OwnedArray<SonoCraftChannelStrip> strips;
    juce::OwnedArray<SonoCraftSourceStrip> sourceStrips;
    std::unique_ptr<SonoCraftSourceControls> sourceDialog;
    juce::TextButton sourceMode;
    juce::Label sourceRouting;
    juce::ImageComponent brand;
    juce::Label session, inspector;
    juce::TextButton connectIP, connectGroup, audio, settings, record, metronome, file, routing, effects, sources, chat, soundboard, play, stop, disconnect;
    juce::ComboBox bufferMode;
    juce::Slider bufferTime;
    juce::String selectedId;
    bool syncing = false, metControlsVisible = false;
    juce::ToggleButton metEnabled, metSend;
    juce::Slider metTempo, metLevel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> metTempoAttachment, metLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> metEnabledAttachment, metSendAttachment;
    void timerCallback() override { refresh(); }
    void applyBuffer() {
        if (!syncing && selectedId.isNotEmpty())
            processor.setMixerChannelBuffer(selectedId, float(bufferTime.getValue()), bufferMode.getSelectedId() == 1);
    }
    void refresh()
    {
        processor.getSourceMixer().pollDiscovery();
        const auto sourcesState = processor.getSourceMixer().views();
        bool sourcesChanged = sourcesState.size() != sourceStrips.size();
        if (!sourcesChanged) for (int i=0;i<sourcesState.size();++i) sourcesChanged |= sourcesState[i].id != sourceStrips[i]->sourceId();
        if (sourcesChanged) {
            sourceStrips.clear();
            for (const auto& state:sourcesState) channels.addAndMakeVisible(sourceStrips.add(new SonoCraftSourceStrip(processor,state.id)));
        }
        for (int i=0;i<sourcesState.size();++i) sourceStrips[i]->sync(sourcesState[i]);
        const bool virtualInput = SonoCraftAudioRouting::isVirtualCable(processor.getStandaloneInputName());
        const bool modeVisibilityChanged = sourceMode.isVisible() != virtualInput;
        sourceMode.setVisible(virtualInput);
        const bool individual = processor.getSourceMixer().usesIndividualSources();
        sourceMode.setButtonText(individual ? "Usar entrada completa" : "Usar fontes individuais");
        sourceRouting.setText(juce::String::fromUTF8(virtualInput
            ? (individual ? "Fontes individuais: a entrada completa do sistema fica fora do envio."
                          : "Entrada completa: o sistema inteiro pode ser enviado. Fontes individuais desligadas.")
            : "Aplicativos com áudio aparecem automaticamente. Ative Enviar nos canais que deseja compartilhar."),juce::dontSendNotification);
        const auto discoveryStatus=processor.getSourceMixer().discoveryStatus();
        sourceRouting.setTooltip(discoveryStatus.isEmpty()?juce::String::fromUTF8("A detecção automática não captura nem envia áudio. Abas são escolhidas em Adicionar fonte."):discoveryStatus);
        if(discoveryStatus.isNotEmpty())sourceRouting.setText(discoveryStatus,juce::dontSendNotification);
        if (sourcesChanged || modeVisibilityChanged) resized();
        auto states = processor.getMixerChannelStates();
        bool changed = states.size() != strips.size();
        if (!changed) for (int i = 0; i < states.size(); ++i) changed |= states[i].id != strips[i]->connectionId();
        if (changed) {
            strips.clear();
            for (auto& state : states) {
                auto* strip = strips.add(new SonoCraftChannelStrip(processor, SonoCraftChannelStrip::peer, state.id));
                strip->onDetails = [this](const juce::String& id) { selectedId = selectedId == id ? juce::String{} : id; refresh(); resized(); };
                channels.addAndMakeVisible(strip);
            }
            if (processor.findRemotePeerConnectionId(selectedId) < 0) selectedId.clear();
            resized();
        }
        syncing = true;
        for (int i = 0; i < states.size(); ++i) {
            strips[i]->setState(states[i]); strips[i]->setSelected(states[i].id == selectedId);
            if (states[i].id == selectedId) {
                inspector.setText(states[i].name + juce::String::fromUTF8("   ·   ") + states[i].address + juce::String::fromUTF8("   ·   Envio: ") + states[i].codec + juce::String::fromUTF8("   ·   Buffer de recepção"), juce::dontSendNotification);
                bufferMode.setSelectedId(states[i].bufferMode == 0 ? 2 : 1, juce::dontSendNotification);
                if (!bufferTime.isMouseButtonDown()) bufferTime.setValue(states[i].bufferMs, juce::dontSendNotification);
                bufferTime.setEnabled(states[i].bufferMode == 0);
            }
        }
        syncing = false;
        input.syncLocal(); master.syncLocal();
        const auto group = processor.getCurrentJoinedGroup();
        session.setText((group.isNotEmpty() ? "Grupo: " + group : juce::String::fromUTF8("Sessão local")) + juce::String::fromUTF8("   ·   ") + juce::String(states.size()) + (states.size() == 1 ? " participante" : " participantes"), juce::dontSendNotification);
        disconnect.setEnabled(!states.isEmpty() || processor.isConnectedToServer());
        const bool hasFile = processor.getTransportSource().getLengthInSeconds() > 0;
        play.setEnabled(hasFile); stop.setEnabled(hasFile);
        play.setButtonText(processor.getTransportSource().isPlaying() ? "Pausar" : "Reproduzir");
        record.setToggleState(processor.isRecordingToFile(), juce::dontSendNotification);
        metronome.setToggleState(metControlsVisible, juce::dontSendNotification);
        repaint();
    }
};
