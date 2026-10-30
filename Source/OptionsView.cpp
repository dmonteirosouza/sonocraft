// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell

#include "SonoCraftTheme.h"
#include "OptionsView.h"

#if JUCE_ANDROID
#include "juce_core/native/juce_BasicNativeHeaders.h"
#include "juce_core/juce_core.h"
#include "juce_core/native/juce_JNIHelpers_android.h"
#endif

using namespace SonoAudio;

class SonobusOptionsTabbedComponent : public TabbedComponent
{
public:
    SonobusOptionsTabbedComponent(TabbedButtonBar::Orientation orientation, OptionsView & editor_) : TabbedComponent(orientation), editor(editor_) {

    }

    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override {

        editor.optionsTabChanged(newCurrentTabIndex);
    }

protected:
    OptionsView & editor;

};


enum {
    nameTextColourId = 0x1002830,
    selectedColourId = 0x1002840,
    separatorColourId = 0x1002850,
};


void OptionsView::initializeLanguages()
{
    // TODO smarter way of figuring out what languages are available
    languages.add(TRANS("System Default Language")); languagesNative.add(""); codes.add("");

    languages.add(TRANS("English")); languagesNative.add("English"); codes.add("en");
    languages.add(TRANS("Spanish")); languagesNative.add(CharPointer_UTF8 ("espa\xc3\xb1ol")); codes.add("es");
    languages.add(TRANS("French"));  languagesNative.add(CharPointer_UTF8 ("fran\xc3\xa7""ais")); codes.add("fr");
    languages.add(TRANS("Italian"));  languagesNative.add("italiano"); codes.add("it");
    languages.add(TRANS("German"));  languagesNative.add("Deutsch"); codes.add("de");
    languages.add(TRANS("Portuguese (Portugal)"));  languagesNative.add(CharPointer_UTF8 ("Portugu\xc3\xaas (Portugal)")); codes.add("pt-pt");
    languages.add(TRANS("Portuguese (Brazil)"));  languagesNative.add(CharPointer_UTF8 ("Portugu\xc3\xaas (Brasil)")); codes.add("pt-br");
    languages.add(TRANS("Dutch"));  languagesNative.add("Nederlands"); codes.add("nl");

    if (processor.getUseUniversalFont()) {
        languages.add(TRANS("Japanese")); languagesNative.add(CharPointer_UTF8 ("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e")); codes.add("ja");
    } else {
        languages.add(TRANS("Japanese")); languagesNative.add("Japanese"); codes.add("ja"); // TODO fix and use above when we have a font that can display this all the time
    }

    if (processor.getUseUniversalFont()) {
        languages.add(TRANS("Korean")); languagesNative.add(juce::CharPointer_UTF8 ("\xed\x95\x9c\xea\xb5\xad\xec\x9d\xb8")); codes.add("ko");
    } else {
        languages.add(TRANS("Korean")); languagesNative.add("Korean"); codes.add("ko");
    }

    if (processor.getUseUniversalFont()) {
        languages.add(TRANS("Chinese (Simplified)")); languagesNative.add(CharPointer_UTF8 ("\xe4\xb8\xad\xe6\x96\x87\xef\xbc\x88\xe7\xae\x80\xe4\xbd\x93\xef\xbc\x89")); codes.add("zh-hans");
    }
    else {
        languages.add(TRANS("Chinese (Simplified)")); languagesNative.add("Chinese (Simplified)"); codes.add("zh-hans");
    }
    //languages.add(TRANS("Chinese (Traditional)")); languagesNative.add(CharPointer_UTF8 ("\xe4\xb8\xad\xe6\x96\x87\xef\xbc\x88\xe7\xb9\x81\xe9\xab\x94\xef\xbc\x89")); codes.add("zh-hant");

    languages.add(TRANS("Russian"));  languagesNative.add(juce::CharPointer_UTF8 ("p\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9")); codes.add("ru");


    // TODO - parse any user files with localized_%s.txt in our settings folder and add as options if not existing already

}


OptionsView::OptionsView(SonobusAudioProcessor& proc, std::function<AudioDeviceManager*()> getaudiodevicemanager)
: Component(), getAudioDeviceManager(getaudiodevicemanager), processor(proc), smallLNF(14), sonoSliderLNF(13)
{
    setColour (nameTextColourId, Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.9f));
    setColour (selectedColourId, Colour::fromFloatRGBA(0.0f, 0.4f, 0.8f, 0.5f));
    setColour (separatorColourId, Colour::fromFloatRGBA(0.3f, 0.3f, 0.3f, 0.3f));

    sonoSliderLNF.textJustification = Justification::centredRight;
    sonoSliderLNF.sliderTextJustification = Justification::centredRight;

    initializeLanguages();

    mRecOptionsComponent = std::make_unique<SonoCraftSettingsPage>();
    mOptionsComponent = std::make_unique<SonoCraftSettingsPage>();

    mSettingsTab = std::make_unique<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
#if JUCE_IOS || JUCE_ANDROID
    mSettingsTab->setTabBarDepth(48);
#else
    mSettingsTab->setTabBarDepth(36);
