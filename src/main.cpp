#include "ui/Workspace.h"
#include <BinaryData.h>

namespace auralis
{
class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow() : DocumentWindow(design::windowTitle, colour(design::colour::background), allButtons)
    {
        setUsingNativeTitleBar(true);
        setIcon(juce::ImageCache::getFromMemory(BinaryData::auralisicon_png, BinaryData::auralisicon_pngSize));
        setContentOwned(new Workspace(), true);
        setResizable(true, false);
        setResizeLimits(design::minimumWidth, design::minimumHeight, design::maximumWidth, design::maximumHeight);
        const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        const auto screen = display != nullptr ? display->userArea : juce::Rectangle<int>(0, 0, 1920, 1080);
        centreWithSize(juce::jmin(design::initialWidth, screen.getWidth()-60),
                       juce::jmin(design::initialHeight, screen.getHeight()-60));
        setVisible(true);
    }
    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};
class Application final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return design::appName; }
    const juce::String getApplicationVersion() override { return design::appVersion; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String& args) override { window = std::make_unique<MainWindow>(); applyWindowSize(args); }
    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String& args) override { applyWindowSize(args); if (window) window->toFront(true); }
private:
    // Reproducible manual QA viewport; does not change project/session data.
    void applyWindowSize(const juce::String& args)
    {
        if (window && args.trim() == "--compact") window->centreWithSize(design::minimumWidth, design::minimumHeight);
    }
    std::unique_ptr<MainWindow> window;
};
}
START_JUCE_APPLICATION(auralis::Application)
