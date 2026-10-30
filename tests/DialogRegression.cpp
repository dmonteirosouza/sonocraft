// Modal lifecycle coverage: preserve borrowed views and edits, and dismiss only
// the topmost sheet. No audio hardware, network connection or visible window.
#include "../Source/SonoCallOutBox.h"
#include "../Source/SonoLookAndFeel.h"
#include "../Source/SonoCraftMixerView.h"
#include "../Source/ChatView.h"
#include <iostream>

class DialogChecks final : private juce::Timer {
public:
    DialogChecks() {
        desktop.setSize(1024, 800); desktop.setLookAndFeel(&look);
        desktop.addChildComponent(editor);
        editor.setText("Texto preservado");
        startTimer(50);
    }
    ~DialogChecks() { desktop.setLookAndFeel(nullptr); }
    int failures = 0;
private:
    SonoLookAndFeel look;
    juce::Component desktop;
    juce::TextEditor editor;
    juce::Component::SafePointer<SonoCallOutBox> panel, nested;
    int stage = 0;
    void check(bool ok, const char* message) {
        if (!ok) { ++failures; std::cerr << message << '\n'; }
    }
    void timerCallback() override {
        switch (stage++) {
        case 0:
            panel = &SonoCallOutBox::launchForComponent(editor, desktop, 600, 480);
            check(editor.isVisible(), "Borrowed content was not shown");
            check(panel->getPanelContent().getWidth() < desktop.getWidth(), "Desktop sheet should be inset");
            editor.setText("Edicao mantida");
            // Equivalent to a persistent panel's own close action.
            editor.setVisible(false);
            break;
        case 1:
            check(panel == nullptr, "Dismissed sheet was not released");
            check(editor.getParentComponent() == &desktop, "Borrowed view was not returned to its owner");
            check(!editor.isVisible(), "Returned view should be hidden");
            panel = &SonoCallOutBox::launchForComponent(editor, desktop, 600, 480);
            check(editor.getText() == "Edicao mantida", "Reopening lost edited state");
            {
                auto content = std::make_unique<juce::Component>();
                content->setName("Editor interno"); content->setSize(320, 200);
                nested = &SonoCallOutBox::launchAsynchronously(std::move(content), {}, &desktop, false);
                check(juce::Component::getCurrentlyModalComponent() == nested.getComponent(), "Nested sheet is not on top");
                nested->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
            }
            break;
        case 2:
            check(nested == nullptr && panel != nullptr, "Closing nested sheet also closed its parent");
            desktop.setSize(360, 640);
            check(panel && panel->getBounds() == desktop.getLocalBounds(), "Dialog did not follow parent resize");
            check(panel && panel->getLocalBounds().contains(panel->getLocalArea(&editor, editor.getLocalBounds())), "Content escaped the resized sheet");
            panel->dismiss();
            break;
        case 3:
            check(panel == nullptr, "Explicit close did not release the sheet");
            check(editor.getParentComponent() == &desktop, "Explicit close lost original parent");
            // Exercise closing an owning window while a borrowed view is open.
            panel = &SonoCallOutBox::launchForComponent(editor, desktop, 600, 480);
            desktop.removeChildComponent(panel.getComponent());
            break;
        default:
            check(panel == nullptr, "Detached sheet retained modal state");
            stopTimer();
            juce::MessageManager::getInstance()->stopDispatchLoop();
            break;
        }
    }
};
static juce::Component* findComponent(juce::Component& root, const juce::String& id) {
    if (root.getComponentID() == id) return &root;
    for (auto* child : root.getChildren()) if (auto* found = findComponent(*child, id)) return found;
    return nullptr;
}
static juce::Button* findButton(juce::Component& root, const juce::String& text) {
    if (auto* button = dynamic_cast<juce::Button*>(&root)) if (button->getButtonText() == text) return button;
    for (auto* child : root.getChildren()) if (auto* found = findButton(*child, text)) return found;
    return nullptr;
}
static bool checkDesktopDock() {
    auto processor = std::make_unique<SonobusAudioProcessor>();
    SonoLookAndFeel look;
    juce::TextEditor conversation, pads;
    conversation.setText("Texto preservado");
    SonoCraftMixerView mixer(*processor, false);
    mixer.setLookAndFeel(&look);
    bool ok = true;
    for (const auto size : {juce::Point<int>(820,580), juce::Point<int>(1040,680), juce::Point<int>(1440,900)}) {
        mixer.setSize(size.x,size.y);
        auto* channels = findComponent(mixer,"mixer.channels");
        const auto fullArea = channels->getBounds();
        mixer.showDesktopPanel(conversation);
        ok &= mixer.isEnabled() && conversation.getParentComponent()==&mixer && conversation.isVisible();
        ok &= juce::Component::getCurrentlyModalComponent()==nullptr;
        ok &= channels->getRight() <= conversation.getX() && channels->getY()==fullArea.getY();
        ok &= mixer.getLocalBounds().contains(conversation.getBounds());
        mixer.showDesktopPanel(pads);
        ok &= !conversation.isVisible() && pads.isVisible();
        mixer.hideDesktopPanel(pads);
        ok &= channels->getBounds()==fullArea && conversation.getText()=="Texto preservado";
        mixer.showDesktopPanel(conversation);
        for (const auto text : {"Arquivo", "Gravação", "Metrônomo"}) {
            auto* tab = findButton(mixer,juce::String::fromUTF8(text));
            if (!tab || !tab->onClick) return false;
            tab->onClick();
            ok &= !conversation.isVisible() && channels->getY()==fullArea.getY();
            auto* tools = dynamic_cast<juce::Viewport*>(findComponent(mixer,"tools.viewport"));
            ok &= tools && tools->isVisible() && channels->getRight() < tools->getX();
            if (tools) for (auto* control : tools->getViewedComponent()->getChildren())
                if (control->isVisible()) ok &= tools->getViewedComponent()->getLocalBounds().contains(control->getBounds());
            ok &= juce::Component::getCurrentlyModalComponent()==nullptr;
        }
        const auto preview = juce::SystemStats::getEnvironmentVariable("SONOCRAFT_DOCK_PREVIEW",{});
        if (preview.isNotEmpty() && size.x==1040) {
            juce::FileOutputStream out{juce::File(preview)};
            out.setPosition(0); out.truncate();
            ok &= out.openedOk() && juce::PNGImageFormat().writeImageToStream(mixer.createComponentSnapshot(mixer.getLocalBounds()),out);
        }
        auto* close = dynamic_cast<juce::Button*>(findComponent(mixer,"tools.close"));
        if (!close || !close->onClick) return false;
        close->onClick();
        ok &= channels->getBounds()==fullArea;
        if (preview.isNotEmpty() && size.x==1040) {
            AooServerConnectionInfo connection;
            ChatView chat(*processor, connection);
            mixer.showDesktopPanel(chat);
            juce::FileOutputStream out{juce::File(preview).getSiblingFile("desktop-chat-preview.png")};
            out.setPosition(0); out.truncate();
            ok &= out.openedOk() && juce::PNGImageFormat().writeImageToStream(mixer.createComponentSnapshot(mixer.getLocalBounds()),out);
            mixer.hideDesktopPanel(chat);
        }
    }
    mixer.setLookAndFeel(nullptr);
    std::cout << (ok ? "Desktop dock geometry, non-modal navigation and tool scrolling passed\n" : "Desktop dock regression failed\n");
    return ok;
}
int main() {
    juce::ScopedJuceInitialiser_GUI gui;
    DialogChecks checks;
    juce::MessageManager::getInstance()->runDispatchLoop();
    if (!checks.failures) std::cout << "Dialog lifecycle checks passed\n";
    return checks.failures || !checkDesktopDock() ? 1 : 0;
}