#endif
    mSettingsTab->setOutline(0);
    mSettingsTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
    //mSettingsTab->addComponentListener(this);

    mBufferTimeSlider    = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight);
    mBufferTimeSlider->setName("time");
    mBufferTimeSlider->setTitle(TRANS("Default Jitter Buffer"));
    mBufferTimeSlider->setSliderSnapsToMousePosition(false);
    mBufferTimeSlider->setChangeNotificationOnlyOnRelease(true);
    mBufferTimeSlider->setDoubleClickReturnValue(true, 15.0);
    mBufferTimeSlider->setTextBoxIsEditable(true);
    mBufferTimeSlider->setScrollWheelEnabled(false);
    mBufferTimeSlider->setColour(Slider::trackColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.3));
    mBufferTimeSlider->setWantsKeyboardFocus(true);

    mBufferTimeSlider->setTooltip(TRANS("This controls controls the default jitter buffer size to start with. When using the Auto modes, it is recommended to keep the value at the minimum so it starts from the lowest possible value. Generally you will only want to set this higher if you are using a Manual mode default which is not recommended."));

    mBufferTimeAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramDefaultNetbufMs, *mBufferTimeSlider);

    mOptionsAutosizeDefaultChoice = std::make_unique<SonoChoiceButton>();
    mOptionsAutosizeDefaultChoice->addChoiceListener(this);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Manual"), SonobusAudioProcessor::AutoNetBufferModeOff);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Auto Up"), SonobusAudioProcessor::AutoNetBufferModeAutoIncreaseOnly);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Auto"), SonobusAudioProcessor::AutoNetBufferModeAutoFull);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Initial Auto"), SonobusAudioProcessor::AutoNetBufferModeInitAuto);

    mOptionsAutosizeDefaultChoice->setTooltip(TRANS("This controls how the jitter buffers are automatically adjusted based on network conditions. The Auto mode is the recommended choice as it will adjust the jitter buffers up or down based on current conditions. The Auto-Up will only make the buffers larger. The Initial Auto will do an initial adjustment from the smallest value and once it stabilizes will no longer change, even if network conditions worsen. Manual will let you set the jitter buffer manually, leaving it up to you to deal with if network conditions change, but can be useful with known users."));

    mOptionsFormatChoiceDefaultChoice = std::make_unique<SonoChoiceButton>();
    mOptionsFormatChoiceDefaultChoice->setTitle(TRANS("Default Send Quality:"));

    mOptionsFormatChoiceDefaultChoice->addChoiceListener(this);
    int numformats = processor.getNumberAudioCodecFormats();
    for (int i=0; i < numformats; ++i) {
        SonobusAudioProcessor::AudioCodecFormatInfo finfo;
        processor.getAudioCodeFormatInfo(i, finfo);
        auto name = finfo.name;
        if (finfo.codec == SonobusAudioProcessor::AudioCodecFormatCodec::CodecOpus && finfo.bitrate < 96000) {
            name += String(" (*)");
        }
        mOptionsFormatChoiceDefaultChoice->addItem(name, i+1);
    }
    mOptionsFormatChoiceDefaultChoice->addItem("(*) " + TRANS("not recommended"), -2, true, true);

    mOptionsFormatChoiceDefaultChoice->setTooltip(TRANS("The default send quality will be used when you first connect with someone. The values specified with a kbps/ch are Opus compressed audio and use less network bandwidth at the expense of a little latency. It is not recommended to use less than 96 kpbs/ch, as that will increase latency more. The PCM 16bit (and above) use uncompressed audio data and will use the most network bandwidth, but have the least latency and CPU load. If you are connecting with a known small group who all have network service that can support it, using PCM 16 bit is recommended for the lowest latency. Otherwise 96 kbps/ch is a good default."));





    mOptionsLanguageChoice = std::make_unique<SonoChoiceButton>();
    mOptionsLanguageChoice->setTitle(TRANS("Language"));
    mOptionsLanguageChoice->addChoiceListener(this);
    auto overridelang = processor.getLanguageOverrideCode();
    int langsel = -1;
    for (int i=0; i < languages.size(); ++i) {
        String itemname;
        if (languagesNative[i] != languages[i] && !languagesNative[i].isEmpty()) {
            itemname = languagesNative[i] + " - " + languages[i];
        }
        else {
            itemname = languages[i];
        }
        mOptionsLanguageChoice->addItem(itemname, i);
        if (overridelang == codes[i]) {
            langsel = i;
        }
    }

    if (langsel >= 0) {
        mOptionsLanguageChoice->setSelectedId(langsel);
    }



    mOptionsUnivFontButton = std::make_unique<ToggleButton>(TRANS("Use Universal Font"));
    mOptionsUnivFontButton->setTooltip(TRANS("Use font that always supports Chinese, Japanese, and Korean characters. Can cause slowdowns on some systems, so only use it if you need it."));
    mOptionsUnivFontButton->setToggleState(processor.getUseUniversalFont(), dontSendNotification);
    mOptionsUnivFontButton->addListener(this);


    //mOptionsHearLatencyButton = std::make_unique<ToggleButton>(TRANS("Make Latency Test Audible"));
    //mOptionsHearLatencyButton->addListener(this);
    //mHearLatencyTestAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramHearLatencyTest, *mOptionsHearLatencyButton);

    mOptionsMetRecordedButton = std::make_unique<ToggleButton>(TRANS("Metronome output recorded in full mix"));
    mOptionsMetRecordedButton->addListener(this);
    mMetRecordedAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramMetIsRecorded, *mOptionsMetRecordedButton);

    mOptionsRecFinishOpenButton = std::make_unique<ToggleButton>(TRANS("Open finished recording for playback"));
    mOptionsRecFinishOpenButton->addListener(this);



    mOptionsRecMixButton = std::make_unique<ToggleButton>(TRANS("Full Mix"));
    mOptionsRecMixButton->addListener(this);

    mOptionsRecMixMinusButton = std::make_unique<ToggleButton>(TRANS("Full Mix without yourself"));
    mOptionsRecMixMinusButton->addListener(this);

    mOptionsRecSelfButton = std::make_unique<ToggleButton>(TRANS("Yourself"));
    mOptionsRecSelfButton->addListener(this);

    mOptionsRecOthersButton = std::make_unique<ToggleButton>(TRANS("Each Connected User"));
    mOptionsRecOthersButton->addListener(this);

    mOptionsRecSelfPostFxButton = std::make_unique<ToggleButton>(TRANS("Record yourself including input FX"));
    mOptionsRecSelfPostFxButton->addListener(this);

    mOptionsRecSelfSilenceMutedButton = std::make_unique<ToggleButton>(TRANS("Silence self recording when input is muted"));
    mOptionsRecSelfSilenceMutedButton->addListener(this);


    mRecFormatChoice = std::make_unique<SonoChoiceButton>();
    mRecFormatChoice->addChoiceListener(this);
    mRecFormatChoice->addItem(TRANS("FLAC"), SonobusAudioProcessor::FileFormatFLAC);
    mRecFormatChoice->addItem(TRANS("WAV"), SonobusAudioProcessor::FileFormatWAV);
    mRecFormatChoice->addItem(TRANS("OGG"), SonobusAudioProcessor::FileFormatOGG);

    mRecBitsChoice = std::make_unique<SonoChoiceButton>();
    mRecBitsChoice->addChoiceListener(this);
    mRecBitsChoice->addItem(TRANS("16 bit"), 16);
    mRecBitsChoice->addItem(TRANS("24 bit"), 24);





    mRecLocationButton = std::make_unique<TextButton>("fileloc");
    mRecLocationButton->setButtonText("");
    mRecLocationButton->setLookAndFeel(&smallLNF);
    mRecLocationButton->addListener(this);




    mOptionsUseSpecificUdpPortButton = std::make_unique<ToggleButton>(TRANS("Use Specific UDP Port"));
    mOptionsUseSpecificUdpPortButton->addListener(this);

    mOptionsDynamicResamplingButton = std::make_unique<ToggleButton>(TRANS("Use Drift Correction (NOT RECOMMENDED)"));
    mDynamicResamplingAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramDynamicResampling, *mOptionsDynamicResamplingButton);

    mOptionsAutoReconnectButton = std::make_unique<ToggleButton>(TRANS("Auto-Reconnect to Last Group"));
    mAutoReconnectAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramAutoReconnectLast, *mOptionsAutoReconnectButton);

    mOptionsOverrideSamplerateButton = std::make_unique<ToggleButton>(TRANS("Override Device Sample Rate"));
    mOptionsOverrideSamplerateButton->addListener(this);


    mOptionsSliderSnapToMouseButton = std::make_unique<ToggleButton>(TRANS("Sliders Snap to Clicked Position"));
    mOptionsSliderSnapToMouseButton->addListener(this);

    mOptionsDisableShortcutButton = std::make_unique<ToggleButton>(TRANS("Disable keyboard shortcuts"));
    mOptionsDisableShortcutButton->addListener(this);

