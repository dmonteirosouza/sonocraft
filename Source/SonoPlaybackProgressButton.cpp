// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "SonoPlaybackProgressButton.h"
#include "SonoCraftTheme.h"

#include <utility>

SonoPlaybackProgressButton::SonoPlaybackProgressButton() : TextButton()
{
    initStuff();
}

SonoPlaybackProgressButton::SonoPlaybackProgressButton(const String& buttonName) : TextButton(buttonName)
{
    initStuff();
}

SonoPlaybackProgressButton::SonoPlaybackProgressButton(const String& buttonName, const String& toolTip)
        : TextButton(buttonName, toolTip)
{
    initStuff();
}

SonoPlaybackProgressButton::~SonoPlaybackProgressButton()
{
    if (playbackManager != nullptr) {
        playbackManager->detach(this);
    }
}

void SonoPlaybackProgressButton::initStuff()
{
    editImage = Drawable::createFromImageData(BinaryData::dots_svg, BinaryData::dots_svgSize);
}

void SonoPlaybackProgressButton::setPlaybackPosition(double value)
{
    playbackPosition = value;
}

void SonoPlaybackProgressButton::setButtonColour(int newRgb)
{
    buttonColour = newRgb;
}

void SonoPlaybackProgressButton::setShowEditArea(bool show)
{
    if (showEditArea != show) {
        showEditArea = show;
        repaint();
    }
}


