// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SonoCraftTheme.h"
#include "SampleEditView.h"

#include <utility>
#include <iostream>

#if JUCE_ANDROID
#include "juce_core/native/juce_BasicNativeHeaders.h"
#include "juce_core/juce_core.h"
#include "juce_core/native/juce_JNIHelpers_android.h"
#endif

SampleEditView::SampleEditView(
                               std::function<void(SampleEditView&)> submitcallback,
                               std::function<void(SampleEditView&)> gaincallback,
                               const SoundSample* soundSample,
                               String* lastOpenedDirectoryString
                               )
: submitCallback(std::move(submitcallback)),
gainChangeCallback(std::move(gaincallback)),
editModeEnabled(soundSample != nullptr),
selectedColour(soundSample == nullptr ? SoundboardButtonColors::DEFAULT_BUTTON_COLOUR : soundSample->getButtonColour()),
initialName(soundSample == nullptr ? "" : soundSample->getName()),
initialPlaybackBehaviour(soundSample == nullptr ? SoundSample::PlaybackBehaviour::SIMULTANEOUS : soundSample->getPlaybackBehaviour()),
initialButtonBehaviour(soundSample == nullptr ? SoundSample::ButtonBehaviour::TOGGLE : soundSample->getButtonBehaviour()),
initialReplayBehaviour(soundSample == nullptr ? SoundSample::ReplayBehaviour::REPLAY_FROM_START : soundSample->getReplayBehaviour()),
initialEndPlaybackBehaviour(soundSample == nullptr ? SoundSample::EndPlaybackBehaviour::STOP_AT_END : soundSample->getEndPlaybackBehaviour()),
initialGain(soundSample == nullptr ? 1.0 : soundSample->getGain()),
hotkeyCode(soundSample == nullptr ? -1 : soundSample->getHotkeyCode()),
lastOpenedDirectory(lastOpenedDirectoryString),
mFileURL(soundSample == nullptr ? URL() : soundSample->getFileURL())
{
    setOpaque(true);

    if (!mFileURL.isEmpty()) {
        if (mFileURL.isLocalFile()) {
            initialFilePath = mFileURL.getLocalFile().getFullPathName();
        } else {
            initialFilePath = mFileURL.getFileName();
        }
    }
    
    createNameInputs();
    createFilePathInputs();
    createColourInput();
    createPlaybackOptionInputs();
    createVolumeInputs();
    createHotkeyInput();
    createButtonBar();
    editorTitle.setText("Editar pad", dontSendNotification);
    editorTitle.setFont(Font(20.0f, Font::bold));
    editorHint.setText(String::fromUTF8("As alterações são salvas automaticamente."), dontSendNotification);
    editorHint.setFont(12.0f);
    for (auto* label : { &editorTitle, &editorHint }) {
        label->setColour(Label::textColourId, SonoCraftTheme::text());
        addAndMakeVisible(label);
    }
    const char* names[] = { "Ao clicar", "Ao terminar", "Com outros pads", "Ao tocar novamente" };
    const char* hints[] = { "Escolha como o botão inicia e interrompe o som.",
        "O próximo pad segue a ordem da mesa.", "Fundo contínuo não é interrompido por outros pads.",
        "Escolha de onde a reprodução será retomada." };
    ComboBox* choices[] = {mButtonBehaviourButton.get(), mLoopButton.get(), mPlaybackBehaviourButton.get(), mReplayBehaviourButton.get()};
    for (int i = 0; i < 4; ++i) {
        optionLabels[i].setText(String::fromUTF8(names[i]), dontSendNotification);
        optionLabels[i].setFont(Font(13.0f, Font::bold));
        optionLabels[i].setColour(Label::textColourId, SonoCraftTheme::text());
        optionHints[i].setText(String::fromUTF8(hints[i]), dontSendNotification);
        optionHints[i].setFont(11.5f);
        optionHints[i].setColour(Label::textColourId, SonoCraftTheme::secondary());
        choices[i]->setTitle(String::fromUTF8(names[i]));
        choices[i]->setTooltip(String::fromUTF8(hints[i]));
        addAndMakeVisible(optionLabels[i]); addAndMakeVisible(optionHints[i]);
    }
    mSubmitButton = std::make_unique<SonoTextButton>(String::fromUTF8("Concluído"));
    mSubmitButton->onClick = [this] { submitDialog(); };
    addAndMakeVisible(mSubmitButton.get());
}

