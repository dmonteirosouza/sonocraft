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
        g.setColour(juce::Colour(0xffc6cfd7));
        g.fillRoundedRectangle(centre - 7, float(y)-2, 14, float(height)+4, 6);
        g.setColour(juce::Colour(0xff334452));
        g.fillRoundedRectangle(centre - 3, float(y), 6, float(height), 3);
        g.setColour(SonoCraftTheme::accent());
        g.fillRoundedRectangle(centre - 1, position, 2, juce::jmax(0.0f, y + height - position), 1);
        const float capWidth = juce::jmin(42.0f, float(width)-4);
        juce::Rectangle<float> thumb(centre - capWidth/2, position - 13, capWidth, 26);
        g.setColour(juce::Colours::black.withAlpha(0.16f));
        g.fillRoundedRectangle(thumb.translated(0, 3).expanded(1), 4);
        g.setGradientFill(juce::ColourGradient(juce::Colours::white, 0, thumb.getY(),
            juce::Colour(0xffcbd4dc), 0, thumb.getBottom(), false));
        g.fillRoundedRectangle(thumb, 3);
        g.setColour(juce::Colour(0xff8c9baa));
        g.drawRoundedRectangle(thumb, 3, 1);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        for (int offset : {-8, -5, 5, 8})
            g.drawHorizontalLine(juce::roundToInt(position)+offset, thumb.getX()+5, thumb.getRight()-5);
        g.setColour(SonoCraftTheme::accent());
        g.fillRect(thumb.getX()+1, position-1, thumb.getWidth()-2, 2.0f);
    }
};
