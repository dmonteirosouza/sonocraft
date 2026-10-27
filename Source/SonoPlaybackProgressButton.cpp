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
        graphics.setColour(background);
        graphics.fillRoundedRectangle(box, 10);
        graphics.setColour(ink.withAlpha(isPlaying ? 0.9f : 0.2f));
        graphics.drawRoundedRectangle(box, 10, isPlaying ? 2.0f : 1.0f);
        graphics.setColour(ink);
        graphics.setFont(juce::Font(14.0f, juce::Font::bold));
        graphics.drawFittedText(getButtonText().isEmpty() ? "Novo som" : getButtonText(), getLocalBounds().reduced(12).withTrimmedTop(12).withTrimmedBottom(24), juce::Justification::centredLeft, 2);
        graphics.setFont(11.0f);
        graphics.setColour(ink.withAlpha(0.85f));
        graphics.drawText(isPlaying ? "Tocando" : "Reproduzir", getLocalBounds().reduced(12).removeFromBottom(18), juce::Justification::centredLeft);
        graphics.setColour(ink.withAlpha(0.8f));
        graphics.fillRoundedRectangle(12, float(getHeight()-5), float(juce::jmax(0.0, playbackPosition) * (getWidth()-24)), 3, 1);
        editBounds = getLocalBounds().removeFromRight(28).removeFromTop(28);
        if (showEditArea) {
            graphics.setColour(ink);
            for (int i = 0; i < 3; ++i) graphics.fillEllipse(float(editBounds.getCentreX()), 10.0f + i*4, 2, 2);
        }
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
