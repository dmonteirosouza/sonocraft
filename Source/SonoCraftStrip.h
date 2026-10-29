// Shared channel surface: routing stays in the channel's adapter.
#pragma once
#include "SonoCraftFaderLookAndFeel.h"
#include "SonoCraftChannelPresentation.h"

class SonoCraftStrip : public juce::Component, public juce::SettableTooltipClient {
public:
    static constexpr int preferredWidth = 184;
    SonoCraftStrip() {
        for (auto* c : std::initializer_list<juce::Component*> {
                 &name, &subtitle, &status, &listeningRoute, &sendingRoute, &fader, &mute, &secondary, &details, &remove })
            addAndMakeVisible(c);
        name.setFont(juce::Font(15.0f, juce::Font::bold));
        name.setColour(juce::Label::textColourId, SonoCraftTheme::text());
        subtitle.setFont(11.0f);
        subtitle.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        status.setFont(juce::Font(11.5f));
        status.setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
        status.setJustificationType(juce::Justification::centredLeft);
        status.setBorderSize(juce::BorderSize<int>(0, 14, 0, 0));
        status.setMinimumHorizontalScale(1);
        for (auto* route : { &listeningRoute, &sendingRoute }) {
            route->setFont(11.5f); route->setMinimumHorizontalScale(1.0f);
            route->setColour(juce::Label::textColourId, SonoCraftTheme::secondary());
            route->setBorderSize(juce::BorderSize<int>(0));
        }
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
        const bool compact = getHeight() < 430;
        const int row = compact ? 24 : 28;
        name.setBounds(r.removeFromTop(25));
        subtitle.setBounds(r.removeFromTop(18));
        status.setBounds(r.removeFromTop(24));
        r.removeFromTop(4);
        listeningRoute.setBounds(r.removeFromTop(17));
        sendingRoute.setBounds(r.removeFromTop(17));
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
        g.setGradientFill(juce::ColourGradient(panel(), 0, 0, juce::Colour(0xffe8edf1), 0, float(getHeight()), false));
        g.fillRoundedRectangle(box, radius);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillRoundedRectangle(8, 8, float(getWidth()-16), 47, 4);
        g.setColour(border());
        g.drawHorizontalLine(details.getY()-5, 12, float(getWidth()-12));
        g.setColour(selected ? accent() : border());
        g.drawRoundedRectangle(box, radius, selected ? 1.5f : 1.0f);
        g.setColour(statusColour);
        g.fillEllipse(float(status.getX()+2), float(status.getBounds().getCentreY()-3), 6.0f, 6.0f);
        g.setColour(accent());
        g.fillRoundedRectangle(12, 0, float(getWidth() - 24), 3, 1.5f);
        g.setColour(SonoCraftTheme::secondary()); g.setFont(10.0f);
        for (const double db : {0.0, -12.0, -24.0, -36.0, -48.0, -60.0}) {
            const auto y = scale.getBottom() - float(fader.valueToProportionOfLength(db)) * scale.getHeight();
            g.drawText(db == -60 ? juce::String::fromUTF8("−∞") : juce::String(int(db)),
                       int(scale.getX()), int(y - 6), int(scale.getWidth()), 12, juce::Justification::centredRight);
        }
        // Fine calibration marks share the fader's actual dB mapping.
        for (int db = -60; db <= 0; db += 3) {
            const float y = scale.getBottom() - float(fader.valueToProportionOfLength(db)) * scale.getHeight();
            const float edge = float(fader.getX()+fader.getWidth()/2);
            g.setColour(db == 0 ? accent() : border().darker(0.2f));
            g.drawHorizontalLine(juce::roundToInt(y), edge-24, edge- (db % 12 == 0 ? 12 : 17));
            g.drawHorizontalLine(juce::roundToInt(y), edge+ (db % 12 == 0 ? 12 : 17), edge+24);
        }
        for (int ch = 0; ch < 2; ++ch) {
            auto bar = meter.withX(meter.getX() + ch * 10).withWidth(7);
            g.setColour(juce::Colour(0xff23333f)); g.fillRoundedRectangle(bar.expanded(1), 2);
            const float peak = ch == 0 ? meterLeft : meterRight;
            const float level = juce::jlimit(0.0f, 1.0f, (juce::Decibels::gainToDecibels(peak, -60.0f) + 60) / 60);
            const int segments = juce::jmax(8, int(bar.getHeight()/6));
            const float step = bar.getHeight()/segments;
            for (int segment = 0; segment < segments; ++segment) {
                const float threshold = float(segment+1)/segments;
                const auto colour = threshold > 0.95f ? juce::Colour(0xffdd5a57)
                    : threshold > 0.8f ? juce::Colour(0xffe9b44b) : juce::Colour(0xff43b790);
                g.setColour(threshold <= level ? colour : colour.withAlpha(0.16f));
                g.fillRect(bar.getX(), bar.getBottom()-(segment+1)*step, bar.getWidth(), juce::jmax(1.0f, step-2));
            }
        }
    }
protected:
    juce::Label name, subtitle, status, listeningRoute, sendingRoute;
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
    void setStatus(const juce::String& text, const juce::String& explanation = {}, SonoCraftPresentation::Tone tone = SonoCraftPresentation::Tone::neutral) {
        statusColour = tone == SonoCraftPresentation::Tone::active ? SonoCraftTheme::accent()
            : tone == SonoCraftPresentation::Tone::attention ? SonoCraftTheme::warning() : SonoCraftTheme::secondary();
        status.setColour(juce::Label::textColourId, statusColour);
        status.setText(text, juce::dontSendNotification);
        status.setTooltip(explanation.isNotEmpty() ? explanation : text);
    }
    void setRoutes(const juce::String& listening, const juce::String& sending, const juce::String& explanation = {}) {
        listeningRoute.setText(listening, juce::dontSendNotification);
        sendingRoute.setText(sending, juce::dontSendNotification);
        listeningRoute.setTooltip(explanation.isNotEmpty() ? explanation : listening);
        sendingRoute.setTooltip(explanation.isNotEmpty() ? explanation : sending);
    }
private:
    juce::Colour statusColour = SonoCraftTheme::secondary();
    SonoCraftFaderLookAndFeel look;
    juce::Rectangle<float> meter, scale;
};
