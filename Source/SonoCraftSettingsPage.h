#pragma once
#pragma push_macro("Component")
#undef Component
#include "JuceHeader.h"
#include "SonoCraftTheme.h"

// A scrollable form with one spacing and typography system for every settings page.
class SonoCraftSettingsPage final : public juce::Component, private juce::ComponentListener {
public:
    ~SonoCraftSettingsPage() override {
        for (auto& row : rows) if (row.component) row.component->removeComponentListener(this);
    }
    void control(juce::Component& component, int height = 34) {
        rows.push_back({ &component, height });
        addAndMakeVisible(component);
        if (height == 0) component.addComponentListener(this);
    }
    void heading(const char* text) { label(text, true); }
    void help(const char* text) { label(text, false); }
    void resized() override {
        if (layingOut) return;
        const juce::ScopedValueSetter<bool> guard(layingOut, true);
        const int width = juce::jmax(240, getWidth() - 40);
        int y = 14;
        for (auto& row : rows) {
            if (!row.component) continue;
            int height = row.height;
            if (height < 0) {
                auto* text = static_cast<juce::Label*>(row.component.getComponent());
                juce::AttributedString attributed;
                const auto font = text->getLookAndFeel().getLabelFont(*text);
                attributed.append(text->getText(), font, SonoCraftTheme::secondary());
                juce::TextLayout layout; layout.createLayout(attributed, float(width - 8));
                height = juce::jmax(32, int(std::ceil((layout.getNumLines() + 1) * font.getHeight())) + 8);
            } else if (height == 0) height = row.component->getHeight();
            row.component->setBounds(20, y, width, height);
            y += row.component->getHeight() + 8;
        }
        setSize(getWidth(), y + 14);
    }
private:
    struct Row { juce::Component::SafePointer<juce::Component> component; int height; };
    juce::OwnedArray<juce::Label> labels;
    std::vector<Row> rows;
    bool layingOut = false;
    void label(const char* text, bool heading) {
        auto* label = labels.add(new juce::Label({}, juce::String::fromUTF8(text)));
        label->setFont(juce::Font(heading ? 16.0f : 13.0f, heading ? juce::Font::bold : juce::Font::plain));
        label->setColour(juce::Label::textColourId, heading ? SonoCraftTheme::text() : SonoCraftTheme::secondary());
        label->setJustificationType(juce::Justification::centredLeft);
        label->setMinimumHorizontalScale(1.0f);
        control(*label, heading ? 36 : -1);
    }
    void componentMovedOrResized(juce::Component&, bool, bool wasResized) override { if (wasResized) resized(); }
};

#pragma pop_macro("Component")