#if JUCE_IOS
    if (JUCEApplicationBase::isStandaloneApp()) {
        mOptionsAllowBluetoothInput = std::make_unique<ToggleButton>(TRANS("Allow Bluetooth Input"));
        mOptionsAllowBluetoothInput->addListener(this);
    }
#endif

    mOptionsInputLimiterButton = std::make_unique<ToggleButton>(TRANS("Use Input FX Limiter"));
    mOptionsInputLimiterButton->addListener(this);

    mOptionsChangeAllFormatButton = std::make_unique<ToggleButton>(TRANS("Change all connected"));
    mOptionsChangeAllFormatButton->addListener(this);
    mOptionsChangeAllFormatButton->setLookAndFeel(&smallLNF);

    mOptionsUdpPortEditor = std::make_unique<TextEditor>("udp");
    mOptionsUdpPortEditor->addListener(this);
    mOptionsUdpPortEditor->setFont(Font(16 * SonoLookAndFeel::getFontScale()));
    mOptionsUdpPortEditor->setText(""); // 100.36.128.246:11000
    mOptionsUdpPortEditor->setKeyboardType(TextEditor::numericKeyboard);
    mOptionsUdpPortEditor->setInputRestrictions(5, "0123456789");

    configEditor(mOptionsUdpPortEditor.get());


    mOptionsDefaultLevelSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxAbove);
    mOptionsDefaultLevelSlider->setName("uservol");
    mOptionsDefaultLevelSlider->setTitle(TRANS("Default User Level"));
    mOptionsDefaultLevelSlider->setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());
    mOptionsDefaultLevelSlider->setScrollWheelEnabled(false);
    configLevelSlider(mOptionsDefaultLevelSlider.get());
    mOptionsDefaultLevelSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mOptionsDefaultLevelSlider->setTextBoxIsEditable(true);
    //mOptionsDefaultLevelSlider->setSliderStyle(Slider::SliderStyle::LinearBar);
    mOptionsDefaultLevelSlider->setWantsKeyboardFocus(true);

    mDefaultLevelAttachment =  std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramDefaultPeerLevel, *mOptionsDefaultLevelSlider);



    auto autodropname = TRANS("Auto Adjust Drop Threshold");
    mOptionsAutoDropThreshSlider = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight);
    mOptionsAutoDropThreshSlider->setTitle(autodropname);
    mOptionsAutoDropThreshSlider->setRange(1.0, 20.0, 0.1);
    mOptionsAutoDropThreshSlider->setSkewFactor(0.5);
    mOptionsAutoDropThreshSlider->setName("dropthresh");
    mOptionsAutoDropThreshSlider->setTextValueSuffix(" " + TRANS("sec"));
    mOptionsAutoDropThreshSlider->setSliderSnapsToMousePosition(false);
    mOptionsAutoDropThreshSlider->setChangeNotificationOnlyOnRelease(true);
    mOptionsAutoDropThreshSlider->setDoubleClickReturnValue(true, 2.0);
    mOptionsAutoDropThreshSlider->setTextBoxIsEditable(false);
    mOptionsAutoDropThreshSlider->setScrollWheelEnabled(false);
    mOptionsAutoDropThreshSlider->setColour(Slider::trackColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.3));
    mOptionsAutoDropThreshSlider->setWantsKeyboardFocus(true);

    mOptionsAutoDropThreshSlider->onValueChange = [this]() {
        auto thresh = 1.0 / jmax(1.0, mOptionsAutoDropThreshSlider->getValue());
        processor.setAutoresizeBufferDropRateThreshold(thresh);
    };

    mOptionsAutoDropThreshSlider->setTooltip(TRANS("This controls how sensitive the auto-jitter buffer adjustment is when there are audio dropouts. The jitter buffer size will be increased if there are any dropouts within the number of seconds specified here. When this value is smaller it will be less likely to increase the jitter buffer size automatically."));



    mOptionsSavePluginDefaultButton = std::make_unique<TextButton>("saveopt");
    mOptionsSavePluginDefaultButton->setButtonText(TRANS("Save as default plugin options"));
    mOptionsSavePluginDefaultButton->setLookAndFeel(&smallLNF);
    mOptionsSavePluginDefaultButton->addListener(this);

    mOptionsResetPluginDefaultButton = std::make_unique<TextButton>("resetopt");
    mOptionsResetPluginDefaultButton->setButtonText(TRANS("Reset default plugin options"));
    mOptionsResetPluginDefaultButton->setLookAndFeel(&smallLNF);
    mOptionsResetPluginDefaultButton->addListener(this);

    

    mVersionLabel = std::make_unique<Label>("", TRANS("Version: ") + ProjectInfo::versionString);
    configLabel(mVersionLabel.get(), true);
    mVersionLabel->setJustificationType(Justification::centredRight);

    addAndMakeVisible(mSettingsTab.get());

    if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager && getAudioDeviceManager())
    {
        if (!mAudioDeviceSelector) {
            int minNumInputs  = std::numeric_limits<int>::max(), maxNumInputs  = 0,
            minNumOutputs = std::numeric_limits<int>::max(), maxNumOutputs = 0;

            auto updateMinAndMax = [] (int newValue, int& minValue, int& maxValue)
            {
                minValue = jmin (minValue, newValue);
                maxValue = jmax (maxValue, newValue);
            };

            /*
            if (channelConfiguration.size() > 0)
            {
                auto defaultConfig =  channelConfiguration.getReference (0);
                updateMinAndMax ((int) defaultConfig.numIns,  minNumInputs,  maxNumInputs);
                updateMinAndMax ((int) defaultConfig.numOuts, minNumOutputs, maxNumOutputs);
            }
             */

            if (auto* bus = processor.getBus (true, 0)) {
                auto maxsup = bus->getMaxSupportedChannels(128);
                updateMinAndMax (maxsup, minNumInputs, maxNumInputs);
                updateMinAndMax (bus->getDefaultLayout().size(), minNumInputs, maxNumInputs);
                if (bus->isNumberOfChannelsSupported(1)) {
                    updateMinAndMax (1, minNumInputs, maxNumInputs);
                }
                if (bus->isNumberOfChannelsSupported(0)) {
                    updateMinAndMax (0, minNumInputs, maxNumInputs);
                }
            }

            if (auto* bus = processor.getBus (false, 0)) {
                auto maxsup = bus->getMaxSupportedChannels(128);
                updateMinAndMax (maxsup, minNumOutputs, maxNumOutputs);
                updateMinAndMax (bus->getDefaultLayout().size(), minNumOutputs, maxNumOutputs);
                if (bus->isNumberOfChannelsSupported(1)) {
                    updateMinAndMax (1, minNumOutputs, maxNumOutputs);
                }
                if (bus->isNumberOfChannelsSupported(0)) {
                    updateMinAndMax (0, minNumOutputs, maxNumOutputs);
                }
            }


            minNumInputs  = jmin (minNumInputs,  maxNumInputs);
            minNumOutputs = jmin (minNumOutputs, maxNumOutputs);



            mAudioDeviceSelector = std::make_unique<AudioDeviceSelectorComponent>(*getAudioDeviceManager(),
                                                                                  minNumInputs, maxNumInputs,
                                                                                  minNumOutputs, maxNumOutputs,
                                                                                  false, // show MIDI input
                                                                                  false,
                                                                                  false, true, [](const String& name) { return !SonoCraftAudioRouting::isVirtualCable(name); });

#if JUCE_IOS || JUCE_ANDROID
            mAudioDeviceSelector->setItemHeight(44);
#endif

        }
    }

    mAudioPage = std::make_unique<SonoCraftSettingsPage>();
    mConnectionPage = std::make_unique<SonoCraftSettingsPage>();
    mListeningControls = std::make_unique<SonoCraftListeningControls>(processor, [this] { return getAudioDeviceManager ? getAudioDeviceManager() : nullptr; });
    mDiagnosticButton.setButtonText(String::fromUTF8("Copiar diagnóstico de áudio"));
    mDiagnosticButton.setTooltip(String::fromUTF8("Copia dispositivos e níveis para o suporte. Não grava áudio nem inclui nomes de abas."));
    mDiagnosticButton.onClick = [this] {
        SystemClipboard::copyTextToClipboard(mListeningControls->audioDiagnostic());
        mDiagnosticButton.setButtonText(String::fromUTF8("Diagnóstico copiado"));
    };
    configurePages();
    auto addPage = [this](std::unique_ptr<Viewport>& viewport, Component& page, const char* name) {
        viewport = std::make_unique<Viewport>();
        viewport->setViewedComponent(&page, false);
        viewport->setScrollBarsShown(true, false);
        mSettingsTab->addTab(String::fromUTF8(name), SonoCraftTheme::canvas(), viewport.get(), false);
    };
    addPage(mAudioOptionsViewport, *mAudioPage, "Áudio");
    addPage(mConnectionViewport, *mConnectionPage, "Conexão");
    addPage(mRecordOptionsViewport, *mRecOptionsComponent, "Gravação");
    addPage(mOtherOptionsViewport, *mOptionsComponent, "Preferências");

    setFocusContainerType(FocusContainerType::keyboardFocusContainer);

    mSettingsTab->setFocusContainerType(FocusContainerType::none);
    mSettingsTab->getTabbedButtonBar().setFocusContainerType(FocusContainerType::none);
    mSettingsTab->getTabbedButtonBar().setWantsKeyboardFocus(true);
    mSettingsTab->setWantsKeyboardFocus(true);
    for (int i=0; i < mSettingsTab->getTabbedButtonBar().getNumTabs(); ++i) {
        if (auto tabbut = mSettingsTab->getTabbedButtonBar().getTabButton(i)) {
            tabbut->setRadioGroupId(3);
            tabbut->setWantsKeyboardFocus(true);
        }
        if (auto tabcomp = mSettingsTab->getTabContentComponent(i)) {
            tabcomp->setFocusContainerType(FocusContainerType::focusContainer);
        }
    }

    updateLayout();
}

