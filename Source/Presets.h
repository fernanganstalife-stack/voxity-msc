#pragma once
#include "Parameters.h"
#include <vector>

// Any parameter NOT listed in a preset is reset to its default value
// (the defaults are the MYSPACECORE look from the reference image).
// Input gain, output gain, master and bypass are never touched by presets.
namespace MSC
{
struct PV { P id; float v; };
struct Preset { const char* name; std::vector<PV> v; };

inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list =
    {
        { "MYSPACE DREAM", {
            { compThresh, -14 }, { compRatio, 3 }, { compAttack, 8 }, { compRelease, 150 }, { compMakeup, 3 },
            { eqLow, 0 }, { eqLowMid, -1 }, { eqHighMid, 2 }, { eqHigh, 3.5f }, { hpf, 90 },
            { air, 0.65f }, { pres, 0.5f }, { dsAmt, 0.5f },
            { satType, 0 }, { satDrive, 0.15f }, { satMix, 0.3f },
            { revSize, 0.6f }, { revDecay, 2.6f }, { revDamp, 0.45f }, { revPre, 25 }, { revMix, 0.3f }, { revType, 1 },
            { dlyTime, 0.3f }, { dlyFb, 0.3f }, { dlyMix, 0.25f }, { dlyStereo, 1 },
            { choRate, 0.8f }, { choDepth, 0.5f }, { choWidth, 1 }, { choMix, 0.4f } } },

        { "DESTROYED DREAM", {
            { compThresh, -22 }, { compRatio, 8 }, { compAttack, 2 }, { compRelease, 90 }, { compMakeup, 6 },
            { eqLow, -4 }, { eqLowMid, -2 }, { eqHighMid, 3 }, { eqHigh, -2 }, { hpf, 160 },
            { air, 0.3f }, { pres, 0.6f }, { dsAmt, 0.4f },
            { satType, 3 }, { satDrive, 0.65f }, { satMix, 0.55f },
            { revSize, 0.8f }, { revDecay, 5.5f }, { revDamp, 0.6f }, { revPre, 40 }, { revMix, 0.45f }, { revType, 1 },
            { dlyTime, 0.5f }, { dlyFb, 0.6f }, { dlyMix, 0.4f }, { dlyStereo, 1 },
            { choRate, 0.6f }, { choDepth, 0.9f }, { choWidth, 1 }, { choMix, 0.5f } } },

        { "CHORUS HELL", {
            { compThresh, -16 }, { compRatio, 4 }, { compAttack, 5 }, { compRelease, 120 }, { compMakeup, 4 },
            { eqLow, -2 }, { eqLowMid, 0 }, { eqHighMid, 3 }, { eqHigh, 4 }, { hpf, 120 },
            { air, 0.7f }, { pres, 0.6f }, { dsAmt, 0.6f },
            { satType, 1 }, { satDrive, 0.2f }, { satMix, 0.3f },
            { revSize, 0.5f }, { revDecay, 2.0f }, { revDamp, 0.5f }, { revPre, 15 }, { revMix, 0.25f }, { revType, 2 },
            { dlyTime, 0.1f }, { dlyFb, 0.45f }, { dlyMix, 0.3f }, { dlyStereo, 1 },
            { choRate, 3.0f }, { choDepth, 1 }, { choWidth, 1 }, { choMix, 1 } } },

        { "VAMPIRE LOVE", {
            { compThresh, -16 }, { compRatio, 4 }, { compAttack, 10 }, { compRelease, 200 }, { compMakeup, 5 },
            { eqLow, 3 }, { eqLowMid, 2 }, { eqHighMid, 0 }, { eqHigh, -1.5f }, { hpf, 70 },
            { air, 0.3f }, { pres, 0.4f }, { dsAmt, 0.6f },
            { satType, 1 }, { satDrive, 0.4f }, { satMix, 0.5f },
            { revSize, 0.7f }, { revDecay, 4.0f }, { revDamp, 0.6f }, { revPre, 30 }, { revMix, 0.38f }, { revType, 1 },
            { dlyTime, 0.5f }, { dlyFb, 0.4f }, { dlyMix, 0.3f }, { dlyStereo, 1 },
            { choRate, 0.5f }, { choDepth, 0.4f }, { choWidth, 0.8f }, { choMix, 0.3f } } },

        { "ANGEL VOICE", {
            { compThresh, -14 }, { compRatio, 2.5f }, { compAttack, 15 }, { compRelease, 250 }, { compMakeup, 2 },
            { eqLow, -3 }, { eqLowMid, -1 }, { eqHighMid, 1.5f }, { eqHigh, 4.5f }, { hpf, 130 },
            { air, 1.0f }, { pres, 0.55f }, { dsFreq, 7000 }, { dsThresh, -24 }, { dsAmt, 0.75f },
            { satType, 0 }, { satDrive, 0.05f }, { satMix, 0.1f },
            { revSize, 0.85f }, { revDecay, 6.5f }, { revDamp, 0.35f }, { revPre, 35 }, { revMix, 0.5f }, { revType, 1 },
            { dlyTime, 0.5f }, { dlyFb, 0.35f }, { dlyMix, 0.2f }, { dlyStereo, 1 },
            { choRate, 0.7f }, { choDepth, 0.4f }, { choWidth, 1 }, { choMix, 0.5f } } },

        { "EVERYTHING BROKEN", {
            { compThresh, -26 }, { compRatio, 10 }, { compAttack, 1 }, { compRelease, 80 }, { compMakeup, 8 },
            { eqLow, -5 }, { eqLowMid, -3 }, { eqHighMid, 5 }, { eqHigh, 2 }, { hpf, 220 },
            { air, 0.4f }, { pres, 0.8f }, { dsAmt, 0.3f },
            { satType, 2 }, { satDrive, 0.8f }, { satMix, 0.75f },
            { revSize, 0.3f }, { revDecay, 1.2f }, { revDamp, 0.7f }, { revPre, 5 }, { revMix, 0.2f }, { revType, 0 },
            { dlyTime, 0.1f }, { dlyFb, 0.6f }, { dlyMix, 0.35f }, { dlyStereo, 1 },
            { choRate, 5.0f }, { choDepth, 0.9f }, { choWidth, 1 }, { choMix, 0.45f } } },

        { "RAW UNDERGROUND", {
            { compThresh, -18 }, { compRatio, 6 }, { compAttack, 3 }, { compRelease, 100 }, { compMakeup, 6 },
            { eqLow, 2 }, { eqLowMid, 0.5f }, { eqHighMid, 3 }, { eqHigh, 0 }, { hpf, 100 },
            { air, 0.15f }, { pres, 0.7f }, { dsAmt, 0.5f },
            { satType, 0 }, { satDrive, 0.3f }, { satMix, 0.5f },
            { revSize, 0.2f }, { revDecay, 0.6f }, { revDamp, 0.7f }, { revPre, 0 }, { revMix, 0.1f }, { revType, 0 },
            { dlyOn, 0 }, { choOn, 0 } } },

        { "MYSPACECORE", {
            { compThresh, -12 }, { compRatio, 4 }, { compAttack, 5 }, { compRelease, 120 }, { compMakeup, 4 },
            { eqLow, -3 }, { eqLowMid, 1.5f }, { eqHighMid, 2 }, { eqHigh, 3 }, { hpf, 80 },
            { air, 0.8f }, { pres, 0.7f }, { dsFreq, 6500 }, { dsThresh, -20 }, { dsAmt, 0.6f },
            { satType, 0 }, { satDrive, 0.35f }, { satMix, 0.4f },
            { revSize, 0.5f }, { revDecay, 2.8f }, { revDamp, 0.5f }, { revPre, 20 }, { revMix, 0.3f }, { revType, 1 },
            { dlyTime, 0.5f }, { dlyFb, 0.35f }, { dlyMix, 0.4f }, { dlyStereo, 1 },
            { choRate, 1.2f }, { choDepth, 0.7f }, { choWidth, 1 }, { choMix, 0.6f } } },
    };
    return list;
}
} // namespace MSC
