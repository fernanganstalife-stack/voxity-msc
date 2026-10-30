#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "Presets.h"
#include "DSP.h"

class MSCVoxAudioProcessor : public juce::AudioProcessor
{
public:
    MSCVoxAudioProcessor();
    ~MSCVoxAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MSC VOX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- message-thread API -------------------------------------------------
    // Loads factory preset idx. Increments presetChangeCounter ONLY if the
    // preset actually differs from the current one (this triggers the animation).
    void loadPreset (int idx);

    juce::AudioProcessorValueTreeState apvts;

    // ---- shared with the GUI (all lock-free) ---------------------------------
    std::atomic<int>   presetIndex { 7 };
    std::atomic<int>   presetChangeCounter { 0 };
    std::atomic<float> inPeak { 0.f }, outPeak { 0.f };
    std::atomic<bool>  vocalCheck { false };   // while true: you hear the dry signal
    std::array<std::atomic<float>, 256> wave;
    std::atomic<int>   wavePos { 0 };

private:
    struct Chan
    {
        DSP::Biquad hp, eqLo, eqLm, eqHm, eqHi, air, pres, ds;
        DSP::DelayLine pre, dly, cho;
        float lpFb = 0.f, dcX = 0.f, dcY = 0.f;
    };

    void updateTargets (double bpm);
    float R (int p) const { return raw[p]->load(); }

    std::atomic<float>* raw[MSC::numParams] {};
    juce::SmoothedValue<float> sm[MSC::numParams];
    juce::SmoothedValue<float> gInSm, gOutSm, mixSm, dlySm;

    Chan ch[2];
    juce::Reverb reverb;

    double sr = 44100.0;
    float grDb = 0.f, dsEnv = 0.f, lpA = 0.5f, dsAtk = 0.f, dsRel = 0.f;
    float choPhase = 0.f;
    float waveAcc = 0.f;
    int waveCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MSCVoxAudioProcessor)
};