OptionsView::~OptionsView() {}

juce::Rectangle<int> OptionsView::getMinimumContentBounds() const {
    int defWidth = 200;
    int defHeight = 100;
    return Rectangle<int>(0,0,defWidth,defHeight);
}

juce::Rectangle<int> OptionsView::getPreferredContentBounds() const
{
    return Rectangle<int> (0, 0, 640, prefHeight);
}


void OptionsView::timerCallback(int timerid)
{

}

void OptionsView::grabInitialFocus()
{
    if (auto * butt = mSettingsTab->getTabbedButtonBar().getTabButton(mSettingsTab->getCurrentTabIndex())) {
        butt->setWantsKeyboardFocus(true);
        butt->grabKeyboardFocus();
    }
}


void OptionsView::configLabel(Label *label, bool val)
{
    if (val) {
        label->setFont(12);
        label->setColour(Label::textColourId, SonoCraftTheme::secondary());
        label->setJustificationType(Justification::centred);
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centredLeft);
    }
}

void OptionsView::configLevelSlider(Slider * slider)
{
    //slider->setVelocityBasedMode(true);
    //slider->setVelocityModeParameters(2.5, 1, 0.05);
    //slider->setTextBoxStyle(Slider::NoTextBox, true, 40, 18);
    slider->setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 50, 14);
    slider->setMouseDragSensitivity(128);
    slider->setScrollWheelEnabled(false);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, SonoCraftTheme::secondary());
    slider->setColour(TooltipWindow::textColourId, SonoCraftTheme::text());

    slider->setLookAndFeel(&sonoSliderLNF);
}

void OptionsView::configEditor(TextEditor *editor, bool passwd)
{
    editor->addListener(this);
    if (passwd)  {
        editor->setIndents(8, 6);
    } else {
        editor->setIndents(8, 8);
    }
}

