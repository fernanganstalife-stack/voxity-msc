#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Widgets.h"
#include "Characters.h"

class MSCVoxAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit MSCVoxAudioProcessorEditor (MSCVoxAudioProcessor&);
    ~MSCVoxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    void timerCallback() override;
    void paintStatic (juce::Graphics&);          // everything that never moves (design space 1536 x 1024)
    juce::Image makeTexture();

    void select (int presetIdx);
    void refreshPresetUI (int presetIdx);
    void saveUserPreset();
    void loadUserPreset();
    void showSettings();

    void add (juce::Component& c, juce::Rectangle<int> designBounds);
    UI::Knob* addKnob (const char* id, const char* name, std::function<juce::String (double)> fmt,
                       int cx, int top, int w = 70, int h = 104, float font = 14.f);
    void addToggle (const char* id, const juce::String& text, juce::Rectangle<int> r, const char* compId = "");
    void addPower (const char* id, int x, int y);

    MSCVoxAudioProcessor& proc;
    UI::MSCLookAndFeel laf;

    juce::Image bg, texture;
    float sc = 0.75f;
    juce::Rectangle<int> charRegion;

    Chars::Animator anim;
    int lastCounter = 0, shownPreset = -1, fps = 30, frameCount = 0;
    double lastT = 0.0;
    float outLevel = 0.f;

    struct Item { juce::Component* c; juce::Rectangle<int> r; };
    std::vector<Item> items;

    std::vector<std::unique_ptr<UI::Knob>> knobs;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    UI::Knob* delayKnob = nullptr;

    juce::ComboBox presetBox, satBox, revBox;
    juce::TextButton prevBtn, nextBtn, saveBtn, loadBtn, gearBtn, vocalBtn;
    juce::Slider masterSlider;
    UI::PresetList presetList;
    UI::MeterBar inMeter, outMeter;
    std::unique_ptr<UI::LiveView> waveView, eqView, dsView, choView, revView;

    std::unique_ptr<juce::FileChooser> chooser;

    // attachments last => destroyed first
    std::vector<std::unique_ptr<APVTS::ButtonAttachment>> btnAtt;
    std::unique_ptr<APVTS::ComboBoxAttachment> satAtt, revAtt;
    std::unique_ptr<APVTS::SliderAttachment> masterAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MSCVoxAudioProcessorEditor)
};
