// SonoCraft's desktop mixing workspace. All meters are GUI snapshots;
// audio processing remains owned by SonobusAudioProcessor.
#pragma once
#include "SonobusPluginProcessor.h"
#include "SonoCraftTheme.h"

class SonoCraftFaderLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float position, float, float, juce::Slider::SliderStyle, juce::Slider&) override
    {
        const auto centre = x + width * 0.5f;
        g.setColour(SonoCraftTheme::border());
        g.fillRoundedRectangle(centre - 2, float(y), 4, float(height), 2);
        g.setColour(SonoCraftTheme::accent().withAlpha(0.45f));
        g.fillRoundedRectangle(centre - 2, position, 4, juce::jmax(0.0f, y + height - position), 2);
        juce::Rectangle<float> thumb(centre - 17, position - 8, 34, 16);
        g.setColour(juce::Colours::black.withAlpha(0.08f));
        g.fillRoundedRectangle(thumb.translated(0, 2), 4);
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(thumb, 4);
        g.setColour(juce::Colour(0xffa9b9c7));
        g.drawRoundedRectangle(thumb, 4, 1);
        g.setColour(SonoCraftTheme::secondary());
        g.drawHorizontalLine(juce::roundToInt(position), centre - 10, centre + 10);
    }
};

class SonoCraftChannelStrip final : public juce::Component {
public:
    enum Role { input, peer, master };
    SonoCraftChannelStrip(SonobusAudioProcessor& p, Role r, juce::String connection = {})
        : processor(p), role(r), id(std::move(connection))
    {
        name.setFont(juce::Font(15.0f, juce::Font::bold));
        name.setColour(juce::Label::textColourId, SonoCraftTheme::text());
        subtitle.setFont(juce::Font(11.5f));
        subtitle.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        value.setJustificationType(juce::Justification::centred);
        value.setFont(juce::Font(13.0f));
        for (auto* component : std::initializer_list<juce::Component*> { &name, &subtitle, &value,
                                 &fader, &mute, &solo, &details, &remove })
            addAndMakeVisible(component);
        fader.setLookAndFeel(&faderLookAndFeel);
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        fader.setRange(-60.0, role == input ? 12.0 : 6.0, 0.1);
        fader.setDoubleClickReturnValue(true, 0.0);
        fader.setScrollWheelEnabled(false);
        fader.onValueChange = [this] {
            const auto db = float(fader.getValue());
            const auto gain = db <= -60 ? 0.0f : juce::Decibels::decibelsToGain(db);
            if (role == peer) processor.setMixerChannelGain(id, gain);
            else writeParameter(role == input ? SonobusAudioProcessor::paramInGain : SonobusAudioProcessor::paramWet, gain);
            value.setText(db <= -60 ? juce::String::fromUTF8("−∞ dB") : juce::String(db, 1) + " dB", juce::dontSendNotification);
        };
        fader.onDragStart = [this] { if (auto* p = gainParameter()) p->beginChangeGesture(); };
        fader.onDragEnd = [this] { if (auto* p = gainParameter()) p->endChangeGesture(); };
        mute.setButtonText(role == master ? juce::String::fromUTF8("Recepção") : "Silenciar");
        solo.setButtonText(role == input ? "Monitorar" : "Solo");
        mute.setClickingTogglesState(true);
        solo.setClickingTogglesState(true);
        mute.setColour(juce::TextButton::buttonOnColourId, role == master ? SonoCraftTheme::selection() : juce::Colour(0xffffe6cf));
        solo.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffefb7));
        mute.setTooltip(role == input ? juce::String::fromUTF8("Silenciar somente meu microfone, mantendo os pads") : role == master ? juce::String::fromUTF8("Ativar ou suspender a recepção de todos os participantes") : juce::String::fromUTF8("Silenciar o áudio recebido deste participante"));
        solo.setTooltip(role == input ? "Monitorar a entrada local" : "Ouvir somente este participante e outros canais em solo");
        mute.onClick = [this] {
            if (role == peer) processor.setMixerChannelMute(id, mute.getToggleState());
            else writeParameter(role == input ? SonobusAudioProcessor::paramMainInMute : SonobusAudioProcessor::paramMainRecvMute, (role == master ? !mute.getToggleState() : mute.getToggleState()) ? 1.0f : 0.0f);
        };
        solo.onClick = [this] {
            if (role == peer) processor.setMixerChannelSolo(id, solo.getToggleState());
            else if (role == input) writeParameter(SonobusAudioProcessor::paramDry, solo.getToggleState() ? 1.0f : 0.0f);
        };
        details.setButtonText(role == peer ? "Detalhes" : "Dispositivo");
        details.onClick = [this] { if (onDetails) onDetails(id); };
        remove.setButtonText("Remover");
        remove.setTooltip(juce::String::fromUTF8("Remover somente esta conexão da sua sessão"));
        remove.setColour(juce::TextButton::textColourOffId, SonoCraftTheme::danger());
        remove.onClick = [this] { processor.removeRemotePeerByConnectionId(id); };
        remove.setVisible(role == peer);
        solo.setVisible(role != master);
        fader.setTitle(role == input ? "Ganho de entrada" : role == master ? "Volume master" : "Volume do participante");
    }
    ~SonoCraftChannelStrip() override { fader.setLookAndFeel(nullptr); }

    void setState(const SonobusAudioProcessor::MixerChannelState& state)
    {
        name.setText(state.name, juce::dontSendNotification);
        subtitle.setText(state.address, juce::dontSendNotification);
        name.setTooltip(state.name + juce::String::fromUTF8(" · ") + state.address);
        meterLeft = state.left; meterRight = state.right; connected = state.connected;
        mute.setToggleState(state.muted, juce::dontSendNotification);
        solo.setToggleState(state.soloed, juce::dontSendNotification);
        syncGain(state.gain);
        repaint();
    }
    void syncLocal()
    {
        const bool meetingMode = readParameter(SonobusAudioProcessor::paramMeetingMode) > 0;
        solo.setEnabled(!meetingMode);
        name.setText(role == input ? "Sua entrada" : meetingMode ? juce::String::fromUTF8("Saída reunião") : "Master", juce::dontSendNotification);
        subtitle.setText(role == input ? "ENVIO" : juce::String::fromUTF8("SAÍDA"), juce::dontSendNotification);
        auto& source = role == input ? processor.getInputMeterSource() : processor.getOutputMeterSource();
        source.decayIfNeeded();
        meterLeft = source.getNumChannels() > 0 ? source.getMaxLevel(0) : 0;
        meterRight = source.getNumChannels() > 1 ? source.getMaxLevel(1) : meterLeft;
        syncGain(readParameter(role == input ? SonobusAudioProcessor::paramInGain : SonobusAudioProcessor::paramWet));
        const auto muted = readParameter(role == input ? SonobusAudioProcessor::paramMainInMute : SonobusAudioProcessor::paramMainRecvMute) > 0;
        mute.setToggleState(role == master ? !muted : muted, juce::dontSendNotification);
        if (role == input) solo.setToggleState(readParameter(SonobusAudioProcessor::paramDry) > 0, juce::dontSendNotification);
        repaint();
    }
    void setSelected(bool yes) { selected = yes; repaint(); }
    const juce::String& connectionId() const { return id; }
    std::function<void(const juce::String&)> onDetails;

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(12);
        name.setBounds(bounds.removeFromTop(25));
        subtitle.setBounds(bounds.removeFromTop(22));
        bounds.removeFromTop(12);
        auto switches = bounds.removeFromTop(28);
        mute.setBounds(switches);
        bounds.removeFromTop(6);
        auto soloRow = bounds.removeFromTop(28);
        if (role != master) solo.setBounds(soloRow);
        bounds.removeFromTop(8);
        auto removeRow = bounds.removeFromBottom(27);
        if (role == peer) remove.setBounds(removeRow);
        bounds.removeFromBottom(6);
        details.setBounds(bounds.removeFromBottom(29));
        bounds.removeFromBottom(8);
        value.setBounds(bounds.removeFromBottom(22));
        bounds.removeFromBottom(4);
        meterBounds = bounds.removeFromRight(23).toFloat().reduced(2, 8);
        bounds.removeFromRight(18);
        fader.setBounds(bounds.withTrimmedLeft(18));
        scaleBounds = bounds.withWidth(24).toFloat();
    }
    void paint(juce::Graphics& g) override
    {
        using namespace SonoCraftTheme;
        auto box = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(role == master ? juce::Colour(0xffeaf2f4) : panel());
        g.fillRoundedRectangle(box, radius);
        g.setColour(selected ? accent() : border());
        g.drawRoundedRectangle(box, radius, selected ? 1.5f : 1.0f);
        g.setColour(role == peer ? (connected ? accent() : warning()) : accent());
        g.fillRoundedRectangle(12, 0, float(getWidth() - 24), 3, 1.5f);
        g.setFont(10.0f);
        g.setColour(secondary());
        for (const auto db : { 0, -12, -24, -36, -48, -60 }) {
            const auto proportion = (fader.getMaximum() - db) / (fader.getMaximum() - fader.getMinimum());
            const auto y = fader.getY() + 8 + proportion * juce::jmax(1, fader.getHeight() - 16);
            g.drawText(db == -60 ? juce::String::fromUTF8("−∞") : juce::String(db), int(scaleBounds.getX()), int(y - 6), 26, 12, juce::Justification::centredRight);
        }
        const auto left = juce::jlimit(0.0f, 1.0f, (juce::Decibels::gainToDecibels(meterLeft, -60.0f) + 60) / 60);
        const auto right = juce::jlimit(0.0f, 1.0f, (juce::Decibels::gainToDecibels(meterRight, -60.0f) + 60) / 60);
        for (int ch = 0; ch < 2; ++ch) {
            auto bar = meterBounds.withX(meterBounds.getX() + ch * 10).withWidth(7);
            g.setColour(recessed()); g.fillRoundedRectangle(bar, 2);
            const auto level = ch == 0 ? left : right;
            g.setColour(level > 0.98f ? danger() : level > 0.9f ? juce::Colour(0xffca962b) : accent());
            g.fillRoundedRectangle(bar.removeFromBottom(bar.getHeight() * level), 2);
        }
    }