void OptionsView::updateState(bool ignorecheck)
{
    mOptionsFormatChoiceDefaultChoice->setSelectedItemIndex(processor.getDefaultAudioCodecFormat(), dontSendNotification);
    mOptionsAutosizeDefaultChoice->setSelectedId((int)processor.getDefaultAutoresizeBufferMode(), dontSendNotification);

    mOptionsChangeAllFormatButton->setToggleState(processor.getChangingDefaultAudioCodecSetsExisting(), dontSendNotification);

    mOptionsAutoDropThreshSlider->setValue(1 / jmax(0.001f, processor.getAutoresizeBufferDropRateThreshold()), dontSendNotification);

    int port = processor.getUseSpecificUdpPort();
    if (port > 0) {
        mOptionsUdpPortEditor->setText(String::formatted("%d", port), dontSendNotification);
        mOptionsUdpPortEditor->setEnabled(true);
        mOptionsUdpPortEditor->setAlpha(1.0);
        if (!ignorecheck) {
            mOptionsUseSpecificUdpPortButton->setToggleState(true, dontSendNotification);
        }
    }
    else {
        port = processor.getUdpLocalPort();
        mOptionsUdpPortEditor->setEnabled(mOptionsUseSpecificUdpPortButton->getToggleState());
        mOptionsUdpPortEditor->setAlpha(mOptionsUseSpecificUdpPortButton->getToggleState() ? 1.0 : 0.6);
        mOptionsUdpPortEditor->setText(String::formatted("%d", port), dontSendNotification);
        if (!ignorecheck) {
            mOptionsUseSpecificUdpPortButton->setToggleState(false, dontSendNotification);
        }
    }


    if (JUCEApplication::isStandaloneApp()) {
        if (getShouldOverrideSampleRateValue) {
            Value * val = getShouldOverrideSampleRateValue();
            mOptionsOverrideSamplerateButton->setToggleState((bool)val->getValue(), dontSendNotification);
        }

        if (getAllowBluetoothInputValue && mOptionsAllowBluetoothInput) {
            Value * val = getAllowBluetoothInputValue();
            mOptionsAllowBluetoothInput->setToggleState((bool)val->getValue(), dontSendNotification);
        }

    }

    mOptionsUnivFontButton->setToggleState(processor.getUseUniversalFont(), dontSendNotification);

    mOptionsSliderSnapToMouseButton->setToggleState(processor.getSlidersSnapToMousePosition(), dontSendNotification);
    mOptionsDisableShortcutButton->setToggleState(processor.getDisableKeyboardShortcuts(), dontSendNotification);

    uint32 recmask = processor.getDefaultRecordingOptions();

    mOptionsRecOthersButton->setToggleState((recmask & SonobusAudioProcessor::RecordIndividualUsers) != 0, dontSendNotification);
    mOptionsRecMixButton->setToggleState((recmask & SonobusAudioProcessor::RecordMix) != 0, dontSendNotification);
    mOptionsRecMixMinusButton->setToggleState((recmask & SonobusAudioProcessor::RecordMixMinusSelf) != 0, dontSendNotification);
    mOptionsRecSelfButton->setToggleState((recmask & SonobusAudioProcessor::RecordSelf) != 0, dontSendNotification);

    mOptionsRecSelfPostFxButton->setToggleState(!processor.getSelfRecordingPreFX(), dontSendNotification);
    mOptionsRecSelfSilenceMutedButton->setToggleState(processor.getSelfRecordingSilenceWhenMuted(), dontSendNotification);

    mOptionsRecFinishOpenButton->setToggleState(processor.getRecordFinishOpens(), dontSendNotification);

    mRecFormatChoice->setSelectedId((int)processor.getDefaultRecordingFormat(), dontSendNotification);
    mRecBitsChoice->setSelectedId((int)processor.getDefaultRecordingBitsPerSample(), dontSendNotification);

    auto recdirurl = processor.getDefaultRecordingDirectory();
    String dispath = recdirurl.getFileName();
    if (recdirurl.isLocalFile()) {
        File recdir = recdirurl.getLocalFile();
        dispath = recdir.getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
        if (dispath.startsWith(".")) dispath = recdir.getFullPathName();
    }
    mRecLocationButton->setButtonText(dispath);

    CompressorParams limparams;
    processor.getInputLimiterParams(0, limparams);
    mOptionsInputLimiterButton->setToggleState(limparams.enabled, dontSendNotification);

    if (getShouldOverrideSampleRateValue) {
        Value * val = getShouldOverrideSampleRateValue();
        mOptionsOverrideSamplerateButton->setToggleState((bool)val->getValue(), dontSendNotification);
    }
    if (mOptionsAllowBluetoothInput && getAllowBluetoothInputValue) {
        Value * val = getAllowBluetoothInputValue();
        mOptionsAllowBluetoothInput->setToggleState((bool)val->getValue(), dontSendNotification);
    }
}