void SampleEditView::setEditMode(bool flag)
{
    editModeEnabled = flag;
    if (mDeleteButton) {
        mDeleteButton->setButtonText( isEditMode() ? TRANS("Delete") : TRANS("Cancel"));
    }
}

void SampleEditView::createNameInputs()
{
    mNameLabel = std::make_unique<Label>("nameLabel", TRANS("Name"));
    mNameLabel->setJustificationType(Justification::left);
    mNameLabel->setFont(Font(14, Font::bold));
    mNameLabel->setColour(Label::textColourId, SonoCraftTheme::text());
    addAndMakeVisible(mNameLabel.get());

    mNameInput = std::make_unique<TextEditor>("nameInput");
    mNameInput->setText(initialName);
    mNameInput->onReturnKey = [this]() {
        submitDialog(false);
        mNameInput->giveAwayKeyboardFocus();
    };
    mNameInput->onEscapeKey = [this]() {
        dismissDialog();
    };
    addAndMakeVisible(mNameInput.get());
}

void SampleEditView::createFilePathInputs()
{
    mFilePathLabel = std::make_unique<Label>("filePathLabel", TRANS("File"));
    mFilePathLabel->setJustificationType(Justification::left);
    mFilePathLabel->setFont(Font(14, Font::bold));
    mFilePathLabel->setColour(Label::textColourId, SonoCraftTheme::text());
    addAndMakeVisible(mFilePathLabel.get());

    mFilePathInput = std::make_unique<TextEditor>("filePathInput");
    mFilePathInput->setText(initialFilePath);
#if JUCE_IOS || JUCE_ANDROID
    mFilePathInput->setReadOnly(true);
#endif
    addAndMakeVisible(mFilePathInput.get());

    mBrowseFilePathButton = std::make_unique<SonoTextButton>(TRANS("Browse"));
    mBrowseFilePathButton->onClick = [this]() {
        browseFilePath();
    };
    addAndMakeVisible(mBrowseFilePathButton.get());


}

