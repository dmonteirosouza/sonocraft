#pragma once
#include "SonoCraftTheme.h"
class SonoCraftFaderLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    SonoCraftFaderLookAndFeel() {
        setColourScheme(getLightColourScheme());
        setColour(juce::Slider::textBoxTextColourId,SonoCraftTheme::text());
        setColour(juce::Slider::textBoxBackgroundColourId,SonoCraftTheme::panel());
        setColour(juce::Slider::textBoxOutlineColourId,SonoCraftTheme::border());
        setColour(juce::Slider::textBoxHighlightColourId,SonoCraftTheme::selection());
        setColour(juce::Label::textColourId,SonoCraftTheme::text());
        setColour(juce::Label::textWhenEditingColourId,SonoCraftTheme::text());
        setColour(juce::TextEditor::textColourId,SonoCraftTheme::text());
        setColour(juce::TextEditor::highlightedTextColourId,SonoCraftTheme::text());
        setColour(juce::TextEditor::highlightColourId,SonoCraftTheme::selection());
    }
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