void OptionsView::configurePages()
{
    const auto text = [](const char* value) { return String::fromUTF8(value); };
    const auto button = [&](Button& control, const char* name, const char* help = "") {
        control.setButtonText(text(name)); control.setTooltip(text(help));
    };
    const auto slider = [&](Slider& control, const char* name) {
        control.setTitle(text(name)); control.setSliderStyle(Slider::LinearHorizontal);
        control.setTextBoxStyle(Slider::TextBoxRight, false, 90, 28);
    };
    // Controls retain their existing parameter attachments and saved values.
    button(*mOptionsInputLimiterButton, "Proteger a entrada contra picos de volume", "Ativa o limitador da entrada. Ajuda a evitar distorção quando o som fica alto.");
    button(*mOptionsOverrideSamplerateButton, "Restaurar a taxa de amostragem ao abrir", "Usa a taxa salva pelo SonoCraft ao reabrir o dispositivo. Desligado: usa a taxa escolhida pelo sistema.");
    button(*mOptionsAutoReconnectButton, "Reconectar ao último grupo ao abrir");
    button(*mOptionsChangeAllFormatButton, "Aplicar a qualidade também às conexões atuais", "Desligado: a nova qualidade vale apenas para as próximas conexões.");
    button(*mOptionsUseSpecificUdpPortButton, "Usar uma porta de rede fixa (UDP)", "Normalmente o sistema escolhe a porta. Defina uma porta fixa somente se sua rede exigir.");
    mOptionsUdpPortEditor->setTitle(text("Porta de rede (UDP)"));
    button(*mOptionsDynamicResamplingButton, "Correção de relógio experimental", "Reamostra o áudio para compensar diferenças entre dispositivos. Mantenha desligada se não precisar desse ajuste.");
    button(*mOptionsSliderSnapToMouseButton, "Mover o volume para a posição clicada");
    button(*mOptionsDisableShortcutButton, "Desativar atalhos de teclado");
    button(*mOptionsUnivFontButton, "Usar fonte para idiomas asiáticos", "Melhora a exibição de chinês, japonês e coreano. A alteração exige reiniciar.");
    button(*mOptionsSavePluginDefaultButton, "Salvar como padrão do plug-in");
    button(*mOptionsResetPluginDefaultButton, "Restaurar o padrão do plug-in");
    if (mOptionsAllowBluetoothInput) button(*mOptionsAllowBluetoothInput, "Permitir microfone Bluetooth");
    mOptionsLanguageChoice->setTitle("Idioma");
    mOptionsFormatChoiceDefaultChoice->setTitle(text("Qualidade do áudio enviado"));
    mOptionsFormatChoiceDefaultChoice->setTooltip(text("Opus usa menos rede; PCM preserva o áudio sem compressão e usa mais rede. A qualidade é definida por canal."));
    mOptionsAutosizeDefaultChoice->clearItems();
    mOptionsAutosizeDefaultChoice->addItem("Manual", SonobusAudioProcessor::AutoNetBufferModeOff);
    mOptionsAutosizeDefaultChoice->addItem(text("Automático: só aumentar"), SonobusAudioProcessor::AutoNetBufferModeAutoIncreaseOnly);
    mOptionsAutosizeDefaultChoice->addItem(text("Automático (recomendado)"), SonobusAudioProcessor::AutoNetBufferModeAutoFull);
    mOptionsAutosizeDefaultChoice->addItem(text("Automático só ao conectar"), SonobusAudioProcessor::AutoNetBufferModeInitAuto);
    mOptionsAutosizeDefaultChoice->setTitle(text("Ajuste da reserva de áudio"));
    mOptionsAutosizeDefaultChoice->setTooltip(text("Automático adapta a reserva às falhas da rede. Manual mantém o tempo fixo. Só aumentar prioriza estabilidade; só ao conectar faz um ajuste inicial."));
    slider(*mBufferTimeSlider, "Reserva inicial de áudio (ms)");
    mBufferTimeSlider->setTooltip(text("Mais tempo ajuda a evitar cortes, mas aumenta o atraso. No modo automático, comece com um valor baixo."));
    slider(*mOptionsAutoDropThreshSlider, "Intervalo de análise de falhas (s)");
    mOptionsAutoDropThreshSlider->setTextValueSuffix(" s");
    mOptionsAutoDropThreshSlider->setTooltip(text("Se ocorrerem cortes neste intervalo, a reserva automática pode aumentar. Um intervalo maior torna o ajuste mais sensível."));
    slider(*mOptionsDefaultLevelSlider, "Volume inicial dos participantes (dB)");
    mOptionsDefaultLevelSlider->setTooltip(text("Volume aplicado a novas conexões. 0 dB mantém o nível original."));

    auto& audio = *mAudioPage;
    audio.heading("Entrada e saída principal");
    if (mAudioDeviceSelector) {
        audio.help("Entrada é o microfone ou a captura que você compartilha. Saída principal é onde você ouve os participantes e os pads. Escolher Nenhum desativa a saída principal. Você ainda pode transmitir e ouvir pela segunda saída. As alterações são aplicadas na hora.");
        audio.control(*mAudioDeviceSelector, 0);
        audio.help("Opções avançadas do dispositivo ajustam a taxa de amostragem e o tempo de processamento. Um tempo maior pode reduzir cortes, mas aumenta o atraso.");
        audio.control(*mOptionsOverrideSamplerateButton);
        if (mOptionsAllowBluetoothInput) audio.control(*mOptionsAllowBluetoothInput);
    } else audio.help("No plug-in, a entrada e a saída são escolhidas no programa que hospeda o SonoCraft.");
    audio.control(*mOptionsInputLimiterButton);
    if (JUCEApplicationBase::isStandaloneApp()) {
        audio.heading("Segunda saída · opcional");
        audio.control(*mListeningControls, 184);
    }
    audio.help("Para silenciar sua entrada ou um participante, use os controles da mesa. O envio de cada aplicativo ou aba também é controlado na mesa.");

    auto& network = *mConnectionPage;
    network.heading("Conexão e envio");
    network.help("Estas opções controlam o áudio entre participantes do SonoCraft. Para entrar em um grupo ou conectar por IP, use Conexões na mesa.");
    network.control(*mOptionsAutoReconnectButton);
    network.help("Qualidade do áudio enviado");
    network.control(*mOptionsFormatChoiceDefaultChoice);
    network.control(*mOptionsChangeAllFormatButton);
    network.heading("Estabilidade e atraso");
    network.help("A reserva de áudio absorve variações da rede. Aumentá-la pode evitar cortes, com um pouco mais de atraso. O modo automático é recomendado.");
    network.control(*mOptionsAutosizeDefaultChoice);
    network.help("Reserva inicial, em milissegundos"); network.control(*mBufferTimeSlider);
    network.help("Intervalo para detectar cortes, em segundos"); network.control(*mOptionsAutoDropThreshSlider);
    network.help("Volume inicial de novos participantes"); network.control(*mOptionsDefaultLevelSlider);
    network.heading("Rede avançada");
    network.help("Mantenha os valores atuais se a conexão estiver funcionando. Estes ajustes não são necessários na maioria das redes.");
    network.control(*mOptionsUseSpecificUdpPortButton); network.control(*mOptionsUdpPortEditor);
    network.control(*mOptionsDynamicResamplingButton);

    button(*mOptionsRecMixButton, "Mix completo (todos juntos)");
    button(*mOptionsRecMixMinusButton, "Mix sem minha entrada");
    button(*mOptionsRecSelfButton, "Minha entrada em arquivo separado");
    button(*mOptionsRecOthersButton, "Um arquivo para cada participante");
    button(*mOptionsMetRecordedButton, "Incluir metrônomo no mix completo");
    button(*mOptionsRecSelfPostFxButton, "Incluir os efeitos da minha entrada");
    button(*mOptionsRecSelfSilenceMutedButton, "Respeitar o botão Silenciar ao gravar minha entrada");
    button(*mOptionsRecFinishOpenButton, "Carregar a gravação no player ao terminar", "Abre o arquivo para reprodução; não começa a tocá-lo automaticamente.");
    mRecFormatChoice->setTitle(text("Formato do arquivo")); mRecBitsChoice->setTitle(text("Resolução da gravação"));
    mRecLocationButton->setTitle(text("Pasta das gravações"));
    auto& recording = *mRecOptionsComponent;
    recording.heading("Arquivos de gravação");
    recording.help("Configure o que será salvo. Para iniciar ou parar, use Ferramentas → Gravação na mesa.");
#if !JUCE_IOS
    recording.help("Pasta onde salvar · clique para escolher"); recording.control(*mRecLocationButton);
#endif
    recording.help("Formato do arquivo · WAV sem compressão, FLAC sem perda, OGG compacto");
    recording.control(*mRecFormatChoice); recording.control(*mRecBitsChoice);
    recording.heading("O que gravar");
    recording.help("Você pode escolher mais de uma opção. Cada opção marcada gera seus próprios arquivos.");
    for (auto* control : { mOptionsRecMixButton.get(), mOptionsRecMixMinusButton.get(), mOptionsRecSelfButton.get(), mOptionsRecOthersButton.get() }) recording.control(*control);
    recording.heading("Conteúdo e finalização");
    for (auto* control : { mOptionsMetRecordedButton.get(), mOptionsRecSelfPostFxButton.get(), mOptionsRecSelfSilenceMutedButton.get(), mOptionsRecFinishOpenButton.get() }) recording.control(*control);

    auto& prefs = *mOptionsComponent;
    prefs.heading("Idioma e aparência");
    prefs.control(*mOptionsLanguageChoice); prefs.control(*mOptionsUnivFontButton);
    prefs.heading("Controles da mesa");
    prefs.control(*mOptionsSliderSnapToMouseButton); prefs.control(*mOptionsDisableShortcutButton);
    if (!JUCEApplicationBase::isStandaloneApp()) {
        prefs.heading("Padrões do plug-in"); prefs.control(*mOptionsSavePluginDefaultButton); prefs.control(*mOptionsResetPluginDefaultButton);
    }
    prefs.heading("Sobre e suporte");
    mVersionLabel->setText("SonoCraft " + String(ProjectInfo::versionString), dontSendNotification);
    mVersionLabel->setJustificationType(Justification::centredLeft);
    prefs.control(*mVersionLabel);
    prefs.help("O diagnóstico ajuda a identificar problemas com dispositivos, captura e conexões. Copiá-lo não envia nada automaticamente.");
    prefs.control(mDiagnosticButton);
}