void SampleEditView::createColourInput()
{
    auto buttonH = 32;
#if JUCE_IOS || JUCE_ANDROID
    buttonH = 36;
#endif

    mColourInputLabel = std::make_unique<Label>("colourInputLabel", String::fromUTF8("Cor do pad"));
    mColourInputLabel->setJustificationType(Justification::left);
    mColourInputLabel->setFont(Font(14, Font::bold));
    mColourInputLabel->setColour(Label::textColourId, SonoCraftTheme::text());
    addAndMakeVisible(mColourInputLabel.get());

    // Add default 11 colour buttons.
    colourButtonRowTopBox.flexDirection = FlexBox::Direction::row;
    colourButtonRowTopBox.items.add(FlexItem(ELEMENT_MARGIN * 2, ELEMENT_MARGIN).withMargin(0));
    for (int i = 0; i < 6; ++i) {
        auto colourButton = createColourButton(i);
        colourButtonRowTopBox.items.add(FlexItem(32, buttonH, *colourButton).withMargin(0).withFlex(1));
        colourButtonRowTopBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
        mColourButtons.emplace_back(std::move(colourButton));
    }

    colourButtonRowBottomBox.flexDirection = FlexBox::Direction::row;
    colourButtonRowBottomBox.items.add(FlexItem(ELEMENT_MARGIN * 2, ELEMENT_MARGIN).withMargin(0));
    for (int i = 6; i < 11; ++i) {
        auto colourButton = createColourButton(i);
        colourButtonRowBottomBox.items.add(FlexItem(32, buttonH, *colourButton).withMargin(0).withFlex(1));
        colourButtonRowBottomBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
        mColourButtons.emplace_back(std::move(colourButton));
    }

    // Add colour picker button.
    auto customColourButton = std::make_unique<SonoDrawableButton>("colourPicker", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    mColourPicker = std::make_unique<SoundSampleButtonColourPicker>(&selectedColour, customColourButton.get());
    mColourPicker->colourChangedCallback = [this]() {
        updateColourButtonCheckmark();
        submitDialog(false); // no dismiss
    };
    customColourButton->onClick = [this]() {
        mColourPicker->show(getScreenBounds());
    };
    customColourButton->setColour(
            SonoTextButton::buttonColourId,
            Colour(selectedColour | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA)
    );
    addAndMakeVisible(customColourButton.get());

    std::unique_ptr<Drawable> eyedropperImage(Drawable::createFromImageData(BinaryData::eyedropper_svg, BinaryData::eyedropper_svgSize));
    customColourButton->setImages(eyedropperImage.get());
    customColourButton->setForegroundImageRatio(0.55f);

    colourButtonRowBottomBox.items.add(FlexItem(32, buttonH, *customColourButton).withMargin(0).withFlex(1));
    colourButtonRowBottomBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    mColourButtons.emplace_back(std::move(customColourButton));

    colourButtonBox.flexDirection = FlexBox::Direction::column;
    colourButtonBox.items.add(FlexItem(32 * 6, buttonH, colourButtonRowTopBox).withMargin(0).withFlex(0));
    colourButtonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    colourButtonBox.items.add(FlexItem(32 * 6, buttonH, colourButtonRowBottomBox).withMargin(0).withFlex(0));

    updateColourButtonCheckmark();
}

void SampleEditView::createPlaybackOptionInputs()
{
    mPlaybackOptionsLabel = std::make_unique<Label>("playbackOptionsLabel", TRANS("Playback options"));
    mPlaybackOptionsLabel->setJustificationType(Justification::left);
    mPlaybackOptionsLabel->setFont(Font(14, Font::bold));
    mPlaybackOptionsLabel->setColour(Label::textColourId, SonoCraftTheme::text());
    addAndMakeVisible(mPlaybackOptionsLabel.get());


    createButtonBehaviourButton();

    createLoopButton();

    createPlaybackBehaviourButton();

    createReplayBehaviourButton();
    
    mApplyPlaybackOptionsToOthersButton = std::make_unique<SonoTextButton>(String::fromUTF8("Aplicar reprodução aos outros pads desta mesa"));
    mApplyPlaybackOptionsToOthersButton->setTooltip(TRANS("Apply these options to all samples in the selected soundboard"));
    mApplyPlaybackOptionsToOthersButton->onClick = [this]() {
        if (applyPlaybackOptionsToOthersCallback) {
            applyPlaybackOptionsToOthersCallback(*this);
        }
    };
    addAndMakeVisible(mApplyPlaybackOptionsToOthersButton.get());
}

void SampleEditView::createVolumeInputs()
{
    mVolumeLabel = std::make_unique<Label>("gainLabel", String::fromUTF8("Volume"));
    mVolumeLabel->setJustificationType(Justification::left);
    mVolumeLabel->setFont(Font(14, Font::bold));
    mVolumeLabel->setColour(Label::textColourId, SonoCraftTheme::text());
    addAndMakeVisible(mVolumeLabel.get());

    mVolumeSlider = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxRight);
    mVolumeSlider->setRange(0.0, 2.0, 0.0);
    mVolumeSlider->setSkewFactor(0.5);
    mVolumeSlider->setName("volumeSlider");
    mVolumeSlider->setTitle(TRANS("Playback gain level"));
    mVolumeSlider->setSliderSnapsToMousePosition(false);
    mVolumeSlider->setDoubleClickReturnValue(true, 1.0);
    mVolumeSlider->setTextBoxIsEditable(true);
    mVolumeSlider->setScrollWheelEnabled(false);
    mVolumeSlider->setMouseDragSensitivity(256);
    mVolumeSlider->setTextBoxStyle(Slider::TextBoxRight, false, 60, 32);
    mVolumeSlider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
    mVolumeSlider->textFromValueFunction = [](float v) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); };
    mVolumeSlider->setValue(initialGain);
    mVolumeSlider->setChangeNotificationOnlyOnRelease(false);
    mVolumeSlider->onValueChange = [this]() {
        if (gainChangeCallback)
            gainChangeCallback(*this);
    };

    addAndMakeVisible(mVolumeSlider.get());


}