private:
    SonobusAudioProcessor& processor;
    Role role;
    juce::String id;
    juce::Label name, subtitle, value;
    juce::Slider fader;
    juce::TextButton mute, solo, details, remove;
    SonoCraftFaderLookAndFeel faderLookAndFeel;
    juce::Rectangle<float> meterBounds, scaleBounds;
    float meterLeft = 0, meterRight = 0;
    bool connected = false, selected = false;
    float readParameter(const juce::String& key) const { return processor.getValueTreeState().getRawParameterValue(key)->load(); }
    void writeParameter(const juce::String& key, float value) {
        auto* parameter = processor.getValueTreeState().getParameter(key);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
    juce::RangedAudioParameter* gainParameter() {
        return role == peer ? nullptr : processor.getValueTreeState().getParameter(role == input ? SonobusAudioProcessor::paramInGain : SonobusAudioProcessor::paramWet);
    }
    void syncGain(float gain) {
        if (!fader.isMouseButtonDown()) fader.setValue(juce::Decibels::gainToDecibels(gain, -60.0f), juce::dontSendNotification);
        value.setText(gain <= 0 ? juce::String::fromUTF8("−∞ dB") : juce::String(juce::Decibels::gainToDecibels(gain), 1) + " dB", juce::dontSendNotification);
    }
};

