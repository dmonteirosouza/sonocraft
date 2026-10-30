// Shared visual roles for the SonoCraft desktop interface.
#pragma once
#include "JuceHeader.h"
namespace SonoCraftTheme {
inline juce::Colour canvas() { return juce::Colour(0xfff2f4f7); }
inline juce::Colour panel() { return juce::Colour(0xffffffff); }
inline juce::Colour recessed() { return juce::Colour(0xffe9eef3); }
inline juce::Colour border() { return juce::Colour(0xffd8e0e8); }
inline juce::Colour text() { return juce::Colour(0xff243444); }
inline juce::Colour secondary() { return juce::Colour(0xff647586); }
inline juce::Colour accent() { return juce::Colour(0xff167d8d); }
inline juce::Colour selection() { return juce::Colour(0xffdceff0); }
inline juce::Colour danger() { return juce::Colour(0xffb43d4b); }
inline juce::Colour warning() { return juce::Colour(0xff9a581b); }
constexpr int space = 8;
constexpr float radius = 8.0f;
constexpr int panelInset = 12;
#if JUCE_IOS || JUCE_ANDROID
constexpr int headerHeight = 48;
#else
constexpr int headerHeight = 44;
#endif
// Shared frame for floating dialogs and docked panels. Keep child content
// inset so rectangular child backgrounds cannot cover the rounded corners.
inline void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds) {
    g.setColour(panel());
    g.fillRoundedRectangle(bounds, radius);
    g.setColour(border());
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);
}
}