void OptionsView::updateLayout() { prefHeight = 640; }

void OptionsView::resized() {
    mSettingsTab->setBounds(getLocalBounds().reduced(8));
    for (auto* page : { mAudioPage.get(), mConnectionPage.get(), mOptionsComponent.get(), mRecOptionsComponent.get() }) {
        page->setSize(jmax(1, mSettingsTab->getWidth() - 16), page->getHeight());
        page->resized();
    }
}

void OptionsView::showAudioTab() { mSettingsTab->setCurrentTabIndex(0); }
void OptionsView::showListeningTab() { showAudioTab(); }
void OptionsView::showOptionsTab() { mSettingsTab->setCurrentTabIndex(3); }
void OptionsView::showRecordingTab() { mSettingsTab->setCurrentTabIndex(2); }
void OptionsView::optionsTabChanged(int) {}
void OptionsView::showWarnings() {} // Device backends are a choice, not a prerequisite.

void OptionsView::textEditorReturnKeyPressed (TextEditor& ed)
{
    DBG("Return pressed");
    if (&ed == mOptionsUdpPortEditor.get()) {
        int port = mOptionsUdpPortEditor->getText().getIntValue();
        changeUdpPort(port);
    }
}

void OptionsView::textEditorEscapeKeyPressed (TextEditor& ed)
{
    DBG("escape pressed");
}

void OptionsView::textEditorTextChanged (TextEditor& ed)
{

}


void OptionsView::textEditorFocusLost (TextEditor& ed)
{
    // only one we care about live is udp port
    if (&ed == mOptionsUdpPortEditor.get()) {
        int port = mOptionsUdpPortEditor->getText().getIntValue();
        changeUdpPort(port);
    }
}

void OptionsView::changeUdpPort(int port)
{
    if (port >= 0) {
        DBG("changing udp port to: " << port);
        processor.setUseSpecificUdpPort(port);

        //updateState();
    }
    updateState(true);

}

void OptionsView::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mRecLocationButton.get()) {
        // browse folder chooser
        SafePointer<OptionsView> safeThis (this);

#if JUCE_ANDROID
        if (getAndroidSDKVersion() < 29) {
            if (! RuntimePermissions::isGranted (RuntimePermissions::readExternalStorage))
            {
                RuntimePermissions::request (RuntimePermissions::readExternalStorage,
                                             [safeThis] (bool granted) mutable
                                             {
                    if (granted)
                        safeThis->buttonClicked (safeThis->mRecLocationButton.get());
                });
                return;
            }
        }
#endif
        
         
        chooseRecDirBrowser();
    }
    else if (buttonThatWasClicked == mOptionsInputLimiterButton.get()) {
        CompressorParams params;
        for (int j=0; j < processor.getInputGroupCount(); ++j) {
            processor.getInputLimiterParams(j, params);
            params.enabled = mOptionsInputLimiterButton->getToggleState();
            processor.setInputLimiterParams(j, params);
        }
    }
    else if (buttonThatWasClicked == mOptionsRecMixButton.get()
             || buttonThatWasClicked == mOptionsRecSelfButton.get()
             || buttonThatWasClicked == mOptionsRecOthersButton.get()
             || buttonThatWasClicked == mOptionsRecMixMinusButton.get()
             ) {
        uint32 recmask = 0;
        recmask |= (mOptionsRecMixButton->getToggleState() ? SonobusAudioProcessor::RecordMix : 0);
        recmask |= (mOptionsRecOthersButton->getToggleState() ? SonobusAudioProcessor::RecordIndividualUsers : 0);
        recmask |= (mOptionsRecSelfButton->getToggleState() ? SonobusAudioProcessor::RecordSelf : 0);
        recmask |= (mOptionsRecMixMinusButton->getToggleState() ? SonobusAudioProcessor::RecordMixMinusSelf : 0);

        // ensure at least one is selected
        if (recmask == 0) {
            recmask = SonobusAudioProcessor::RecordMix;
            mOptionsRecMixButton->setToggleState(true, dontSendNotification);
        }

        processor.setDefaultRecordingOptions(recmask);
    }
    else if (buttonThatWasClicked == mOptionsChangeAllFormatButton.get()) {
        processor.setChangingDefaultAudioCodecSetsExisting(mOptionsChangeAllFormatButton->getToggleState());
    }
    else if (buttonThatWasClicked == mOptionsRecSelfPostFxButton.get()) {
        processor.setSelfRecordingPreFX(!mOptionsRecSelfPostFxButton->getToggleState());
    }
    else if (buttonThatWasClicked == mOptionsRecSelfSilenceMutedButton.get()) {
        processor.setSelfRecordingSilenceWhenMuted(mOptionsRecSelfSilenceMutedButton->getToggleState());
    }
    else if (buttonThatWasClicked == mOptionsRecFinishOpenButton.get()) {
        processor.setRecordFinishOpens(mOptionsRecFinishOpenButton->getToggleState());
    }
    else if (buttonThatWasClicked == mOptionsUseSpecificUdpPortButton.get()) {
        if (!mOptionsUseSpecificUdpPortButton->getToggleState()) {
            // toggled off, change back to use system chosen port
            changeUdpPort(0);
        } else {
            updateState(true);
        }
    }
    else if (buttonThatWasClicked == mOptionsOverrideSamplerateButton.get()) {

        if (JUCEApplicationBase::isStandaloneApp() && getShouldOverrideSampleRateValue) {
            Value * val = getShouldOverrideSampleRateValue();
            val->setValue((bool)mOptionsOverrideSamplerateButton->getToggleState());
        }
    }
    else if (buttonThatWasClicked == mOptionsAllowBluetoothInput.get()) {

        if (JUCEApplicationBase::isStandaloneApp() && getAllowBluetoothInputValue && getAudioDeviceManager) {
            Value * val = getAllowBluetoothInputValue();
            val->setValue((bool)mOptionsAllowBluetoothInput->getToggleState());
#if JUCE_IOS
            // get current audio device and change a setting if necessary
            if (auto device = dynamic_cast<iOSAudioIODevice*> (getAudioDeviceManager()->getCurrentAudioDevice())) {
                device->setAllowBluetoothInput((bool)val->getValue());
            }
#endif
        }
    }
    else if (buttonThatWasClicked == mOptionsSliderSnapToMouseButton.get()) {
        bool newval = mOptionsSliderSnapToMouseButton->getToggleState();
        processor.setSlidersSnapToMousePosition(newval);

        mOptionsDefaultLevelSlider->setSliderSnapsToMousePosition(newval);
        
        if (updateSliderSnap) {
            updateSliderSnap();
        }
    }
    else if (buttonThatWasClicked == mOptionsDisableShortcutButton.get()) {
        bool newval = mOptionsDisableShortcutButton->getToggleState();
        processor.setDisableKeyboardShortcuts(newval);
        if (updateKeybindings) {
            updateKeybindings();
        }
    }
    else if (buttonThatWasClicked == mOptionsSavePluginDefaultButton.get()) {
        processor.saveCurrentAsDefaultPluginSettings();
    }
    else if (buttonThatWasClicked == mOptionsResetPluginDefaultButton.get()) {
        processor.resetDefaultPluginSettings();
    }

    else if (buttonThatWasClicked == mOptionsUnivFontButton.get()) {
        bool newval = mOptionsUnivFontButton->getToggleState();
        String message;
        String title;
        if (JUCEApplication::isStandaloneApp()) {
            message = TRANS("In order to change the universal font option, the application must be closed and restarted by you.");
            title = TRANS("App restart required");
        } else {
            message = TRANS("In order to change the universal font option, the plugin host must close the plugin view and reopen it.");
            title = TRANS("Host session reload required");
        }

        AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                     title,
                                     message,
                                     TRANS("Change and Close"),
                                     TRANS("Cancel"),
                                     this,
                                     ModalCallbackFunction::create( [this,newval](int result) {
            if (result) {

                processor.setUseUniversalFont(newval);

                if (JUCEApplication::isStandaloneApp()) {
                    if (saveSettingsIfNeeded) {
                        saveSettingsIfNeeded();
                    }
                    Timer::callAfterDelay(500, [] {
                        JUCEApplication::getInstance()->quit();
                    });
                }
            }
            else {
                mOptionsUnivFontButton->setToggleState(!newval, dontSendNotification);
            }
        }));


    }

}