void SonoPlaybackProgressButton::paintButton(Graphics& graphics,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    if (padStyle) {
        using namespace SonoCraftTheme;
        auto box = getLocalBounds().toFloat().reduced(1);
        auto background = juce::Colour(buttonColour | 0xff000000);
        if (shouldDrawButtonAsHighlighted) background = background.brighter(0.08f);
        if (shouldDrawButtonAsDown) background = background.darker(0.12f);
        auto linear = [](float value) { return value <= 0.04045f ? value / 12.92f : std::pow((value+0.055f)/1.055f, 2.4f); };
        const float luminance = 0.2126f*linear(background.getFloatRed()) + 0.7152f*linear(background.getFloatGreen()) + 0.0722f*linear(background.getFloatBlue());
        const auto ink = luminance > 0.179f ? juce::Colour(0xff101820) : juce::Colours::white;
        graphics.setColour(Colours::black.withAlpha(0.14f));
        graphics.fillRoundedRectangle(box.translated(0, 2), 9);
        graphics.setGradientFill(ColourGradient(background.brighter(0.10f), 0, 0,
            background.darker(0.12f), 0, float(getHeight()), false));
        graphics.fillRoundedRectangle(box.withTrimmedBottom(2), 9);
        graphics.setColour(ink.withAlpha(isPlaying || hasKeyboardFocus(true) ? 0.95f : 0.25f));
        graphics.drawRoundedRectangle(box.reduced(2), 7, isPlaying || hasKeyboardFocus(true) ? 2.0f : 1.0f);
        // A dedicated grip keeps ordering separate from playback and scrubbing.
        graphics.setColour(ink.withAlpha(0.7f));
        for (int x = 0; x < 2; ++x) for (int y = 0; y < 3; ++y)
            graphics.fillEllipse(11.0f + x*4, 10.0f + y*4, 2, 2);
        graphics.setFont(Font(11.0f, Font::bold));
        graphics.drawText(String(padNumber).paddedLeft('0', 2), 24, 6, 24, 22, Justification::centredLeft);
        editBounds = getLocalBounds().removeFromRight(32).removeFromTop(30);
        if (showEditArea) {
            graphics.setColour(ink.withAlpha(overEditArea ? 0.22f : 0.10f));
            graphics.fillRoundedRectangle(editBounds.toFloat().reduced(4), 4);
            graphics.setColour(ink);
            for (int i = 0; i < 3; ++i) graphics.fillEllipse(float(editBounds.getCentreX()-5+i*4), 13, 2, 2);
        }
        graphics.setColour(ink);
        graphics.setFont(Font(15.0f, Font::bold));
        graphics.drawFittedText(getButtonText().isEmpty() ? "Novo som" : getButtonText(),
            getLocalBounds().reduced(12).withTrimmedTop(20).withTrimmedBottom(35), Justification::centredLeft, 2);
        auto footer = getLocalBounds().reduced(12).removeFromBottom(24);
        Path symbol;
        const float sy = float(footer.getCentreY());
        if (isPlaying) graphics.fillRoundedRectangle(12, sy-5, 10, 10, 1);
        else { symbol.addTriangle(12, sy-6, 12, sy+6, 22, sy); graphics.fillPath(symbol); }
        graphics.setFont(11.0f);
        graphics.drawText(isPlaying ? "Tocando" : "Tocar", footer.withTrimmedLeft(17), Justification::centredLeft);
        if (shortcutLabel.isNotEmpty()) {
            auto key = footer.removeFromRight(32).reduced(0, 3);
            graphics.setColour(ink.withAlpha(0.12f)); graphics.fillRoundedRectangle(key.toFloat(), 3);
            graphics.setColour(ink); graphics.drawFittedText(shortcutLabel, key, Justification::centred, 1);
        }
        graphics.setColour(ink.withAlpha(0.15f));
        graphics.fillRoundedRectangle(12, float(getHeight()-9), float(getWidth()-24), 3, 1);
        graphics.setColour(ink);
        graphics.fillRoundedRectangle(12, float(getHeight()-9), float(jlimit(0.0, 1.0, playbackPosition) * (getWidth()-24)), 3, 1);
        return;
    }
    auto& lf = getLookAndFeel();

    lf.drawButtonBackground(
            graphics,
            *this,
            Colour(buttonColour | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA),
            shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown
    );

    // Probably not the best place for this code
    auto cornerSize = 6.0f;
    auto bounds = getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    bounds = bounds.withWidth(playbackPosition * bounds.getWidth());
    auto colour = Colour(PROGRESS_COLOUR);

    graphics.setColour(colour);
    graphics.fillRoundedRectangle(bounds, cornerSize);
    if (isPlaying) {
        graphics.setColour(Colour(PROGRESS_PLAY_OUTLINE_COLOUR));
        graphics.drawRoundedRectangle(bounds, cornerSize, 2.0f);
    }

    lf.drawButtonText(graphics, *this, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    if (showEditArea) {
        auto imgbounds = getLocalBounds().toFloat().reduced(2).withLeft(getLocalBounds().getRight() - getLocalBounds().getHeight());
        editBounds = imgbounds.toNearestIntEdges();
        if (overEditArea) {
            graphics.setColour(colour);
            graphics.fillRoundedRectangle(imgbounds, 6.0f);
        }
        editImage->drawWithin(graphics, imgbounds, RectanglePlacement(RectanglePlacement::centred), 0.6f);
    }
}

void SonoPlaybackProgressButton::onPlaybackPositionChanged(SamplePlaybackManager * manager)
{
    auto position = manager->getLength() != 0.0
        ? manager->getCurrentPosition() / manager->getLength()
        : 0.0;

    if (!ignoreNextClick) {
        if (abs(playbackPosition - position) > 1e-10 || manager->isPlaying() != isPlaying) {
            isPlaying = manager->isPlaying();
            setPlaybackPosition(position);
            repaint();
        }
    }
}

void SonoPlaybackProgressButton::attachToPlaybackManager(std::shared_ptr<SamplePlaybackManager> playbackManager_)
{
    playbackManager = std::move(playbackManager_);
    playbackManager->detach(this);
    playbackManager->attach(this);
}

void SonoPlaybackProgressButton::setMouseListener(std::unique_ptr<MouseListener> listener) {
    mouseListener = std::move(listener);
    addMouseListener(mouseListener.get(), false);
}
