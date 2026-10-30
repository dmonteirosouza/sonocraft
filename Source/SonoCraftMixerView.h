// SonoCraft's desktop mixing workspace. All meters are GUI snapshots;
// audio processing remains owned by SonobusAudioProcessor.
#pragma once
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"
#include "SonoCraftSourceControls.h"
#include "SonoCraftStrip.h"
#include "SonoCallOutBox.h"

// Draw menu indicators as geometry: the Windows UI font need not contain ▾.
class SonoCraftNavigationButton : public juce::TextButton {
public:
    bool opensMenu = false;
    void paintButton(juce::Graphics& g, bool over, bool down) override {
        if (!opensMenu) { juce::TextButton::paintButton(g, over, down); return; }
        auto& look = getLookAndFeel();
        look.drawButtonBackground(g, *this, findColour(getToggleState() ? buttonOnColourId : buttonColourId), over, down);
        g.setColour(findColour(getToggleState() ? textColourOnId : textColourOffId).withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f));
        g.setFont(look.getTextButtonFont(*this, getHeight()));
        g.drawFittedText(getButtonText(), getLocalBounds().reduced(8, 2).withTrimmedRight(16), juce::Justification::centred, 1);
        const float x = float(getWidth() - 16), y = getHeight() * 0.5f;
        juce::Path chevron;
        chevron.startNewSubPath(x - 4, y - 2); chevron.lineTo(x, y + 2); chevron.lineTo(x + 4, y - 2);
        g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};

