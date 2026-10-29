// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include <JuceHeader.h>
#include "SoundboardChannelProcessor.h"
#include "SoundboardButtonColors.h"

class SonoPlaybackProgressButton : public TextButton, public PlaybackPositionListener
{
public:
    SonoPlaybackProgressButton();
    explicit SonoPlaybackProgressButton(const String& buttonName);
    SonoPlaybackProgressButton(const String& buttonName, const String& toolTip);

    ~SonoPlaybackProgressButton() override;

    void setPlaybackPosition(double value);
    void setPadNumber(int number) { padNumber = number; }
    void setShortcutLabel(const String& label) { shortcutLabel = label; }
    bool isReorderGesture() const { return reorderGesture; }
    void setPadStyle(bool enabled) { padStyle = enabled; repaint(); }

    /**
     * @param newRgb The new button background colour with an alpha of 0.
     */
    void setButtonColour(int newRgb);

    void setShowEditArea(bool show);
    bool getShowEditArea() const { return showEditArea; }

    /**
     * Function that gets called whenever the button is clicked with the primary button.
     * onClick also gets called before this callback.
     */
    std::function<void(const ModifierKeys& mods)> onPrimaryClick;

    /**
     * Function that gets called whenever the button is clicked with the secondary button.
     * onClick also gets called before this callback.
     */
    std::function<void(const ModifierKeys& mods)> onSecondaryClick;

    void internalClickCallback(const ModifierKeys& mods) override
    {
        TextButton::internalClickCallback(mods);

        if (!ignoreNextClick) {
            // Keyboard/accessibility activation has no mouse-button flags.
            // Mouse-up may also arrive after the button flag has been cleared.
            if (clickIsEdit || mods.isPopupMenu()) {
                if (onSecondaryClick) onSecondaryClick(mods);
            } else {
                if (onPrimaryClick) onPrimaryClick(mods);
            }
        }
        clickIsEdit = false;

        ignoreNextClick = false;
    }

    void mouseDown (const MouseEvent& e) override
    {
        reorderGesture = padStyle && e.mods.isLeftButtonDown() && e.getPosition().x < 48 && e.getPosition().y < 30;
        ignoreNextClick = reorderGesture;
        clickIsEdit = e.mods.isPopupMenu();

        if (showEditArea && editBounds.contains(e.getPosition())) {
            clickIsEdit = true;
        }
        TextButton::mouseDown(e);
    }

    void mouseMove (const MouseEvent& e) override
    {
        if (showEditArea) {
            bool inarea = editBounds.contains(e.getPosition());
            if (inarea != overEditArea) {
                overEditArea = inarea;
                repaint();
            }
        }
        TextButton::mouseMove(e);
    }

    void mouseExit (const MouseEvent& e) override
    {
        if (overEditArea) {
            overEditArea = false;
            repaint();
        }

        TextButton::mouseExit(e);
    }

    void paintButton(Graphics& graphics, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void onPlaybackPositionChanged(SamplePlaybackManager* samplePlaybackManager) override;

    void attachToPlaybackManager(std::shared_ptr<SamplePlaybackManager> playbackManager);

    void setMouseListener(std::unique_ptr<MouseListener> listener);

    SamplePlaybackManager * getPlaybackManager() const { return playbackManager.get(); }

    void setIgnoreNextClick() { ignoreNextClick = true; }

    bool isClickEdit() const { return clickIsEdit; }

    void setPositionDragging(bool flag) { posDragging = flag; }
    bool isPositionDragging() const { return posDragging; }
    
private:
    bool padStyle = false, reorderGesture = false;
    int padNumber = 1;
    String shortcutLabel;

    void initStuff();

    constexpr static const uint32 PROGRESS_COLOUR = 0x33FFFFFF;
    constexpr static const uint32 PROGRESS_PLAY_OUTLINE_COLOUR = 0x99AAAAFF;

    double playbackPosition = 0.0;
    bool isPlaying = false;
    bool ignoreNextClick = false;
    bool clickIsEdit = false;
    bool overEditArea = false;

    bool showEditArea = true;
    bool posDragging = false;
    
    /**
     * The RGB value of the button background color with an alpha of 0.
     */
    uint32 buttonColour = SoundboardButtonColors::DEFAULT_BUTTON_COLOUR;

    std::shared_ptr<SamplePlaybackManager> playbackManager;

    std::unique_ptr<MouseListener> mouseListener;

    std::unique_ptr<Drawable> editImage;
    Rectangle<int> editBounds;
};
