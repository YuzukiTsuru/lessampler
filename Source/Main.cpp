#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"

class LESSAMPLERApplication : public JUCEApplication {
  public:
    LESSAMPLERApplication() {}

    const String getApplicationName() override { return ProjectInfo::projectName; }
    const String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void initialise(const String &commandLine) override {
        Typeface::Ptr getTypefaceForFont(const Font &f);
        LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override {
        mainWindow = nullptr;
    }
    void systemRequestedQuit() override {
        quit();
    }

    void anotherInstanceStarted(const String &commandLine) override {
    }

    class MainWindow : public DocumentWindow {
      public:
        MainWindow(String name) : DocumentWindow(name, Desktop::getInstance().getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
#endif

            setVisible(true);
        }

        void closeButtonPressed() override {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

      private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

  private:
    class CustomFontLookAndFeel : public LookAndFeel_V4 {
      public:
        CustomFontLookAndFeel() {
            LookAndFeel::setDefaultLookAndFeel(this);
        }

        static const Font getCustomFont() {
            static auto typeface = Typeface::createSystemTypefaceFor(BinaryData::SiHei_otf, BinaryData::SiHei_otfSize);
            return Font(typeface);
        }

        Typeface::Ptr getTypefaceForFont(const Font &f) override {
            return getCustomFont().getTypeface();
        }

      private:
    } customLookAndFeel;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(LESSAMPLERApplication)