inline bool sonoCraftMainOutputAvailable(SonobusAudioProcessor& processor) {
    return !processor.isStandaloneOutputBlocked() && processor.getMainBusNumOutputChannels() > 0
        && (!juce::JUCEApplicationBase::isStandaloneApp() || processor.getStandaloneOutputName().isNotEmpty());
}

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
        mute.setTooltip(juce::String::fromUTF8(role == input ? "Silenciar a entrada de áudio, mantendo os pads e fontes selecionadas"
            : role == master ? "Silenciar todos os participantes recebidos. Pads, arquivos e metrônomo locais continuam tocando."
                             : "Silenciar o áudio deste participante, sem interromper a conexão"));
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
        details.setVisible(role == peer);
        secondary.setVisible(role != master);
        fader.setTitle(role == input ? "Volume da entrada (dB)" : role == master ? juce::String::fromUTF8("Volume da saída principal (dB)") : "Volume do participante (dB)");
    }
    void setState(const SonobusAudioProcessor::MixerChannelState& state, bool excludedBySolo) {
        name.setText(state.name, juce::dontSendNotification);
        subtitle.setText("PARTICIPANTE", juce::dontSendNotification);
        name.setTooltip(state.name + juce::String::fromUTF8(" · ") + state.address);
        const bool silent = state.muted || excludedBySolo || state.gain <= 0.00001f;
        const bool mainOutput = sonoCraftMainOutputAvailable(processor) && readParameter(SonobusAudioProcessor::paramWet) > 0;
        const bool additionalOutput = processor.getPadMonitor().isActive();
        const bool ready = state.connected && state.receiving && !state.safetyMuted;
        setStatus(juce::String::fromUTF8(!state.connected ? "Aguardando conexão" : excludedBySolo ? "Outro canal em solo"
            : state.muted ? "Recepção silenciada" : state.gain <= 0.00001f ? "Volume zerado"
            : !ready ? "Aguardando áudio" : "Recebendo"),
            state.address, silent ? SonoCraftPresentation::Tone::attention : ready ? SonoCraftPresentation::Tone::active : SonoCraftPresentation::Tone::neutral);
        setRoutes(juce::String::fromUTF8(silent ? "Escuta: silenciada" : !ready ? "Escuta: aguardando"
            : mainOutput && additionalOutput ? "Escuta: duas saídas" : mainOutput ? "Escuta: saída principal"
            : additionalOutput ? "Escuta: saída adicional" : "Escuta: sem saída"),
            "Origem: participante", juce::String::fromUTF8("Áudio recebido deste participante. O volume e o silêncio deste canal controlam a escuta local. Rotas avançadas ficam em Ferramentas."));
        updateMeter(state.left, state.right);
        setTooltip(juce::String::fromUTF8("Nível recebido deste participante, incluindo entrada, pads e fontes separadas."));
        mute.setToggleState(state.muted, juce::dontSendNotification);
        secondary.setToggleState(state.soloed, juce::dontSendNotification);
        syncGain(state.gain);
    }
    void syncLocal(bool hasRecipients, bool sendPaused) {
        name.setText(role == input ? "Sua entrada" : juce::String::fromUTF8("Saída principal"), juce::dontSendNotification);
        subtitle.setText(role == input ? "ENTRADA" : juce::String::fromUTF8("SAÍDA"), juce::dontSendNotification);
        auto& source = role == input ? processor.getInputMeterSource() : processor.getOutputMeterSource();
        source.decayIfNeeded();
        const float left = source.getNumChannels() > 0 ? source.getMaxLevel(0) : 0;
        updateMeter(left, source.getNumChannels() > 1 ? source.getMaxLevel(1) : left);
        syncGain(readParameter(role == input ? SonobusAudioProcessor::paramInGain : SonobusAudioProcessor::paramWet));
        const bool muted = readParameter(role == input ? SonobusAudioProcessor::paramMainInMute : SonobusAudioProcessor::paramMainRecvMute) > 0;
        mute.setToggleState(muted, juce::dontSendNotification);
        const bool outputAvailable = sonoCraftMainOutputAvailable(processor);
        if (role == input) {
            const bool suppressed = processor.isSystemInputSuppressed();
            const bool zeroGain = readParameter(SonobusAudioProcessor::paramInGain) <= 0.00001f;
            const bool blocked = muted || suppressed || zeroGain;
            const bool monitorSelected = readParameter(SonobusAudioProcessor::paramDry) > 0;
            const bool soloBlocks = processor.isAnythingSoloed() && readParameter(SonobusAudioProcessor::paramMainMonitorSolo) <= 0;
            const bool listen = monitorSelected && !blocked && !soloBlocks && outputAvailable && readParameter(SonobusAudioProcessor::paramWet) > 0;
            setStatus(juce::String::fromUTF8(suppressed ? "Fontes individuais" : muted ? "Entrada silenciada" : zeroGain ? "Volume zerado"
                : sendPaused ? "Envio pausado" : meterLeft > 0.0001f || meterRight > 0.0001f ? "Com sinal" : "Pronta"),
                juce::String::fromUTF8(suppressed ? "A entrada completa fica fora do envio para não duplicar as fontes individuais."
                    : "A entrada é independente dos pads e dos aplicativos. Ouvir entrada controla somente sua escuta local."),
                blocked || sendPaused ? SonoCraftPresentation::Tone::attention : SonoCraftPresentation::Tone::neutral);
            setRoutes(juce::String::fromUTF8(listen ? "Escuta: saída principal" : monitorSelected ? "Escuta: indisponível" : "Escuta: desligada"),
                juce::String::fromUTF8(blocked ? "Envio: silenciado" : sendPaused ? "Envio: pausado" : hasRecipients ? "Envio: aos conectados" : "Envio: sem conexão"));
            secondary.setToggleState(monitorSelected, juce::dontSendNotification);
        } else {
            setStatus(juce::String::fromUTF8(!outputAvailable ? "Sem saída principal" : muted ? "Recebidos silenciados"
                : meterLeft > 0.0001f || meterRight > 0.0001f ? "Reproduzindo" : "Sem som agora"),
                juce::String::fromUTF8("Este volume controla a saída principal. Silenciar recebidos afeta os participantes; pads, arquivos e metrônomo locais continuam."),
                muted || !outputAvailable ? SonoCraftPresentation::Tone::attention : SonoCraftPresentation::Tone::neutral);
            setRoutes(juce::String::fromUTF8(outputAvailable ? "Escuta: saída principal" : "Escuta: sem dispositivo"), "Uso: escuta local",
                juce::String::fromUTF8("Dispositivo: ") + processor.getStandaloneOutputName() + juce::String::fromUTF8(". A saída adicional tem controle independente em Ajustes."));
        }
    }
    void setSelected(bool yes) { if (selected != yes) { selected = yes; repaint(); } }
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
    explicit SonoCraftMixerView(SonobusAudioProcessor& p, bool discoverSources = true) : processor(p), input(p, SonoCraftChannelStrip::input), master(p, SonoCraftChannelStrip::master) {
        for (auto* c : std::initializer_list<juce::Component*> { &input, &master, &viewport, &session, &sourceRouting, &brand,
                &connections, &sources, &soundboard, &chat, &tools, &settings,
                &toolTitle, &toolHelp, &closeTool, &file, &play, &stop, &record, &playingActivity, &recordingActivity, &metronomeActivity,
                &playerTab, &recorderTab, &metronomeTab, &moreTools, &tempoLabel, &levelLabel,
                &metEnabled, &metTempo, &metLevel, &metSend, &inspector, &bufferMode, &bufferTime })
            addAndMakeVisible(c);
        brand.setImage(juce::ImageCache::getFromMemory(BinaryData::sonocraft_icon_256_png, BinaryData::sonocraft_icon_256_pngSize));
        viewport.setComponentID("mixer.channels");
        toolViewport.setComponentID("tools.viewport");
        closeTool.setComponentID("tools.close");
        viewport.setViewedComponent(&channels, false);
#if !(JUCE_IOS || JUCE_ANDROID)
        addAndMakeVisible(toolViewport);
        toolViewport.setViewedComponent(&toolContent, false);
        toolViewport.setScrollBarsShown(true, false);
        for (auto* c : std::initializer_list<juce::Component*> { &toolHelp, &file, &play, &stop, &record,
                &metEnabled, &metSend, &metTempo, &metLevel, &tempoLabel, &levelLabel, &moreTools })
            toolContent.addAndMakeVisible(c);
#endif
        viewport.setScrollBarsShown(true, true);
        session.setFont(13.0f); session.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        session.setJustificationType(juce::Justification::centredRight);
        connections.setButtonText(juce::String::fromUTF8("Conexões")); connections.opensMenu = true;
        sources.setButtonText("Fontes"); soundboard.setButtonText("Pads"); chat.setButtonText("Conversa");
        tools.setButtonText("Ferramentas"); tools.opensMenu = false; settings.setButtonText("Ajustes");
        for (auto* button : { &connections, &sources, &soundboard, &chat, &tools, &settings }) {
            button->setColour(juce::TextButton::buttonColourId, SonoCraftTheme::panel());
            button->setColour(juce::TextButton::buttonOnColourId, SonoCraftTheme::selection());
            button->setColour(juce::TextButton::textColourOffId, SonoCraftTheme::text());
            button->setColour(juce::TextButton::textColourOnId, SonoCraftTheme::accent());
        }
        connections.setTooltip(juce::String::fromUTF8("Entrar em grupo, conectar por IP ou desconectar"));
        sources.setTooltip(juce::String::fromUTF8("Adicionar aplicativos ou abas e escolher o modo de envio"));
        soundboard.setTooltip("Abrir ou fechar a mesa de pads");
        chat.setTooltip("Abrir ou fechar a conversa com os participantes");
        tools.setTooltip(juce::String::fromUTF8("Arquivo, gravação, metrônomo, efeitos e roteamento"));
        settings.setTooltip(juce::String::fromUTF8("Dispositivos de áudio, saídas, gravação e preferências"));
        connections.onClick = [this] { showConnections(); };
        tools.onClick = [this] { showTools(); };
        for (auto* activity : { &playingActivity, &recordingActivity, &metronomeActivity }) {
            activity->setColour(juce::TextButton::buttonColourId, SonoCraftTheme::selection());
            activity->setColour(juce::TextButton::textColourOffId, SonoCraftTheme::accent());
            activity->setTooltip("Ativo mesmo com o painel fechado. Clique para abrir os controles.");
            activity->setVisible(false);
        }
        recordingActivity.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffe6cf));
        recordingActivity.setColour(juce::TextButton::textColourOffId, SonoCraftTheme::warning());
        playingActivity.onClick = [this] { selectTool(Tool::player); };
        recordingActivity.onClick = [this] { selectTool(Tool::recorder); };
        metronomeActivity.onClick = [this] { selectTool(Tool::metronome); };
        settings.onClick = [this] { if (onSettings) onSettings(); };
        chat.onClick = [this] { if (onChat) onChat(); };
        soundboard.onClick = [this] { if (onSoundboard) onSoundboard(); };
        sources.onClick = [this] {
            if (!sourceDialog) {
                sourceDialog = std::make_unique<SonoCraftSourceControls>(processor);
                addChildComponent(sourceDialog.get());
                sourceDialog->onClose = [this] { sourceDialog->setVisible(false); };
            }
            sourceDialog->refreshMode();
            if (sourcePopup) sourcePopup->toFront(true);
            else sourcePopup = &SonoCallOutBox::launchForComponent(*sourceDialog, *this, 470, 470);
        };
        sourceRouting.setFont(12.0f); sourceRouting.setMinimumHorizontalScale(1);
        sourceRouting.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        playerTab.setButtonText("Arquivo"); recorderTab.setButtonText(juce::String::fromUTF8("Gravação"));
        metronomeTab.setButtonText(juce::String::fromUTF8("Metrônomo"));
        moreTools.setButtonText("Mais controles");
        moreTools.onClick = [this] { showAdvancedTools(); };
        playerTab.onClick = [this] { selectTool(Tool::player); };
        recorderTab.onClick = [this] { selectTool(Tool::recorder); };
        metronomeTab.onClick = [this] { selectTool(Tool::metronome); };
        for (auto* tab : { &playerTab, &recorderTab, &metronomeTab }) {
            tab->setColour(juce::TextButton::buttonColourId, SonoCraftTheme::recessed());
            tab->setColour(juce::TextButton::buttonOnColourId, SonoCraftTheme::selection());
            tab->setColour(juce::TextButton::textColourOnId, SonoCraftTheme::accent());
        }
        tempoLabel.setText("Tempo", juce::dontSendNotification);
        levelLabel.setText("Volume", juce::dontSendNotification);
        for (auto* label : { &tempoLabel, &levelLabel }) {
            label->setFont(juce::Font(13.0f, juce::Font::bold));
            label->setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        }
        toolTitle.setFont(juce::Font(17.0f, juce::Font::bold));
        toolHelp.setFont(12.0f); toolHelp.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        toolHelp.setMinimumHorizontalScale(1.0f);
        toolHelp.setJustificationType(juce::Justification::topLeft);
        closeTool.configureAsCloseControl();
        closeTool.setTooltip(juce::String::fromUTF8("Ocultar os controles sem parar a reprodução, a gravação ou o metrônomo"));
        closeTool.onClick = [this] { selectTool(Tool::none); };
        file.setButtonText("Abrir arquivo"); play.setButtonText("Reproduzir"); stop.setButtonText("Parar");
        file.onClick = [this] { if (onFile) onFile(); };
        play.onClick = [this] { if (onPlay) onPlay(); };
        stop.onClick = [this] { processor.getTransportSource().stop(); processor.getTransportSource().setPosition(0.0); };
        record.onClick = [this] { if (onRecord) onRecord(); };
        record.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffe6cf));
        metEnabled.setButtonText("Ativar"); metSend.setButtonText("Enviar aos conectados");
        metTempo.setSliderStyle(juce::Slider::LinearHorizontal);
        metTempo.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 26);
        metTempo.setTitle(juce::String::fromUTF8("Tempo do metrônomo (BPM)"));
        metLevel.setSliderStyle(juce::Slider::LinearHorizontal);
        metLevel.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 26);
        metLevel.setTitle(juce::String::fromUTF8("Volume do metrônomo (dB)"));
        auto& state = processor.getValueTreeState();
        metTempoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, SonobusAudioProcessor::paramMetTempo, metTempo);
        metLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, SonobusAudioProcessor::paramMetGain, metLevel);
        metEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state, SonobusAudioProcessor::paramMetEnabled, metEnabled);
        metSendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state, SonobusAudioProcessor::paramSendMetAudio, metSend);
        metTempo.textFromValueFunction = [](double bpm) { return juce::String(juce::roundToInt(bpm)) + " BPM"; };
        metTempo.valueFromTextFunction = [](const juce::String& text) { return text.replaceCharacter(',', '.').getDoubleValue(); };
        metTempo.updateText();
        metLevel.textFromValueFunction = [](double gain) { return gain <= 0 ? juce::String::fromUTF8("−∞ dB") : juce::String(juce::Decibels::gainToDecibels(gain), 1) + " dB"; };
        metLevel.valueFromTextFunction = [](const juce::String& text) { return text.contains(juce::String::fromUTF8("∞")) ? 0.0 : juce::Decibels::decibelsToGain(text.replaceCharacter(',', '.').getDoubleValue()); };
        metLevel.updateText();
        metLevel.setTooltip(juce::String::fromUTF8("Volume do metrônomo. 0 dB mantém o nível original."));
        bufferMode.addItem(juce::String::fromUTF8("Automático"), 1); bufferMode.addItem("Fixo", 2);
        bufferMode.onChange = [this] { applyBuffer(); };
        bufferTime.setRange(0, 1000, 1); bufferTime.setTextValueSuffix(" ms");
        bufferTime.setSliderStyle(juce::Slider::LinearHorizontal);
        bufferTime.setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 24);
        bufferTime.onValueChange = [this] { applyBuffer(); };
        inspector.setFont(13.0f);