void SampleEditView::createLoopButton()
{
    mLoopButton = std::make_unique<ComboBox>();
    mLoopButton->addItem(String::fromUTF8("Parar no final"), 1);
    mLoopButton->addItem(String::fromUTF8("Repetir em loop"), 2);
    mLoopButton->addItem(String::fromUTF8("Tocar o próximo pad"), 3);
    mLoopButton->setSelectedItemIndex(int(initialEndPlaybackBehaviour), dontSendNotification);
    mLoopButton->setColour(ComboBox::backgroundColourId, SonoCraftTheme::panel());
    mLoopButton->setColour(ComboBox::textColourId, SonoCraftTheme::text());
    mLoopButton->setColour(ComboBox::arrowColourId, SonoCraftTheme::secondary());
    mLoopButton->onChange = [this]() { submitDialog(false); };
    addAndMakeVisible(mLoopButton.get());
}

void SampleEditView::createReplayBehaviourButton()
{
    mReplayBehaviourButton = std::make_unique<ComboBox>();
    mReplayBehaviourButton->addItem(String::fromUTF8("Voltar ao início"), 1);
    mReplayBehaviourButton->addItem(String::fromUTF8("Continuar de onde parou"), 2);
    mReplayBehaviourButton->setSelectedItemIndex(int(initialReplayBehaviour), dontSendNotification);
    mReplayBehaviourButton->setColour(ComboBox::backgroundColourId, SonoCraftTheme::panel());
    mReplayBehaviourButton->setColour(ComboBox::textColourId, SonoCraftTheme::text());
    mReplayBehaviourButton->setColour(ComboBox::arrowColourId, SonoCraftTheme::secondary());
    mReplayBehaviourButton->onChange = [this]() { submitDialog(false); };
    addAndMakeVisible(mReplayBehaviourButton.get());
}

void SampleEditView::createPlaybackBehaviourButton()
{
    mPlaybackBehaviourButton = std::make_unique<ComboBox>();
    mPlaybackBehaviourButton->addItem(String::fromUTF8("Misturar com outros pads"), 1);
    mPlaybackBehaviourButton->addItem(String::fromUTF8("Parar os outros pads"), 2);
    mPlaybackBehaviourButton->addItem(String::fromUTF8("Fundo contínuo"), 3);
    mPlaybackBehaviourButton->setSelectedItemIndex(int(initialPlaybackBehaviour), dontSendNotification);
    mPlaybackBehaviourButton->setColour(ComboBox::backgroundColourId, SonoCraftTheme::panel());
    mPlaybackBehaviourButton->setColour(ComboBox::textColourId, SonoCraftTheme::text());
    mPlaybackBehaviourButton->setColour(ComboBox::arrowColourId, SonoCraftTheme::secondary());
    mPlaybackBehaviourButton->onChange = [this]() { submitDialog(false); };
    addAndMakeVisible(mPlaybackBehaviourButton.get());
}

void SampleEditView::createButtonBehaviourButton()
{
    mButtonBehaviourButton = std::make_unique<ComboBox>();
    mButtonBehaviourButton->addItem(String::fromUTF8("Clique para tocar / parar"), 1);
    mButtonBehaviourButton->addItem(String::fromUTF8("Tocar enquanto pressionado"), 2);
    mButtonBehaviourButton->addItem(String::fromUTF8("Disparar a cada clique"), 3);
    mButtonBehaviourButton->setSelectedItemIndex(int(initialButtonBehaviour), dontSendNotification);
    mButtonBehaviourButton->setColour(ComboBox::backgroundColourId, SonoCraftTheme::panel());
    mButtonBehaviourButton->setColour(ComboBox::textColourId, SonoCraftTheme::text());
    mButtonBehaviourButton->setColour(ComboBox::arrowColourId, SonoCraftTheme::secondary());
    mButtonBehaviourButton->onChange = [this]() { submitDialog(false); };
    addAndMakeVisible(mButtonBehaviourButton.get());
}

