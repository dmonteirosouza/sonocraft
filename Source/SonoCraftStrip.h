// Shared channel surface: routing stays in the channel's adapter.
#pragma once
#include "SonoCraftFaderLookAndFeel.h"

class SonoCraftStrip : public juce::Component {
public:
    static constexpr int preferredWidth = 184;
    SonoCraftStrip() {
        for (auto* c : std::initializer_list<juce::Component*> {
                 &name, &subtitle, &status, &fader, &mute, &secondary, &details, &remove })
            addAndMakeVisible(c);
        name.setFont(juce::Font(15.0f, juce::Font::bold));
        name.setColour(juce::Label::textColourId, SonoCraftTheme::text());
        subtitle.setFont(11.0f);
        subtitle.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        status.setFont(12.0f);
        status.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        status.setJustificationType(juce::Justification::topLeft);
        status.setMinimumHorizontalScale(1);
        fader.setLookAndFeel(&look);
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 24);
        fader.setDoubleClickReturnValue(true, 0.0);
        fader.setScrollWheelEnabled(false);
        fader.setTooltip(juce::String::fromUTF8("0 dB = volume original. Arraste ou digite o valor. Duplo clique para voltar a 0 dB."));
        fader.textFromValueFunction = [](double db) {
            if (db <= -60) return juce::String::fromUTF8("−∞ dB");
            return juce::String(std::abs(db) < 0.05 ? 0.0 : db, 1) + " dB";
        };
        fader.valueFromTextFunction = [this](const juce::String& text) {
            if (text.contains(juce::String::fromUTF8("∞"))) return -60.0;
            const auto number = text.trim().upToFirstOccurrenceOf("dB", false, true).trim().replaceCharacter(',', '.');
            // Incomplete edits must not unexpectedly reset a live channel.
            if (!number.containsAnyOf("0123456789") || number.containsAnyOf("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"))
                return fader.getValue();
            return number.getDoubleValue();
        };
        setGainLimit(2.0f);
        mute.setButtonText("Silenciar");
        mute.setClickingTogglesState(true);
        secondary.setClickingTogglesState(true);
        mute.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffe6cf));
        secondary.setColour(juce::TextButton::buttonOnColourId, SonoCraftTheme::selection());
        remove.setButtonText("Remover");
        remove.setColour(juce::TextButton::textColourOffId, SonoCraftTheme::danger());
    }
    ~SonoCraftStrip() override { fader.setLookAndFeel(nullptr); }
    void resized() override {
        auto r = getLocalBounds().reduced(12);
        const bool compact = getHeight() < 360;
        const int row = compact ? 24 : 28;
        name.setBounds(r.removeFromTop(25));
        subtitle.setBounds(r.removeFromTop(20));
        status.setBounds(r.removeFromTop(compact ? 32 : 44));
        r.removeFromTop(6);
        mute.setBounds(r.removeFromTop(row));
        r.removeFromTop(6);
        secondary.setBounds(r.removeFromTop(row));
        r.removeFromTop(10);
        // Reserve identical footer rows, even when a channel is not removable.
        remove.setBounds(r.removeFromBottom(row));
        r.removeFromBottom(6);
        details.setBounds(r.removeFromBottom(row));
        r.removeFromBottom(8);
        fader.setBounds(r.withTrimmedLeft(25).withTrimmedRight(24));
        const auto track = look.getSliderLayout(fader).sliderBounds.translated(fader.getX(), fader.getY());
        meter = juce::Rectangle<float>(float(r.getRight() - 18), float(track.getY()), 18.0f, float(track.getHeight()));
        scale = juce::Rectangle<float>(float(r.getX() - 2), float(track.getY()), 28.0f, float(track.getHeight()));
    }
    void paint(juce::Graphics& g) override {
        using namespace SonoCraftTheme;
        const auto box = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(panel()); g.fillRoundedRectangle(box, radius);
        g.setColour(selected ? accent() : border());
        g.drawRoundedRectangle(box, radius, selected ? 1.5f : 1.0f);
        g.setColour(accent());
        g.fillRoundedRectangle(12, 0, float(getWidth() - 24), 3, 1.5f);
        g.setColour(SonoCraftTheme::secondary()); g.setFont(10.0f);
        for (const double db : {0.0, -12.0, -24.0, -36.0, -48.0, -60.0}) {
            const auto y = scale.getBottom() - float(fader.valueToProportionOfLength(db)) * scale.getHeight();
            g.drawText(db == -60 ? juce::String::fromUTF8("−∞") : juce::String(int(db)),
                       int(scale.getX()), int(y - 6), int(scale.getWidth()), 12, juce::Justification::centredRight);
        }
        for (int ch = 0; ch < 2; ++ch) {
            auto bar = meter.withX(meter.getX() + ch * 10).withWidth(7);
            g.setColour(recessed()); g.fillRoundedRectangle(bar, 2);
            const float peak = ch == 0 ? meterLeft : meterRight;
            const float level = juce::jlimit(0.0f, 1.0f, (juce::Decibels::gainToDecibels(peak, -60.0f) + 60) / 60);
            g.setColour(peak >= 1 ? danger() : peak > 0.5f ? warning() : accent());
            g.fillRoundedRectangle(bar.removeFromBottom(bar.getHeight() * level), 2);
        }
    }
protected:
    juce::Label name, subtitle, status;
    juce::Slider fader;
    juce::TextButton mute, secondary, details, remove;
    float meterLeft = 0, meterRight = 0;
    bool selected = false;
    void setGainLimit(float maxGain) {
        const double maxDb = juce::Decibels::gainToDecibels(double(maxGain));
        // Unity sits at the same height, even for an input with extra headroom.
        fader.setNormalisableRange(juce::NormalisableRange<double>(-60.0, maxDb,
            [](double lo, double hi, double p) { return p <= 0.85 ? lo + (0.0-lo)*p/0.85 : hi*(p-0.85)/0.15; },
            [](double lo, double hi, double db) { return db <= 0 ? 0.85*(db-lo)/(0.0-lo) : 0.85+0.15*db/hi; },
            [](double lo, double hi, double db) { return juce::jlimit(lo, hi, std::round(db*10.0)/10.0); }));
    }
    static float gainForDb(double db) { return db <= -60 ? 0.0f : juce::Decibels::decibelsToGain(float(db)); }
    void syncGain(float gain) {
        if (!fader.isMouseButtonDown() && !(dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()) != nullptr && fader.isParentOf(juce::Component::getCurrentlyFocusedComponent())))
            fader.setValue(juce::Decibels::gainToDecibels(double(gain), -60.0), juce::dontSendNotification);
    }
    void setStatus(const juce::String& text) { status.setText(text, juce::dontSendNotification); status.setTooltip(text); }
private:
    SonoCraftFaderLookAndFeel look;
    juce::Rectangle<float> meter, scale;
};