void OptionsView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    if (comp == mOptionsFormatChoiceDefaultChoice.get()) {
        processor.setDefaultAudioCodecFormat(index);
    }
    else if (comp == mOptionsAutosizeDefaultChoice.get()) {
        processor.setDefaultAutoresizeBufferMode((SonobusAudioProcessor::AutoNetBufferMode) ident);
    }
    else if (comp == mRecFormatChoice.get()) {
        processor.setDefaultRecordingFormat((SonobusAudioProcessor::RecordFileFormat) ident);
    }
    else if (comp == mRecBitsChoice.get()) {
        processor.setDefaultRecordingBitsPerSample(ident);
    }
    else if (comp == mOptionsLanguageChoice.get()) {
        String code = codes[ident];
        //app->mainConfig.languageOverrideCode =  codes[comp->getRowId()].toStdString();

        String message;
        String title;
        if (JUCEApplication::isStandaloneApp()) {
            message = TRANS("In order to change the language, the application must be closed and restarted by you.");
            title = TRANS("App restart required");
        } else {
            message = TRANS("In order to change the language, the plugin host must close the plugin view and reopen it.");
            title = TRANS("Host session reload required");
        }

        AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                     title,
                                     message,
                                     TRANS("Change and Close"),
                                     TRANS("Cancel"),
                                     this,
                                     ModalCallbackFunction::create( [this,code](int result) {
            if (result) {
                String langOverride = code;
                if (setupLocalisation == nullptr || !setupLocalisation(langOverride)) {
                    DBG("Error overriding language with " << langOverride);

                    langOverride = "";
                }

                processor.setLanguageOverrideCode(langOverride);

                if (JUCEApplication::isStandaloneApp()) {
                    if (saveSettingsIfNeeded) {
                        saveSettingsIfNeeded();
                    }
                    Timer::callAfterDelay(500, [] {
                        JUCEApplication::getInstance()->quit();
                    });
                }
            }
        }));
    }

}

void OptionsView::chooseRecDirBrowser()
{
    SafePointer<OptionsView> safeThis (this);

    File recdir;
    if (processor.getDefaultRecordingDirectory().isLocalFile()) {
        recdir = processor.getDefaultRecordingDirectory().getLocalFile();
    }

    mFileChooser.reset(new FileChooser(TRANS("Choose the folder for new recordings"),
                                       recdir,
                                       "",
                                       true, false, getTopLevelComponent()));


    int modes = FileBrowserComponent::canSelectDirectories | FileBrowserComponent::openMode;
    mFileChooser->launchAsync (modes,
                               [safeThis] (const FileChooser& chooser) mutable
                               {
        auto results = chooser.getURLResults();
        if (safeThis != nullptr && results.size() > 0)
        {
            auto url = results.getReference (0);

            DBG("Chose directory: " <<  url.toString(false));

#if JUCE_ANDROID
            auto docdir = AndroidDocument::fromTree(url);
            if (!docdir.hasValue()) {
                docdir = AndroidDocument::fromFile(url.getLocalFile());
            }
            
            if (docdir.hasValue()) {
                AndroidDocumentPermission::takePersistentReadWriteAccess(url);
                if (docdir.getInfo().isDirectory()) {
                    safeThis->processor.setDefaultRecordingDirectory(url);
                }
            }
#else
            if (url.isLocalFile()) {
                File lfile = url.getLocalFile();
                if (lfile.isDirectory()) {
                    safeThis->processor.setDefaultRecordingDirectory(url);
                } else {
                    auto parurl = URL(lfile.getParentDirectory());
                    safeThis->processor.setDefaultRecordingDirectory(parurl);
                }

            }
#endif

            safeThis->updateState();

        }

        if (safeThis) {
            safeThis->mFileChooser.reset();
        }

    }, nullptr);

}



void OptionsView::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{
    popTip.reset(new BubbleMessageComponent());
    popTip->setAllowedPlacement(BubbleComponent::above);
    
    if (target) {
        if (auto * parent = target->findParentComponentOfClass<AudioProcessorEditor>()) {
            parent->addChildComponent (popTip.get());
        } else {
            addChildComponent(popTip.get());            
        }
    }
    else {
        addChildComponent(popTip.get());
    }
    
    AttributedString text(message);
    text.setJustification (Justification::centred);
    text.setColour (findColour (TextButton::textColourOffId));
    text.setFont(Font(12 * SonoLookAndFeel::getFontScale()));
    if (target) {
        popTip->showAt(target, text, timeoutMs);
    }
    else {
        Rectangle<int> topbox(getWidth()/2 - maxwidth/2, 0, maxwidth, 2);
        popTip->showAt(topbox, text, timeoutMs);
    }
    popTip->toFront(false);
    //AccessibilityHandler::postAnnouncement(message, AccessibilityHandler::AnnouncementPriority::high);
}

void OptionsView::paint(Graphics & g)
{
    /*
    //g.fillAll (Colours::black);
    Rectangle<int> bounds = getLocalBounds();

    bounds.reduce(1, 1);
    bounds.removeFromLeft(3);
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(outlineColor);
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 0.5f);
*/
}