void SampleEditView::createHotkeyInput()
{
    mHotkeyLabel = std::make_unique<Label>("hotkeyLabel", String::fromUTF8("Atalho"));
    mHotkeyLabel->setJustificationType(Justification::left);
    mHotkeyLabel->setFont(Font(14, Font::bold));
    mHotkeyLabel->setColour(Label::textColourId, SonoCraftTheme::text());
    addAndMakeVisible(mHotkeyLabel.get());

    auto keyPress = juce::KeyPress(hotkeyCode);
    auto buttonText = hotkeyCode == -1 ? String::fromUTF8("Definir tecla...") : keyPress.getTextDescriptionWithIcons();
    mHotkeyButton = std::make_unique<SonoTextButton>(buttonText);
    mHotkeyButton->setClickingTogglesState(true);
    mHotkeyButton->setToggleState(false, dontSendNotification);
    mHotkeyButton->onClick = [this]() {
        if (mHotkeyButton->getToggleState()) {
            mHotkeyButton->setButtonText(TRANS("Press a key..."));
        }
    };
    addAndMakeVisible(mHotkeyButton.get());

    mRemoveHotkeyButton = std::make_unique<SonoTextButton>(String::fromUTF8("Remover atalho"));
    mRemoveHotkeyButton->onClick = [this]() {
        mHotkeyButton->setButtonText(String::fromUTF8("Definir tecla..."));
        mHotkeyButton->setToggleState(false, dontSendNotification);
        hotkeyCode = -1;
        submitDialog(false);
    };
    addAndMakeVisible(mRemoveHotkeyButton.get());

}

std::unique_ptr<SonoDrawableButton> SampleEditView::createColourButton(const int index)
{
    auto colourButton = std::make_unique<SonoDrawableButton>("colour", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    colourButton->setColour(
            SonoTextButton::buttonColourId,
            Colour(BUTTON_COLOURS[index] | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA)
    );
    colourButton->onClick = [this, index]() {
        selectedColour = BUTTON_COLOURS[index];
        updateColourButtonCheckmark();

        // Also update the eyedropper to reflect the current colour.
        auto& pickerButton = mColourButtons[mColourButtons.size() - 1];
        pickerButton->setColour(
                SonoTextButton::buttonColourId,
                Colour(selectedColour | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA)
        );
        // and call submit callback (no dismiss)
        submitDialog(false);
    };

    addAndMakeVisible(colourButton.get());
    return colourButton;
}

void SampleEditView::updateColourButtonCheckmark()
{
    std::unique_ptr<Drawable> checkImage(Drawable::createFromImageData(BinaryData::checkmark_svg, BinaryData::checkmark_svgSize));
    checkImage->replaceColour(Colour(0xffcccccc), SonoCraftTheme::text());
    auto selectedIndex = indexOfColour(selectedColour);
    auto buttonCount = mColourButtons.size() - 1; /* last one is custom colour */

    for (int i = 0; i < buttonCount; ++i) {
        auto& colourButton = mColourButtons[i];

        // Show checkmark.
        if (i == selectedIndex) {
            colourButton->setForegroundImageRatio(0.55f);
            colourButton->setImages(checkImage.get());
        }
        // Hide checkmark.
        else {
            std::unique_ptr<Drawable> emptyImage = std::make_unique<DrawableRectangle>();
            colourButton->setImages(emptyImage.get());
        }
    }
}

void SampleEditView::createButtonBar()
{
    mDeleteButton = std::make_unique<SonoTextButton>(isEditMode() ? TRANS("Delete") : TRANS("Cancel"));
    mDeleteButton->setColour(SonoTextButton::buttonColourId, SonoCraftTheme::danger().withAlpha(0.12f));
    mDeleteButton->onClick = [this]() {
        if (isEditMode()) {
            deleteSample = true;
            submitDialog();
            // and make sure no more callbacks happen
            submitCallback = nullptr;
            gainChangeCallback = nullptr;
        }
        else {
            dismissDialog();
        }
    };
    addAndMakeVisible(mDeleteButton.get());

    buttonBox.flexDirection = FlexBox::Direction::row;
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0).withFlex(1));
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    buttonBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH / 4, CONTROL_HEIGHT, *mDeleteButton).withMargin(0).withFlex(1).withMaxWidth(140));
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
}

String SampleEditView::getSampleName() const
{
    return mNameInput->getText().trim();
}

String SampleEditView::getAbsoluteFilePath() const
{
    return mFilePathInput->getText().trim();
}

juce::URL SampleEditView::getFileURL() const
{
    return mFileURL;
}