class SonoCraftMixerView final : public juce::Component, private juce::Timer {
public:
    explicit SonoCraftMixerView(SonobusAudioProcessor& p) : processor(p), input(p, SonoCraftChannelStrip::input), master(p, SonoCraftChannelStrip::master)
    {
        for (auto* c : std::initializer_list<juce::Component*> { &input, &master, &viewport, &session,
                         &brand, &connectIP, &connectGroup, &audio, &settings, &record, &metronome, &metEnabled, &metTempo, &metLevel, &metSend,
                         &file, &routing, &effects, &chat, &soundboard, &play, &stop, &meeting, &meetingHelp, &disconnect, &inspector, &bufferMode, &bufferTime })
            addAndMakeVisible(c);
        brand.setImage(juce::ImageCache::getFromMemory(BinaryData::sonocraft_icon_256_png, BinaryData::sonocraft_icon_256_pngSize));
        viewport.setViewedComponent(&channels, false);
        viewport.setScrollBarsShown(false, true);
        session.setFont(juce::Font(13.0f));
        session.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        connectIP.setButtonText("Conectar por IP"); connectGroup.setButtonText("Entrar em grupo");
        connectIP.setColour(juce::TextButton::buttonColourId, SonoCraftTheme::selection());
        audio.setButtonText(juce::String::fromUTF8("Áudio e reunião")); settings.setButtonText("Ajustes");
        meeting.setButtonText(juce::String::fromUTF8("Modo reunião"));
        meeting.setClickingTogglesState(true);
        meeting.setColour(juce::TextButton::buttonOnColourId, SonoCraftTheme::selection());
        meetingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getValueTreeState(), SonobusAudioProcessor::paramMeetingMode, meeting);
        meeting.onClick = [this] { resized(); };
        meetingHelp.setText(juce::String::fromUTF8("Reunião: seu microfone + áudio recebido + pads locais. Em Dispositivos, use seu microfone como entrada e um cabo virtual como saída. Na reunião, selecione o cabo virtual como microfone e seus fones como saída."), juce::dontSendNotification);
        meetingHelp.setColour(juce::Label::textColourId, SonoCraftTheme::accent());
        meetingHelp.setFont(juce::Font(12.5f));
        meetingHelp.setMinimumHorizontalScale(1.0f);
        meetingHelp.setJustificationType(juce::Justification::centredLeft);
        play.setButtonText("Reproduzir"); stop.setButtonText("Parar");
        play.onClick = [this] { if (onPlay) onPlay(); };
        stop.onClick = [this] { processor.getTransportSource().stop(); processor.getTransportSource().setPosition(0.0); };
        record.setButtonText("Gravar"); metronome.setButtonText(juce::String::fromUTF8("Metrônomo")); file.setButtonText("Arquivo");
        routing.setButtonText("Roteamento"); effects.setButtonText("Efeitos");
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
        refresh(); startTimerHz(24);
    }
    ~SonoCraftMixerView() override { stopTimer(); viewport.setViewedComponent(nullptr, false); }
    std::function<void()> onConnectIP, onConnectGroup, onAudio, onSettings, onRecord, onMetronome, onFile, onRouting, onEffects, onChat, onSoundboard, onPlay;
    juce::Component* settingsAnchor() { return &settings; }
    juce::Component* connectionAnchor() { return &connectIP; }

    void resized() override
    {
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
        bounds.removeFromTop(10);
        const bool wrapTransport = bounds.getWidth() < 1020;
        auto transportArea = bounds.removeFromBottom(wrapTransport ? 82 : 44);
        auto transport = wrapTransport ? transportArea.removeFromTop(40) : transportArea;
        for (auto* button : { &play, &stop, &record, &metronome, &file, &routing, &effects, &chat, &soundboard, &meeting }) {
            const auto width = button == &meeting ? 124 : button == &soundboard ? 104 : button == &metronome || button == &routing ? 105 : 78;
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
        const bool meetingActive = meeting.getToggleState();
        meetingHelp.setVisible(meetingActive);
        if (meetingActive) {
            meetingHelp.setBounds(bounds.removeFromBottom(72).reduced(8, 4));
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
        const auto busWidth = getWidth() < 700 ? 128 : 148;
        const int channelHeight = juce::jmax(100, bounds.getHeight() - viewport.getScrollBarThickness());
        input.setBounds(bounds.removeFromLeft(busWidth).withHeight(channelHeight)); bounds.removeFromLeft(14);
        master.setBounds(bounds.removeFromRight(busWidth).withHeight(channelHeight)); bounds.removeFromRight(14);
        viewport.setBounds(bounds);
        const auto width = juce::jmax(bounds.getWidth(), strips.size() * 166);
        channels.setSize(width, juce::jmax(100, bounds.getHeight() - viewport.getScrollBarThickness()));
        int x = 0;
        for (auto* strip : strips) { strip->setBounds(x, 0, 154, channels.getHeight()); x += 166; }
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
        if (strips.isEmpty()) {
            auto empty = viewport.getBounds().reduced(12);
            g.setColour(panel().withAlpha(0.7f)); g.fillRoundedRectangle(empty.toFloat(), 8);
            g.setColour(text()); g.setFont(juce::Font(empty.getWidth() < 250 ? 14.0f : 18.0f, juce::Font::bold));
            g.drawText(juce::String::fromUTF8("Sua sessão começa aqui"), empty.withHeight(30).withY(empty.getCentreY() - 40), juce::Justification::centred);
            g.setColour(secondary()); g.setFont(13.0f);
            g.drawText("Conecte por IP ou entre em um grupo.", empty.withHeight(24).withY(empty.getCentreY()), juce::Justification::centred);
            g.drawText(juce::String::fromUTF8("Cada participante terá seu próprio canal."), empty.withHeight(24).withY(empty.getCentreY() + 24), juce::Justification::centred);
        }
    }
private:
    SonobusAudioProcessor& processor;
    SonoCraftChannelStrip input, master;
    juce::Viewport viewport;
    juce::Component channels;
    juce::OwnedArray<SonoCraftChannelStrip> strips;
    juce::ImageComponent brand;
    juce::Label session, inspector, meetingHelp;
    juce::TextButton connectIP, connectGroup, audio, settings, record, metronome, file, routing, effects, chat, soundboard, play, stop, meeting, disconnect;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> meetingAttachment;
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