#if !(JUCE_IOS || JUCE_ANDROID)
        if (discoverSources) processor.getSourceMixer().startDiscovery();
#endif
#if JUCE_IOS || JUCE_ANDROID
        juce::ignoreUnused(discoverSources);
        sources.setVisible(false); sourceRouting.setVisible(false);
        connections.setButtonText("Conectar"); tools.setButtonText("Mais"); tools.opensMenu = true;
        viewport.setScrollBarsShown(true, false);
#endif
        refresh();
#if JUCE_IOS || JUCE_ANDROID
        startTimerHz(20);
#else
        startTimerHz(24);
#endif
    }
    ~SonoCraftMixerView() override { stopTimer(); viewport.setViewedComponent(nullptr, false); toolViewport.setViewedComponent(nullptr, false); }
    std::function<void()> onConnectIP, onConnectGroup, onSettings, onRecord, onFile, onRouting, onEffects, onChat, onSoundboard, onPlay;
    std::function<bool()> isChatVisible, isPadsVisible, isSettingsVisible, isConnectionVisible, isAdvancedToolVisible;
    juce::Component* connectionAnchor() { return &connections; }
    juce::Component* toolsAnchor() { return &tools; }
    juce::Component* recordingAnchor() { return record.isVisible() ? static_cast<juce::Component*>(&record) : &tools; }

    // Views remain owned by the editor. Docking never enters a modal loop or
    // disables the mixer; closing a panel keeps its contents and transport alive.
    void showDesktopPanel(juce::Component& content) {
        tool = Tool::none;
        if (desktopPanel && desktopPanel != &content) desktopPanel->setVisible(false);
        desktopPanel = &content;
        addAndMakeVisible(content);
        resized(); refresh(); repaint();
    }
    void hideDesktopPanel(juce::Component& content) {
        content.setVisible(false);
        if (desktopPanel == &content) desktopPanel = nullptr;
        resized(); repaint();
    }
    void resized() override {
        auto bounds = getLocalBounds().reduced(18, 12);
#if JUCE_IOS || JUCE_ANDROID
        auto heading = bounds.removeFromTop(56);
        brand.setBounds(heading.removeFromLeft(36).withHeight(36));
        settings.setBounds(heading.removeFromRight(84).withHeight(44));
        session.setBounds(54, 40, juce::jmax(80, getWidth()-160), 24);
        session.setJustificationType(juce::Justification::centredLeft);
        auto connectRow = bounds.removeFromTop(44);
        connections.setBounds(connectRow); bounds.removeFromTop(14);
        auto navigation = bounds.removeFromBottom(56);
        const int navWidth = navigation.getWidth()/3;
        for(auto* button : {&soundboard, &chat, &tools})
            button->setBounds(navigation.removeFromLeft(navWidth).reduced(3,4));
        bounds.removeFromBottom(12);
#else
        auto header = bounds.removeFromTop(44);
        brand.setBounds(header.removeFromLeft(38).reduced(2)); header.removeFromLeft(140);
        session.setBounds(header);
        bounds.removeFromTop(10);
        const int columns = bounds.getWidth() >= 660 ? 6 : 3;
        auto nav = bounds.removeFromTop(columns == 6 ? 38 : 80);
        juce::TextButton* navigation[] { &connections, &sources, &soundboard, &chat, &tools, &settings };
        const int cellWidth = juce::jmin(150, nav.getWidth() / columns);
        for (int i = 0; i < 6; ++i)
            navigation[i]->setBounds(nav.getX() + (i % columns) * cellWidth, nav.getY() + (i / columns) * 42, cellWidth - 6, 34);
        bounds.removeFromTop(6);
#endif
        int activityCount = 0;
        for (auto* activity : { &playingActivity, &recordingActivity, &metronomeActivity }) if (activity->isVisible()) ++activityCount;
        if (activityCount > 0) {
            const int columns = juce::jmax(1, bounds.getWidth() / 180);
            auto area = bounds.removeFromTop(((activityCount + columns - 1) / columns) * 32);
            const int width = juce::jmin(230, area.getWidth() / juce::jmin(columns, activityCount));
            int index = 0;
            for (auto* activity : { &playingActivity, &recordingActivity, &metronomeActivity }) if (activity->isVisible()) {
                activity->setBounds(area.getX() + (index % columns) * width, area.getY() + (index / columns) * 32, width - 6, 26);
                ++index;
            }
        }
#if !(JUCE_IOS || JUCE_ANDROID)
        sourceRouting.setBounds(bounds.removeFromTop(34));
        bounds.removeFromTop(6);
#endif
        const bool hasTool = tool != Tool::none;
        for (auto* c : std::initializer_list<juce::Component*> { &toolTitle, &toolHelp, &closeTool }) c->setVisible(hasTool);
        for (auto* c : { &file, &play, &stop }) c->setVisible(tool == Tool::player);
        record.setVisible(tool == Tool::recorder);
        for (auto* c : std::initializer_list<juce::Component*> { &metEnabled, &metSend, &metTempo, &metLevel }) c->setVisible(tool == Tool::metronome);
        toolBounds = {};
        desktopPanelBounds = {};
#if JUCE_IOS || JUCE_ANDROID
        for (auto* c : std::initializer_list<juce::Component*> { &playerTab, &recorderTab, &metronomeTab, &moreTools, &tempoLabel, &levelLabel }) c->setVisible(false);
        if (hasTool) {
            const bool wrapMet = tool == Tool::metronome && bounds.getWidth() < 760;
            auto area = bounds.removeFromTop(wrapMet ? 164 : 130);
            toolBounds = area.toFloat();
            auto content = area.reduced(12, 8);
            auto titleRow = content.removeFromTop(44);
            closeTool.setBounds(titleRow.removeFromRight(titleRow.getHeight())); toolTitle.setBounds(titleRow);
            toolHelp.setBounds(content.removeFromTop(26));
            auto row = content.removeFromTop(32).withWidth(juce::jmin(900, content.getWidth()));
            if (tool == Tool::player) {
                for (auto* c : { &file, &play, &stop }) c->setBounds(row.removeFromLeft(juce::jmin(140, content.getWidth()/3)).reduced(3, 0));
            } else if (tool == Tool::recorder) record.setBounds(row.removeFromLeft(170));
            else {
                metEnabled.setBounds(row.removeFromLeft(76)); metSend.setBounds(row.removeFromLeft(180));
                if (wrapMet) row = content.removeFromTop(32).withWidth(juce::jmin(900, content.getWidth()));
                const int half = row.getWidth()/2;
                metTempo.setBounds(row.removeFromLeft(half).reduced(4, 0)); metLevel.setBounds(row.reduced(4, 0));
            }
            bounds.removeFromTop(10);
        }
#else
        const bool hasPanel = desktopPanel && desktopPanel->isVisible();
        toolViewport.setVisible(hasTool); moreTools.setVisible(hasTool);
        for (auto* c : { &playerTab, &recorderTab, &metronomeTab }) c->setVisible(hasTool);
        tempoLabel.setVisible(tool == Tool::metronome); levelLabel.setVisible(tool == Tool::metronome);
        if (hasTool || hasPanel) {
            const int preferred = hasPanel ? 400 : 330;
            const int width = juce::jlimit(300, preferred, juce::roundToInt(bounds.getWidth() * .42f));
            auto area = bounds.removeFromRight(width);
            bounds.removeFromRight(14);
            if (hasPanel) {
                desktopPanelBounds = area.toFloat();
                desktopPanel->setBounds(area.reduced(1));
            } else {
                toolBounds = area.toFloat();
                auto content = area.reduced(16, 12);
                auto titleRow = content.removeFromTop(40);
                closeTool.setBounds(titleRow.removeFromRight(36)); toolTitle.setBounds(titleRow);
                content.removeFromTop(8);
                auto tabs = content.removeFromTop(34);
                const int tabWidth = tabs.getWidth()/3;
                playerTab.setBounds(tabs.removeFromLeft(tabWidth).reduced(2, 0));
                recorderTab.setBounds(tabs.removeFromLeft(tabWidth).reduced(2, 0));
                metronomeTab.setBounds(tabs.reduced(2, 0));
                playerTab.setToggleState(tool == Tool::player, juce::dontSendNotification);
                recorderTab.setToggleState(tool == Tool::recorder, juce::dontSendNotification);
                metronomeTab.setToggleState(tool == Tool::metronome, juce::dontSendNotification);
                content.removeFromTop(16);
                toolViewport.setBounds(content);
                toolContent.setSize(content.getWidth() - toolViewport.getScrollBarThickness(),
                                    juce::jmax(content.getHeight(), tool == Tool::metronome ? 380 : 240));
                content = toolContent.getLocalBounds();
                toolHelp.setBounds(content.removeFromTop(68)); content.removeFromTop(12);
                if (tool == Tool::player) {
                    file.setBounds(content.removeFromTop(38)); content.removeFromTop(12);
                    auto transport = content.removeFromTop(38);
                    play.setBounds(transport.removeFromLeft(transport.getWidth()/2).withTrimmedRight(4));
                    stop.setBounds(transport.withTrimmedLeft(4));
                } else if (tool == Tool::recorder) {
                    record.setBounds(content.removeFromTop(40));
                } else {
                    metEnabled.setBounds(content.removeFromTop(32)); content.removeFromTop(12);
                    tempoLabel.setBounds(content.removeFromTop(24));
                    metTempo.setBounds(content.removeFromTop(36)); content.removeFromTop(16);
                    levelLabel.setBounds(content.removeFromTop(24));
                    metLevel.setBounds(content.removeFromTop(36)); content.removeFromTop(16);
                    metSend.setBounds(content.removeFromTop(32));
                }
                content.removeFromTop(24);
                moreTools.setBounds(content.removeFromTop(36));
            }
        }
#endif
        const bool showInspector = selectedId.isNotEmpty();
        inspector.setVisible(showInspector); bufferMode.setVisible(showInspector); bufferTime.setVisible(showInspector);
        if (showInspector) {
            auto details = bounds.removeFromBottom(82).reduced(12, 6);
            inspector.setBounds(details.removeFromTop(30));
            bufferMode.setBounds(details.removeFromLeft(140).withHeight(28)); details.removeFromLeft(16);
            bufferTime.setBounds(details.removeFromLeft(juce::jmin(380, details.getWidth())).withHeight(28));
            bounds.removeFromBottom(10);
        }
#if JUCE_IOS || JUCE_ANDROID
        if(input.getParentComponent()!=&channels)channels.addAndMakeVisible(input);
        if(master.getParentComponent()!=&channels)channels.addAndMakeVisible(master);
        viewport.setBounds(bounds);
        const int available = juce::jmax(100,bounds.getWidth()-viewport.getScrollBarThickness());
        const int columns = available >= 700 ? 2 : 1;
        const int width = (available - (columns-1)*12)/columns;
        int index=0, bottom=0;
        auto place = [&](juce::Component& strip, int height) {
            const int y=columns == 1 ? bottom : (index/columns)*292;
            strip.setBounds((index%columns)*(width+12),y,width,height);
            bottom=juce::jmax(bottom,y+height+12); ++index;
        };
        place(input,244); place(master,244);
        for(auto* strip:sourceStrips)place(*strip,280);
        for(auto* strip:strips)place(*strip,280);
        channels.setSize(available,juce::jmax(bounds.getHeight(),bottom+8));
        if(sourceDialog && sourceDialog->getParentComponent() == this)sourceDialog->setBounds(getLocalBounds().reduced(16));
        return;
#endif
        const int busWidth = getWidth() < 700 ? 156 : SonoCraftStrip::preferredWidth;
        const int availableHeight = juce::jmax(80, bounds.getHeight() - viewport.getScrollBarThickness());
        const bool scrollAll = availableHeight < 420 || bounds.getWidth() < 2*busWidth + 208;
        const int channelHeight = juce::jmax(420, availableHeight);
        int x = 0;
        if (scrollAll) {
            if (input.getParentComponent() != &channels) channels.addAndMakeVisible(input);
            if (master.getParentComponent() != &channels) channels.addAndMakeVisible(master);
            input.setBounds(x, 0, busWidth, channelHeight); x += busWidth + 12;
        } else {
            if (input.getParentComponent() != this) addAndMakeVisible(input);
            if (master.getParentComponent() != this) addAndMakeVisible(master);
            input.setBounds(bounds.removeFromLeft(busWidth).withHeight(channelHeight)); bounds.removeFromLeft(14);
            master.setBounds(bounds.removeFromRight(busWidth).withHeight(channelHeight)); bounds.removeFromRight(14);
        }
        viewport.setBounds(bounds);
        for (auto* strip : sourceStrips) { strip->setBounds(x, 0, busWidth, channelHeight); x += busWidth + 12; }
        for (auto* strip : strips) { strip->setBounds(x, 0, busWidth, channelHeight); x += busWidth + 12; }
        if (scrollAll) { master.setBounds(x, 0, busWidth, channelHeight); x += busWidth + 12; }
        channels.setSize(juce::jmax(bounds.getWidth(), x), channelHeight);
        if (sourceDialog && sourceDialog->getParentComponent() == this) sourceDialog->setBounds(getLocalBounds().withSizeKeepingCentre(juce::jmin(470, getWidth()-24), juce::jmin(470, getHeight()-24)));
    }
    void paint(juce::Graphics& g) override {
        using namespace SonoCraftTheme;
        g.fillAll(canvas());
#if JUCE_IOS || JUCE_ANDROID
        g.setColour(text()); g.setFont(juce::Font(21.0f,juce::Font::bold));
        g.drawText("SonoCraft",56,12,160,30,juce::Justification::centredLeft);
        if(!toolBounds.isEmpty()){g.setColour(panel());g.fillRoundedRectangle(toolBounds,12);}
        return;
#endif
        g.setColour(text()); g.setFont(juce::Font(19.0f, juce::Font::bold));
        g.drawText("SonoCraft", 62, 13, 135, 26, juce::Justification::centredLeft);
        g.setColour(secondary()); g.setFont(11.5f);
        g.drawText(juce::String::fromUTF8("MESA DE ÁUDIO"), 63, 39, 135, 16, juce::Justification::centredLeft);
        if (!toolBounds.isEmpty()) {
            g.setColour(panel()); g.fillRoundedRectangle(toolBounds, radius);
            g.setColour(border()); g.drawRoundedRectangle(toolBounds.reduced(.5f), radius, 1);
        }
        if (!desktopPanelBounds.isEmpty()) {
            g.setColour(border()); g.drawRoundedRectangle(desktopPanelBounds.reduced(.5f), radius, 1);
        }
        if (strips.isEmpty() && sourceStrips.isEmpty() && input.getParentComponent() != &channels) {
            auto empty = viewport.getBounds().reduced(12);
            g.setColour(panel().withAlpha(.7f)); g.fillRoundedRectangle(empty.toFloat(), radius);
            g.setColour(text()); g.setFont(juce::Font(14.0f, juce::Font::bold));
            g.drawText(juce::String::fromUTF8("Suas fontes aparecem aqui"), empty.withHeight(28).withY(empty.getCentreY()-32), juce::Justification::centred);
            g.setColour(secondary()); g.setFont(12.0f);
            g.drawText(juce::String::fromUTF8("Abra Fontes para adicionar uma aba."), empty.withHeight(24).withY(empty.getCentreY()), juce::Justification::centred);
        }
    }
