

#include "SonoCallOutBox.h"
#include "SonoCraftTheme.h"


SonoCallOutBox::SonoCallOutBox (Component& contentComponent,
                                Rectangle<int> areaToPointTo,
                                Component* parentComponent,
                                std::function<bool(const Component*)> canPassthroughFunc)
: CallOutBox(contentComponent, areaToPointTo, parentComponent), canPassthrough(canPassthroughFunc),
  panelContent(contentComponent), preferredWidth(contentComponent.getWidth()), preferredHeight(contentComponent.getHeight())
{
    ownHeader = bool(panelContent.getProperties()["dialogOwnHeader"]);
    body.setViewedComponent(&panelContent, false);
    body.setScrollBarsShown(true, true);
    addAndMakeVisible(body);
    close.configureAsCloseControl();
    close.onClick = [this] { dismiss(); };
    addChildComponent(close); close.setVisible(!ownHeader);
    heading.setText(panelContent.getName().isNotEmpty() ? panelContent.getName() : TRANS("Options"), dontSendNotification);
    heading.setFont(Font(17, Font::bold));
    heading.setColour(Label::textColourId, SonoCraftTheme::text());
    heading.setJustificationType(Justification::centredLeft);
    addChildComponent(heading); heading.setVisible(!ownHeader);
    panelContent.addComponentListener(this);
    setDismissalMouseClicksAreAlwaysConsumed(true);
    parentSizeChanged();
}

SonoCallOutBox::~SonoCallOutBox()
{
    panelContent.removeComponentListener(this);
    body.setViewedComponent(nullptr, false);
}

void SonoCallOutBox::parentSizeChanged()
{
    if (auto* parent = getParentComponent()) setBounds(parent->getLocalBounds());
    resized();
}

void SonoCallOutBox::parentHierarchyChanged()
{
    if (getParentComponent() == nullptr && isCurrentlyModal()) dismiss();
}

void SonoCallOutBox::resized()
{
    if (layingOut) return;
    const ScopedValueSetter<bool> guard(layingOut, true);
#if JUCE_IOS || JUCE_ANDROID
    panelBounds = getLocalBounds().reduced(12);
    constexpr int headerHeight = 48;
#else
    auto available = getLocalBounds().reduced(24);
    panelBounds = available.withSizeKeepingCentre(jmin(available.getWidth(), jmax(360, preferredWidth + 24)),
                                                  jmin(available.getHeight(), jmax(180, preferredHeight + (ownHeader ? 24 : 68))));
    constexpr int headerHeight = 44;
#endif
    auto area = panelBounds.reduced(12);
    if (!ownHeader) {
        auto header = area.removeFromTop(headerHeight);
        close.setBounds(header.removeFromRight(headerHeight));
        heading.setBounds(header);
        area.removeFromTop(8);
    }
    body.setBounds(area);
    const bool fill = bool(panelContent.getProperties()["dialogFillHeight"]);
    if (dynamic_cast<Viewport*>(&panelContent) != nullptr || fill)
        panelContent.setSize(area.getWidth(), area.getHeight());
    else
        panelContent.setSize(jmin(preferredWidth, area.getWidth()), preferredHeight);
    if (bool(panelContent.getProperties()["dialogResponsiveChild"])) {
        if (auto* viewport = dynamic_cast<Viewport*>(&panelContent)) {
            viewport->setScrollBarsShown(true, false);
            if (auto* child = viewport->getViewedComponent())
                child->setSize(jmax(1, viewport->getWidth() - viewport->getScrollBarThickness()), child->getHeight());
        }
    }
}

void SonoCallOutBox::paint(Graphics& g)
{
    g.fillAll(Colours::black.withAlpha(0.25f));
    g.setColour(SonoCraftTheme::panel());
    g.fillRoundedRectangle(panelBounds.toFloat(), 12);
    g.setColour(SonoCraftTheme::border());
    g.drawRoundedRectangle(panelBounds.toFloat().reduced(0.5f), 12, 1);
}

void SonoCallOutBox::mouseDown(const MouseEvent& event)
{
    if (!panelBounds.contains(event.getPosition())) dismiss();
}

void SonoCallOutBox::inputAttemptWhenModal() { dismiss(); }
void SonoCallOutBox::componentMovedOrResized(Component&, bool, bool sizeChanged)
{
    if (sizeChanged && !layingOut) resized();
}

// Borrow persistent editor views without changing their ownership or saved state.
class SonoCraftBorrowedPanel final : public Component, private ComponentListener {
public:
    explicit SonoCraftBorrowedPanel(Component& c) : view(&c), previousParent(c.getParentComponent()) {
        getProperties().set("dialogOwnHeader", true);
        getProperties().set("dialogFillHeight", true);
        addAndMakeVisible(c); c.addComponentListener(this);
    }
    ~SonoCraftBorrowedPanel() override {
        if (!view) return;
        view->removeComponentListener(this);
        view->setVisible(false);
        if (previousParent) previousParent->addChildComponent(view.getComponent());
        else removeChildComponent(view.getComponent());
    }
    void resized() override { if (view) view->setBounds(getLocalBounds()); }
private:
    Component::SafePointer<Component> view, previousParent;
    void componentVisibilityChanged(Component& c) override {
        if (!c.isVisible()) if (auto* dialog = findParentComponentOfClass<CallOutBox>()) dialog->dismiss();
    }
};

SonoCallOutBox& SonoCallOutBox::launchForComponent(Component& view, Component& parent, int width, int height)
{
    auto content = std::make_unique<SonoCraftBorrowedPanel>(view);
    content->setSize(width, height);
    return launchAsynchronously(std::move(content), {}, &parent, false);
}


//==============================================================================
class SonoCallOutBoxCallback  : public ModalComponentManager::Callback,
                            private Timer
{
public:
    SonoCallOutBoxCallback (std::unique_ptr<Component> c, const Rectangle<int>& area, Component* parent, bool dismissIfBg, std::function<bool(const Component*)>passthroughFunc)
        : content (std::move (c)),
          callout (*content, area, parent), dismissIfBackgrounded(dismissIfBg)
    {
        callout.canPassthrough = passthroughFunc;
        callout.setVisible (true);
        callout.enterModalState (true, this);
        if (dismissIfBackgrounded) {
            startTimer (200);
        }
    }

    void modalStateFinished (int) override {}

    void timerCallback() override
    {
        if (! Process::isForegroundProcess())
            callout.dismiss();
    }

    std::unique_ptr<Component> content;
    SonoCallOutBox callout;
    bool dismissIfBackgrounded = true;

    JUCE_DECLARE_NON_COPYABLE (SonoCallOutBoxCallback)
};

SonoCallOutBox& SonoCallOutBox::launchAsynchronously (std::unique_ptr<Component> content, Rectangle<int> area, Component* parent, bool dismissIfBackgrounded, std::function<bool(const Component*)> canPassthroughFunc)
{
    jassert (content != nullptr); // must be a valid content component!

    return (new SonoCallOutBoxCallback (std::move (content), area, parent, dismissIfBackgrounded, canPassthroughFunc))->callout;
}

bool SonoCallOutBox::canModalEventBeSentToComponent (const Component* targetComponent)
{
    if (canPassthrough) {
        return canPassthrough(targetComponent);
    }
    return false;
}

void SonoCallOutBox::setVisible(bool shouldBeVisible)
{
    CallOutBox::setVisible(shouldBeVisible);
    
    if (wasHidden && !shouldBeVisible) {
        wasHidden();
    }
}
