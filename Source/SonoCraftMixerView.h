// SonoCraft's desktop mixing workspace. All meters are GUI snapshots;
// audio processing remains owned by SonobusAudioProcessor.
#pragma once
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"
#include "SonoCraftSourceControls.h"
#include "SonoCraftStrip.h"

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
        meterLeft = state.left; meterRight = state.right;
        setTooltip(juce::String::fromUTF8("Nível recebido deste participante, incluindo entrada, pads e fontes separadas."));
        mute.setToggleState(state.muted, juce::dontSendNotification);
        secondary.setToggleState(state.soloed, juce::dontSendNotification);
        syncGain(state.gain); repaint();
    }
    void syncLocal(bool hasRecipients, bool sendPaused) {
        name.setText(role == input ? "Sua entrada" : juce::String::fromUTF8("Saída principal"), juce::dontSendNotification);
        subtitle.setText(role == input ? "ENTRADA" : juce::String::fromUTF8("SAÍDA"), juce::dontSendNotification);
        auto& source = role == input ? processor.getInputMeterSource() : processor.getOutputMeterSource();
        source.decayIfNeeded();
        meterLeft = source.getNumChannels() > 0 ? source.getMaxLevel(0) : 0;
        meterRight = source.getNumChannels() > 1 ? source.getMaxLevel(1) : meterLeft;
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
    explicit SonoCraftMixerView(SonobusAudioProcessor& p) : processor(p), input(p, SonoCraftChannelStrip::input), master(p, SonoCraftChannelStrip::master) {
        for (auto* c : std::initializer_list<juce::Component*> { &input, &master, &viewport, &session, &sourceRouting, &brand,
                &connections, &sources, &soundboard, &chat, &tools, &settings,
                &toolTitle, &toolHelp, &closeTool, &file, &play, &stop, &record, &playingActivity, &recordingActivity, &metronomeActivity,
                &metEnabled, &metTempo, &metLevel, &metSend, &inspector, &bufferMode, &bufferTime })
            addAndMakeVisible(c);
        brand.setImage(juce::ImageCache::getFromMemory(BinaryData::sonocraft_icon_256_png, BinaryData::sonocraft_icon_256_pngSize));
        viewport.setViewedComponent(&channels, false);
        viewport.setScrollBarsShown(true, true);
        session.setFont(13.0f); session.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        session.setJustificationType(juce::Justification::centredRight);
        connections.setButtonText(juce::String::fromUTF8("Conexões")); connections.opensMenu = true;
        sources.setButtonText("Fontes"); soundboard.setButtonText("Pads"); chat.setButtonText("Conversa");
        tools.setButtonText("Ferramentas"); tools.opensMenu = true; settings.setButtonText("Ajustes");
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
            sourceDialog->setVisible(true); sourceDialog->toFront(true); resized();
        };
        sourceRouting.setFont(12.0f); sourceRouting.setMinimumHorizontalScale(1);
        sourceRouting.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        toolTitle.setFont(juce::Font(15.0f, juce::Font::bold));
        toolHelp.setFont(12.0f); toolHelp.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        closeTool.setButtonText("Fechar painel");
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
        processor.getSourceMixer().startDiscovery();
        refresh(); startTimerHz(24);
    }
    ~SonoCraftMixerView() override { stopTimer(); viewport.setViewedComponent(nullptr, false); }
    std::function<void()> onConnectIP, onConnectGroup, onSettings, onRecord, onFile, onRouting, onEffects, onChat, onSoundboard, onPlay;
    std::function<bool()> isChatVisible, isPadsVisible, isSettingsVisible, isConnectionVisible, isAdvancedToolVisible;
    juce::Component* connectionAnchor() { return &connections; }
    juce::Component* toolsAnchor() { return &tools; }
    juce::Component* recordingAnchor() { return record.isVisible() ? static_cast<juce::Component*>(&record) : &tools; }

    void resized() override {
        auto bounds = getLocalBounds().reduced(18, 12);
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
        sourceRouting.setBounds(bounds.removeFromTop(34));
        bounds.removeFromTop(6);
        const bool hasTool = tool != Tool::none;
        for (auto* c : std::initializer_list<juce::Component*> { &toolTitle, &toolHelp, &closeTool }) c->setVisible(hasTool);
        for (auto* c : { &file, &play, &stop }) c->setVisible(tool == Tool::player);
        record.setVisible(tool == Tool::recorder);
        for (auto* c : std::initializer_list<juce::Component*> { &metEnabled, &metSend, &metTempo, &metLevel }) c->setVisible(tool == Tool::metronome);
        toolBounds = {};
        if (hasTool) {
            const bool wrapMet = tool == Tool::metronome && bounds.getWidth() < 760;
            auto area = bounds.removeFromTop(wrapMet ? 146 : 112);
            toolBounds = area.toFloat();
            auto content = area.reduced(12, 8);
            auto titleRow = content.removeFromTop(26);
            closeTool.setBounds(titleRow.removeFromRight(104)); toolTitle.setBounds(titleRow);
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
        const bool showInspector = selectedId.isNotEmpty();
        inspector.setVisible(showInspector); bufferMode.setVisible(showInspector); bufferTime.setVisible(showInspector);
        if (showInspector) {
            auto details = bounds.removeFromBottom(82).reduced(12, 6);
            inspector.setBounds(details.removeFromTop(30));
            bufferMode.setBounds(details.removeFromLeft(140).withHeight(28)); details.removeFromLeft(16);
            bufferTime.setBounds(details.removeFromLeft(juce::jmin(380, details.getWidth())).withHeight(28));
            bounds.removeFromBottom(10);
        }
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
        if (sourceDialog) sourceDialog->setBounds(getLocalBounds().withSizeKeepingCentre(juce::jmin(470, getWidth()-24), juce::jmin(470, getHeight()-24)));
    }
    void paint(juce::Graphics& g) override {
        using namespace SonoCraftTheme;
        g.fillAll(canvas());
        g.setColour(text()); g.setFont(juce::Font(19.0f, juce::Font::bold));
        g.drawText("SonoCraft", 62, 13, 135, 26, juce::Justification::centredLeft);
        g.setColour(secondary()); g.setFont(11.5f);
        g.drawText(juce::String::fromUTF8("MESA DE ÁUDIO"), 63, 39, 135, 16, juce::Justification::centredLeft);
        if (!toolBounds.isEmpty()) {
            g.setColour(panel()); g.fillRoundedRectangle(toolBounds, radius);
            g.setColour(border()); g.drawRoundedRectangle(toolBounds.reduced(.5f), radius, 1);
        }
        if (strips.isEmpty() && sourceStrips.isEmpty()) {
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
    juce::Label session, sourceRouting, inspector, toolTitle, toolHelp;
    SonoCraftNavigationButton connections, sources, soundboard, chat, tools, settings;
    juce::TextButton closeTool, file, play, stop, record, playingActivity, recordingActivity, metronomeActivity;
    juce::ToggleButton metEnabled, metSend;
    juce::Slider metTempo, metLevel, bufferTime;
    juce::ComboBox bufferMode;
    juce::String selectedId;
    juce::Rectangle<float> toolBounds;
    Tool tool = Tool::none;
    bool syncing = false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> metTempoAttachment, metLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> metEnabledAttachment, metSendAttachment;
    void timerCallback() override { refresh(); }
    void selectTool(Tool next) {
        tool = next;
        if (tool == Tool::player) toolTitle.setText(juce::String::fromUTF8("Arquivo de áudio"), juce::dontSendNotification);
        else if (tool == Tool::recorder) toolTitle.setText(juce::String::fromUTF8("Gravação"), juce::dontSendNotification);
        else if (tool == Tool::metronome) {
            toolTitle.setText(juce::String::fromUTF8("Metrônomo"), juce::dontSendNotification);
            toolHelp.setText(juce::String::fromUTF8("Ajuste o tempo em BPM e o volume em dB. Escolha se os conectados também devem ouvir."), juce::dontSendNotification);
        }
        refresh(); resized();
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
    void showTools() {
        juce::PopupMenu menu;
        menu.addItem(1, juce::String::fromUTF8("Arquivo de áudio"), true, tool == Tool::player);
        menu.addItem(2, juce::String::fromUTF8("Gravação"), true, tool == Tool::recorder);
        menu.addItem(3, juce::String::fromUTF8("Metrônomo"), true, tool == Tool::metronome);
        if (processor.isRecordingToFile()) menu.addItem(6, juce::String::fromUTF8("Parar gravação agora"));
        menu.addSeparator();
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
        });
    }
    void applyBuffer() {
        if (!syncing && selectedId.isNotEmpty()) processor.setMixerChannelBuffer(selectedId, float(bufferTime.getValue()), bufferMode.getSelectedId() == 1);
    }
    void refresh()
    {
        processor.getSourceMixer().pollDiscovery();
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
        sourceRouting.setTooltip(discoveryStatus.isEmpty()?juce::String::fromUTF8("A detecção automática não captura nem envia áudio. Abas são escolhidas em Fontes."):discoveryStatus);
        if(discoveryStatus.isNotEmpty())sourceRouting.setText(discoveryStatus,juce::dontSendNotification);
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
        repaint();
    }
};
