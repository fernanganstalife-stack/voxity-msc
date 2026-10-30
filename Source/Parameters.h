#pragma once
#include <JuceHeader.h>

namespace MSC
{
// The order of this enum MUST match the order of the table in defs().
enum P
{
    inGain, outGain, master, mix, bypass,
    compOn, compThresh, compRatio, compAttack, compRelease, compMakeup,
    eqOn, eqLow, eqLowMid, eqHighMid, eqHigh, hpf,
    airOn, air, presOn, pres,
    dsOn, dsFreq, dsThresh, dsAmt,
    satOn, satDrive, satMix, satType,
    revOn, revSize, revDecay, revDamp, revPre, revMix, revType,
    dlyOn, dlyTime, dlyFb, dlyMix, dlySync, dlyStereo,
    choOn, choRate, choDepth, choWidth, choMix,
    numParams
};

// kind: 0 = float, 1 = bool, 2 = choice ("A|B|C", lo=0, hi=count-1)
struct Def { const char* id; const char* name; float lo, hi, def, skew; int kind; const char* choices; };

inline const Def* defs()
{
    static const Def d[numParams] =
    {
        { "inGain",     "Input Gain",        -24.f, 24.f,   0.f,   1.f, 0, nullptr },
        { "outGain",    "Output Gain",       -24.f, 24.f,   0.f,   1.f, 0, nullptr },
        { "master",     "Master",            -24.f,  6.f,   0.f,   1.f, 0, nullptr },
        { "mix",        "Wet/Dry",             0.f,  1.f,   1.f,   1.f, 0, nullptr },
        { "bypass",     "Bypass",              0.f,  1.f,   0.f,   1.f, 1, nullptr },

        { "compOn",     "Comp On",             0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "compThresh", "Comp Threshold",    -40.f,  0.f, -12.f,   1.f, 0, nullptr },
        { "compRatio",  "Comp Ratio",          1.f, 20.f,   4.f,  0.5f, 0, nullptr },
        { "compAttack", "Comp Attack",        0.1f,100.f,   5.f,  0.4f, 0, nullptr },
        { "compRelease","Comp Release",       10.f,1000.f, 120.f, 0.4f, 0, nullptr },
        { "compMakeup", "Comp Makeup",         0.f, 24.f,   4.f,   1.f, 0, nullptr },

        { "eqOn",       "EQ On",               0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "eqLow",      "EQ Low",            -12.f, 12.f,  -3.f,   1.f, 0, nullptr },
        { "eqLowMid",   "EQ Low Mid",        -12.f, 12.f,  1.5f,   1.f, 0, nullptr },
        { "eqHighMid",  "EQ High Mid",       -12.f, 12.f,   2.f,   1.f, 0, nullptr },
        { "eqHigh",     "EQ High",           -12.f, 12.f,   3.f,   1.f, 0, nullptr },
        { "hpf",        "High-Pass",          20.f,400.f,  80.f,  0.5f, 0, nullptr },

        { "airOn",      "Air On",              0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "air",        "Air",                 0.f,  1.f,  0.8f,   1.f, 0, nullptr },
        { "presOn",     "Presence On",         0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "pres",       "Presence",            0.f,  1.f,  0.7f,   1.f, 0, nullptr },

        { "dsOn",       "De-Esser On",         0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "dsFreq",     "De-Esser Freq",    3000.f,10000.f,6500.f, 0.7f, 0, nullptr },
        { "dsThresh",   "De-Esser Thresh",   -40.f,  0.f, -20.f,   1.f, 0, nullptr },
        { "dsAmt",      "De-Esser Amount",     0.f,  1.f,  0.6f,   1.f, 0, nullptr },

        { "satOn",      "Sat On",              0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "satDrive",   "Sat Drive",           0.f,  1.f, 0.35f,   1.f, 0, nullptr },
        { "satMix",     "Sat Mix",             0.f,  1.f,  0.4f,   1.f, 0, nullptr },
        { "satType",    "Sat Type",            0.f,  3.f,   0.f,   1.f, 2, "Tube|Tape|Hard Clip|Fuzz" },

        { "revOn",      "Reverb On",           0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "revSize",    "Reverb Size",         0.f,  1.f,  0.5f,   1.f, 0, nullptr },
        { "revDecay",   "Reverb Decay",       0.2f, 10.f,  2.8f,  0.5f, 0, nullptr },
        { "revDamp",    "Reverb Damping",      0.f,  1.f,  0.5f,   1.f, 0, nullptr },
        { "revPre",     "Reverb Pre-Delay",    0.f,100.f,  20.f,   1.f, 0, nullptr },
        { "revMix",     "Reverb Mix",          0.f,  1.f,  0.3f,   1.f, 0, nullptr },
        { "revType",    "Reverb Type",         0.f,  2.f,   1.f,   1.f, 2, "Room|Hall|Plate" },

        { "dlyOn",      "Delay On",            0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "dlyTime",    "Delay Time",          0.f,  1.f,  0.5f,   1.f, 0, nullptr },
        { "dlyFb",      "Delay Feedback",      0.f, 0.95f, 0.35f,  1.f, 0, nullptr },
        { "dlyMix",     "Delay Mix",           0.f,  1.f,  0.4f,   1.f, 0, nullptr },
        { "dlySync",    "Delay Sync",          0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "dlyStereo",  "Delay Stereo",        0.f,  1.f,   1.f,   1.f, 1, nullptr },

        { "choOn",      "Chorus On",           0.f,  1.f,   1.f,   1.f, 1, nullptr },
        { "choRate",    "Chorus Rate",        0.1f,  8.f,  1.2f,  0.5f, 0, nullptr },
        { "choDepth",   "Chorus Depth",        0.f,  1.f,  0.7f,   1.f, 0, nullptr },
        { "choWidth",   "Chorus Width",        0.f,  1.f,   1.f,   1.f, 0, nullptr },
        { "choMix",     "Chorus Mix",          0.f,  1.f,  0.6f,   1.f, 0, nullptr },
    };
    return d;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < numParams; ++i)
    {
        const auto& d = defs()[i];
        if (d.kind == 0)
        {
            NormalisableRange<float> r (d.lo, d.hi);
            r.skew = d.skew;
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { d.id, 1 }, d.name, r, d.def));
        }
        else if (d.kind == 1)
        {
            layout.add (std::make_unique<AudioParameterBool> (ParameterID { d.id, 1 }, d.name, d.def > 0.5f));
        }
        else
        {
            layout.add (std::make_unique<AudioParameterChoice> (ParameterID { d.id, 1 }, d.name,
                                                                StringArray::fromTokens (d.choices, "|", ""),
                                                                (int) d.def));
        }
    }
    return layout;
}
} // namespace MSC