float SampleEditView::getGain() const
{
    return mVolumeSlider->getValue();
}

void SampleEditView::browseFilePath()
{
    
#if JUCE_ANDROID
    if (getAndroidSDKVersion() < 29) {
        SafePointer<SampleEditView> safeThis (this);
        if (! RuntimePermissions::isGranted (RuntimePermissions::readExternalStorage))
        {
            RuntimePermissions::request (RuntimePermissions::readExternalStorage,
                                         [safeThis] (bool granted) mutable
                                         {
                if (granted)
                    safeThis->browseFilePath();
            });
            return;
        }
    }
#endif
    
    // Determine where to open the file chooser.
    File defaultDirectory = File::getSpecialLocation(File::userMusicDirectory);
    if (lastOpenedDirectory != nullptr) {
        defaultDirectory = File(*lastOpenedDirectory);
    }

    // Open the file chooser.
    mFileChooser = std::make_unique<FileChooser>(
            TRANS("Select an audio file..."),
            defaultDirectory,
            SoundSample::SUPPORTED_EXTENSIONS,
            true,
            false,
            this->getParentComponent()
    );
    auto folderFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

    mFileChooser->launchAsync(folderFlags, [this](const FileChooser& chooser) {
        URL chosenUrl = chooser.getURLResult();

        if (chosenUrl.isEmpty()) {
            mFilePathInput->clear();
        }
        else {
            if (chosenUrl.isLocalFile()) {
                auto lfile = chosenUrl.getLocalFile();
                mFilePathInput->setText(lfile.getFullPathName(), true);
                
                if (lastOpenedDirectory != nullptr) {
                    *lastOpenedDirectory = lfile.getParentDirectory().getFullPathName();
                }
            }
            
            mFileURL = chosenUrl;
            inferSampleName();
        }

    });
}

void SampleEditView::inferSampleName()
{
    auto absolutePath = getAbsoluteFilePath();
    if (!mNameInput->isEmpty() || absolutePath.isEmpty()) {
        return;
    }

    auto probablyFile = File(getAbsoluteFilePath());
    mNameInput->setText(probablyFile.getFileNameWithoutExtension());
}

void SampleEditView::submitDialog(bool dismiss)
{
    auto inputtedName = mNameInput->getText().trim();
    if (inputtedName.isEmpty() && !deleteSample) {
        mNameInput->setColour(TextEditor::backgroundColourId, SonoCraftTheme::danger().withAlpha(0.12f));
        return;
    }

    this->initialName = inputtedName;

    if (submitCallback)
        submitCallback(*this);

    if (dismiss) {
        dismissDialog();
    }
}

void SampleEditView::dismissDialog()
{
    auto callOutBox = findParentComponentOfClass<CallOutBox>();
    if (callOutBox) {
        callOutBox->dismiss();
    }
}

bool SampleEditView::keyPressed(const KeyPress& keyPress)
{
    // Set new hotkey only when the hotkey button is ready to receive a new hotkey.
    if (!mHotkeyButton->getToggleState()) {
        return false;
    }

    auto keyPressWithoutModifier = juce::KeyPress(keyPress.getKeyCode());
    mHotkeyButton->setButtonText(keyPressWithoutModifier.getTextDescriptionWithIcons());
    mHotkeyButton->setToggleState(false, dontSendNotification);
    hotkeyCode = keyPress.getKeyCode();
    submitDialog(false);
    return true;
}

void SampleEditView::paint(Graphics& g)
{
    g.fillAll(SonoCraftTheme::canvas());
}