private:
    enum class Tool { none, player, recorder, metronome };
    SonobusAudioProcessor& processor;
    SonoCraftChannelStrip input, master;
    juce::Viewport viewport;
    juce::Component channels;
    juce::OwnedArray<SonoCraftChannelStrip> strips;
    juce::OwnedArray<SonoCraftSourceStrip> sourceStrips;
    std::unique_ptr<SonoCraftSourceControls> sourceDialog;
    juce::ImageComponent brand;
    juce::Label session, sourceRouting, inspector, toolTitle, toolHelp, tempoLabel, levelLabel;
    juce::TextButton playerTab, recorderTab, metronomeTab, moreTools;
    juce::Component toolContent;
    juce::Viewport toolViewport;
    juce::Component::SafePointer<juce::Component> desktopPanel;
    juce::Rectangle<float> desktopPanelBounds;
    SonoCraftNavigationButton connections, sources, soundboard, chat, tools, settings;
    SonoDrawableButton closeTool { "Close", juce::DrawableButton::ImageFitted };
    juce::TextButton file, play, stop, record, playingActivity, recordingActivity, metronomeActivity;
    Component::SafePointer<Component> sourcePopup;
    juce::ToggleButton metEnabled, metSend;
    juce::Slider metTempo, metLevel, bufferTime;
    juce::ComboBox bufferMode;
    juce::String selectedId;
    juce::Rectangle<float> toolBounds;
    Tool tool = Tool::none, lastTool = Tool::player;
    bool syncing = false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> metTempoAttachment, metLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> metEnabledAttachment, metSendAttachment;
    void timerCallback() override {
        if (!isShowing()) return;
#if JUCE_IOS || JUCE_ANDROID
        // Covered meters must not compete with typing, menus or modal sheets.
        if ((isChatVisible && isChatVisible()) || (isPadsVisible && isPadsVisible())
            || (isSettingsVisible && isSettingsVisible()) || (isConnectionVisible && isConnectionVisible())
            || (isAdvancedToolVisible && isAdvancedToolVisible()) || (sourceDialog && sourceDialog->isVisible())
            || juce::Component::getCurrentlyModalComponent() != nullptr
            || dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()) != nullptr) return;
