// Modal lifecycle coverage: preserve borrowed views and edits, and dismiss only
// the topmost sheet. No audio hardware, network connection or visible window.
#include "../Source/SonoCallOutBox.h"
#include "../Source/SonoLookAndFeel.h"
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
int main() {
    juce::ScopedJuceInitialiser_GUI gui;
    DialogChecks checks;
    juce::MessageManager::getInstance()->runDispatchLoop();
    if (!checks.failures) std::cout << "Dialog lifecycle checks passed\n";
    return checks.failures ? 1 : 0;
}