void SampleEditView::resized()
{
#if JUCE_IOS || JUCE_ANDROID
    auto mobile = getLocalBounds().withHeight(1400).reduced(16);
    editorTitle.setBounds(mobile.removeFromTop(30));
    editorHint.setBounds(mobile.removeFromTop(36));
    mNameLabel->setBounds(mobile.removeFromTop(24));
    mNameInput->setBounds(mobile.removeFromTop(44)); mobile.removeFromTop(12);
    mFilePathLabel->setBounds(mobile.removeFromTop(24));
    auto fileRow=mobile.removeFromTop(44);
    mBrowseFilePathButton->setBounds(fileRow.removeFromRight(100)); fileRow.removeFromRight(8);
    mFilePathInput->setBounds(fileRow); mobile.removeFromTop(12);
    mColourInputLabel->setBounds(mobile.removeFromTop(26));
    colourButtonBox.performLayout(mobile.removeFromTop(84)); mobile.removeFromTop(12);
    mPlaybackOptionsLabel->setBounds(mobile.removeFromTop(30));
    ComboBox* mobileChoices[] = {mButtonBehaviourButton.get(),mLoopButton.get(),mPlaybackBehaviourButton.get(),mReplayBehaviourButton.get()};
    for(int i=0;i<4;++i){
        optionLabels[i].setBounds(mobile.removeFromTop(26));
        mobileChoices[i]->setBounds(mobile.removeFromTop(44));
        optionHints[i].setMinimumHorizontalScale(1);
        optionHints[i].setBounds(mobile.removeFromTop(40)); mobile.removeFromTop(8);
    }
    mApplyPlaybackOptionsToOthersButton->setButtonText(String::fromUTF8("Aplicar aos outros pads"));
    mApplyPlaybackOptionsToOthersButton->setBounds(mobile.removeFromTop(44)); mobile.removeFromTop(12);
    mVolumeLabel->setBounds(mobile.removeFromTop(26));
    mVolumeSlider->setBounds(mobile.removeFromTop(48)); mobile.removeFromTop(16);
    for(auto* c : std::initializer_list<Component*>{mHotkeyLabel.get(),mHotkeyButton.get(),mRemoveHotkeyButton.get()})c->setVisible(false);
    auto actions=mobile.removeFromTop(48);
    mDeleteButton->setBounds(actions.removeFromLeft((actions.getWidth()-12)/2)); actions.removeFromLeft(12);
    mSubmitButton->setBounds(actions);
    minContentHeight=actions.getBottom()+16; minContentWidth=280;
    return;
#endif
    auto r = getLocalBounds().reduced(14);
    editorTitle.setBounds(r.removeFromTop(28));
    editorHint.setBounds(r.removeFromTop(22));
    r.removeFromTop(10);
    auto row = r.removeFromTop(32);
    mNameLabel->setBounds(row.removeFromLeft(64)); mNameInput->setBounds(row);
    r.removeFromTop(8); row = r.removeFromTop(32);
    mFilePathLabel->setBounds(row.removeFromLeft(64));
    mBrowseFilePathButton->setBounds(row.removeFromRight(86)); row.removeFromRight(8);
    mFilePathInput->setBounds(row);
    r.removeFromTop(12);
    mColourInputLabel->setBounds(r.removeFromTop(22));
    colourButtonBox.performLayout(r.removeFromTop(68));
    r.removeFromTop(12);
    mPlaybackOptionsLabel->setBounds(r.removeFromTop(26));
    ComboBox* choices[] = {mButtonBehaviourButton.get(), mLoopButton.get(), mPlaybackBehaviourButton.get(), mReplayBehaviourButton.get()};
    for (int i = 0; i < 4; ++i) {
        row = r.removeFromTop(32);
        optionLabels[i].setBounds(row.removeFromLeft(getWidth() < 400 ? 112 : 142));
        choices[i]->setBounds(row);
        optionHints[i].setBounds(r.removeFromTop(22));
        r.removeFromTop(6);
    }
    mApplyPlaybackOptionsToOthersButton->setBounds(r.removeFromTop(30));
    r.removeFromTop(12); row = r.removeFromTop(36);
    mVolumeLabel->setBounds(row.removeFromLeft(64)); mVolumeSlider->setBounds(row);
    r.removeFromTop(8); row = r.removeFromTop(34);
    mHotkeyLabel->setBounds(row.removeFromLeft(64));
    mRemoveHotkeyButton->setBounds(row.removeFromRight(jmin(150, row.getWidth()/2)));
    row.removeFromRight(8); mHotkeyButton->setBounds(row);
    r.removeFromTop(12); row = r.removeFromTop(34);
    mDeleteButton->setBounds(row.removeFromLeft(110));
    mSubmitButton->setBounds(row.removeFromRight(110));
    minContentHeight = row.getBottom() + 14;
    minContentWidth = 340;
}