#endif
        refresh();
    }
    void selectTool(Tool next) {
#if !(JUCE_IOS || JUCE_ANDROID)
        if (next != Tool::none && desktopPanel) {
            desktopPanel->setVisible(false);
            desktopPanel = nullptr;
        }
#endif
        tool = next;
        if (next != Tool::none) lastTool = next;
        if (tool == Tool::player) toolTitle.setText(juce::String::fromUTF8("Arquivo de áudio"), juce::dontSendNotification);
        else if (tool == Tool::recorder) toolTitle.setText(juce::String::fromUTF8("Gravação"), juce::dontSendNotification);
        else if (tool == Tool::metronome) {
            toolTitle.setText(juce::String::fromUTF8("Metrônomo"), juce::dontSendNotification);
            toolHelp.setText(juce::String::fromUTF8("Ajuste o tempo em BPM e o volume em dB. Escolha se os conectados também devem ouvir."), juce::dontSendNotification);
        }
        refresh(); resized(); repaint();
    }
    void showConnections() {
        juce::PopupMenu menu;
        menu.addItem(1, "Entrar em grupo"); menu.addItem(2, "Conectar por IP"); menu.addSeparator();
        menu.addItem(3, "Desconectar", processor.getNumberRemotePeers() > 0 || processor.isConnectedToServer());
        juce::Component::SafePointer<SonoCraftMixerView> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&connections).withParentComponent(this), [safe](int choice) {
            if (!safe) return;
            if (choice == 1 && safe->onConnectGroup) safe->onConnectGroup();
            if (choice == 2 && safe->onConnectIP) safe->onConnectIP();
            if (choice == 3) safe->processor.disconnectFromServer();
        });
    }
    void showAdvancedTools() {
        juce::PopupMenu menu;
        menu.addItem(1, juce::String::fromUTF8("Efeitos da saída"));
        menu.addItem(2, "Roteamento de canais");
        juce::Component::SafePointer<SonoCraftMixerView> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&moreTools).withParentComponent(this), [safe](int choice) {
            if (!safe) return;
            if (choice == 1 && safe->onEffects) safe->onEffects();
            if (choice == 2 && safe->onRouting) safe->onRouting();
        });
    }
    void showTools() {
#if !(JUCE_IOS || JUCE_ANDROID)
        selectTool(tool == Tool::none ? lastTool : Tool::none);
        return;
#endif
        juce::PopupMenu menu;
        menu.addItem(1, juce::String::fromUTF8("Arquivo de áudio"), true, tool == Tool::player);
        menu.addItem(2, juce::String::fromUTF8("Gravação"), true, tool == Tool::recorder);
        menu.addItem(3, juce::String::fromUTF8("Metrônomo"), true, tool == Tool::metronome);
        if (processor.isRecordingToFile()) menu.addItem(6, juce::String::fromUTF8("Parar gravação agora"));
        menu.addSeparator();
#if JUCE_IOS || JUCE_ANDROID
        menu.addItem(7, "Fontes no celular");
#endif
        menu.addItem(4, juce::String::fromUTF8("Efeitos da saída")); menu.addItem(5, "Roteamento de canais");
        juce::Component::SafePointer<SonoCraftMixerView> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&tools).withParentComponent(this), [safe](int choice) {
            if (!safe) return;
            if (choice == 1) safe->selectTool(Tool::player);
            if (choice == 2) safe->selectTool(Tool::recorder);
            if (choice == 3) safe->selectTool(Tool::metronome);
            if (choice == 4 && safe->onEffects) safe->onEffects();
            if (choice == 5 && safe->onRouting) safe->onRouting();
            if (choice == 6 && safe->onRecord) safe->onRecord();
            if (choice == 7) safe->sources.triggerClick();
        });
    }
    void applyBuffer() {
        if (!syncing && selectedId.isNotEmpty()) processor.setMixerChannelBuffer(selectedId, float(bufferTime.getValue()), bufferMode.getSelectedId() == 1);
    }
    void refresh()
    {
#if !(JUCE_IOS || JUCE_ANDROID)
        processor.getSourceMixer().pollDiscovery();
#endif
        const auto sourcesState = processor.getSourceMixer().views();
        const auto states = processor.getMixerChannelStates();
        bool hasRecipients = false, anyPeerSolo = false;
        for (const auto& state : states) { hasRecipients |= state.sendReady; anyPeerSolo |= state.soloed; }
        const auto& parameters = processor.getValueTreeState();
        const bool sendPaused = parameters.getRawParameterValue(SonobusAudioProcessor::paramMainSendMute)->load() > 0;
        const bool inputSolo = parameters.getRawParameterValue(SonobusAudioProcessor::paramMainMonitorSolo)->load() > 0;
        bool sourcesChanged = sourcesState.size() != sourceStrips.size();
        if (!sourcesChanged) for (int i=0;i<sourcesState.size();++i) sourcesChanged |= sourcesState[i].id != sourceStrips[i]->sourceId();
        if (sourcesChanged) {
            sourceStrips.clear();
            for (const auto& state:sourcesState) channels.addAndMakeVisible(sourceStrips.add(new SonoCraftSourceStrip(processor,state.id)));
        }
        for (int i=0;i<sourcesState.size();++i) sourceStrips[i]->sync(sourcesState[i], hasRecipients, sendPaused);
        const bool virtualInput = SonoCraftAudioRouting::isVirtualCable(processor.getStandaloneInputName());
        const bool individual = processor.getSourceMixer().usesIndividualSources();
        sourceRouting.setText(juce::String::fromUTF8(virtualInput
            ? (individual ? "Fontes individuais: a entrada completa do sistema fica fora do envio."
                          : "Entrada completa: o sistema inteiro pode ser enviado. Fontes individuais desligadas.")
            : "Aplicativos com áudio aparecem automaticamente. Ative Enviar nos canais que deseja compartilhar."),juce::dontSendNotification);
        const auto discoveryStatus=processor.getSourceMixer().discoveryStatus();
        sourceRouting.setTooltip(discoveryStatus.isEmpty()?juce::String::fromUTF8("A medição automática não grava nem envia áudio. Abas são escolhidas em Fontes."):discoveryStatus);
        if(discoveryStatus.isNotEmpty())sourceRouting.setText(discoveryStatus,juce::dontSendNotification);
#if JUCE_IOS || JUCE_ANDROID
        sourceRouting.setText(juce::String::fromUTF8("Entrada, pads e participantes: ajuste cada volume na mesa."),juce::dontSendNotification);
        sourceRouting.setTooltip(juce::String::fromUTF8("Aplicativos e abas compartilhados por outro dispositivo são recebidos no canal desse participante."));
#endif
        if (sourcesChanged) resized();
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
            strips[i]->setState(states[i], (anyPeerSolo || inputSolo) && !states[i].soloed); strips[i]->setSelected(states[i].id == selectedId);
            if (states[i].id == selectedId) {
                inspector.setText(states[i].name + juce::String::fromUTF8("   ·   ") + states[i].address + juce::String::fromUTF8("   ·   Envio: ") + states[i].codec + juce::String::fromUTF8("   ·   Buffer de recepção"), juce::dontSendNotification);
                bufferMode.setSelectedId(states[i].bufferMode == 0 ? 2 : 1, juce::dontSendNotification);
                if (!bufferTime.isMouseButtonDown()) bufferTime.setValue(states[i].bufferMs, juce::dontSendNotification);
                bufferTime.setEnabled(states[i].bufferMode == 0);
            }
        }
        syncing = false;
        input.syncLocal(hasRecipients, sendPaused); master.syncLocal(hasRecipients, sendPaused);
        const auto group = processor.getCurrentJoinedGroup();
        session.setText((group.isNotEmpty() ? "Grupo: " + group : juce::String::fromUTF8("Sessão local")) + juce::String::fromUTF8("   ·   ") + juce::String(states.size()) + (states.size() == 1 ? " participante" : " participantes"), juce::dontSendNotification);
        session.setTooltip(session.getText());
        const bool recording = processor.isRecordingToFile();
        const bool playing = processor.getTransportSource().isPlaying();
        const bool metronome = parameters.getRawParameterValue(SonobusAudioProcessor::paramMetEnabled)->load() > 0;
        const bool activityLayoutChanged = playingActivity.isVisible() != playing || recordingActivity.isVisible() != recording || metronomeActivity.isVisible() != metronome;
        playingActivity.setVisible(playing); recordingActivity.setVisible(recording); metronomeActivity.setVisible(metronome);
        playingActivity.setButtonText("Arquivo tocando");
        recordingActivity.setButtonText(juce::String::fromUTF8("Gravação em andamento"));
        metronomeActivity.setButtonText(juce::String::fromUTF8("Metrônomo ligado · ") + juce::String(juce::roundToInt(parameters.getRawParameterValue(SonobusAudioProcessor::paramMetTempo)->load())) + " BPM");
        if (activityLayoutChanged) resized();
        const bool hasFile = processor.getTransportSource().getLengthInSeconds() > 0;
        play.setEnabled(hasFile); stop.setEnabled(hasFile);
        play.setButtonText(processor.getTransportSource().isPlaying() ? "Pausar" : "Reproduzir");
        record.setButtonText(recording ? juce::String::fromUTF8("Parar gravação") : juce::String::fromUTF8("Iniciar gravação"));
        record.setToggleState(recording, juce::dontSendNotification);
        tools.setColour(juce::TextButton::buttonOnColourId, recording ? juce::Colour(0xffffe6cf) : SonoCraftTheme::selection());
        tools.setToggleState(tool != Tool::none || recording || playing || metronome || (isAdvancedToolVisible && isAdvancedToolVisible()), juce::dontSendNotification);
        connections.setToggleState(isConnectionVisible && isConnectionVisible(), juce::dontSendNotification);
        settings.setToggleState(isSettingsVisible && isSettingsVisible(), juce::dontSendNotification);
        chat.setToggleState(isChatVisible && isChatVisible(), juce::dontSendNotification);
        soundboard.setToggleState(isPadsVisible && isPadsVisible(), juce::dontSendNotification);
        sources.setToggleState(sourceDialog && sourceDialog->isVisible(), juce::dontSendNotification);
        if (tool == Tool::player) {
            const auto url = processor.getCurrentLoadedTransportURL();
            toolHelp.setText(hasFile ? url.getFileName() : juce::String::fromUTF8("Abra um arquivo de áudio para ouvir e transmitir."), juce::dontSendNotification);
        } else if (tool == Tool::recorder) {
            toolHelp.setText(juce::String::fromUTF8(recording ? "Gravando áudio. Fechar este painel não interrompe a gravação." : "Salve o áudio em um arquivo. Destino e formato ficam em Ajustes → Gravação."), juce::dontSendNotification);
        }
        toolHelp.setTooltip(toolHelp.getText());
    }
};
