#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace MSC;

MSCVoxAudioProcessor::MSCVoxAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MSCVOX", MSC::createLayout())
{
    for (int i = 0; i < numParams; ++i)
        raw[i] = apvts.getRawParameterValue (defs()[i].id);

    for (auto& w : wave) w.store (0.f);

    // The parameter defaults are preset 08 (MYSPACECORE).
    apvts.state.setProperty ("preset", 7, nullptr);
}

bool MSCVoxAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void MSCVoxAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    for (auto& c : ch)
    {
        c.hp.reset(); c.eqLo.reset(); c.eqLm.reset(); c.eqHm.reset(); c.eqHi.reset();
        c.air.reset(); c.pres.reset(); c.ds.reset();
        c.pre.prepare ((int) (0.15 * sr));
        c.cho.prepare ((int) (0.08 * sr));
        c.dly.prepare ((int) (4.5 * sr) + 8);
        c.lpFb = c.dcX = c.dcY = 0.f;
    }

    reverb.setSampleRate (sr);
    reverb.reset();

    grDb = 0.f; dsEnv = 0.f; choPhase = 0.f; waveAcc = 0.f; waveCount = 0;
    lpA = 1.f - std::exp (-2.f * juce::MathConstants<float>::pi * 5000.f / (float) sr);
    dsAtk = std::exp (-1.f / (0.001f * (float) sr));
    dsRel = std::exp (-1.f / (0.050f * (float) sr));

    for (auto& s : sm) s.reset (sr, 0.04);
    gInSm.reset (sr, 0.03); gOutSm.reset (sr, 0.03); mixSm.reset (sr, 0.03); dlySm.reset (sr, 0.20);

    updateTargets (120.0);
    for (auto& s : sm) s.setCurrentAndTargetValue (s.getTargetValue());
    gInSm.setCurrentAndTargetValue (gInSm.getTargetValue());
    gOutSm.setCurrentAndTargetValue (gOutSm.getTargetValue());
    mixSm.setCurrentAndTargetValue (mixSm.getTargetValue());
    dlySm.setCurrentAndTargetValue (dlySm.getTargetValue());
}

void MSCVoxAudioProcessor::updateTargets (double bpm)
{
    for (int i = 0; i < numParams; ++i)
        sm[i].setTargetValue (R (i));

    // "On" switches are folded into the smoothed values so toggling never clicks.
    auto on = [this] (int p) { return R (p) > 0.5f; };
    sm[compRatio] .setTargetValue (on (compOn) ? R (compRatio) : 1.f);
    sm[compMakeup].setTargetValue (on (compOn) ? R (compMakeup) : 0.f);
    sm[eqLow]     .setTargetValue (on (eqOn) ? R (eqLow)     : 0.f);
    sm[eqLowMid]  .setTargetValue (on (eqOn) ? R (eqLowMid)  : 0.f);
    sm[eqHighMid] .setTargetValue (on (eqOn) ? R (eqHighMid) : 0.f);
    sm[eqHigh]    .setTargetValue (on (eqOn) ? R (eqHigh)    : 0.f);
    sm[air]       .setTargetValue (on (airOn)  ? R (air)  : 0.f);
    sm[pres]      .setTargetValue (on (presOn) ? R (pres) : 0.f);
    sm[dsAmt]     .setTargetValue (on (dsOn)  ? R (dsAmt)  : 0.f);
    sm[satMix]    .setTargetValue (on (satOn) ? R (satMix) : 0.f);
    sm[revMix]    .setTargetValue (on (revOn) ? R (revMix) : 0.f);
    sm[dlyMix]    .setTargetValue (on (dlyOn) ? R (dlyMix) : 0.f);
    sm[choMix]    .setTargetValue (on (choOn) ? R (choMix) : 0.f);

    const bool dry = on (bypass) || vocalCheck.load();
    gInSm .setTargetValue (juce::Decibels::decibelsToGain (R (inGain)));
    gOutSm.setTargetValue (dry ? 1.f : juce::Decibels::decibelsToGain (R (outGain) + R (master)));
    mixSm .setTargetValue (dry ? 0.f : R (mix));

    // delay time: host-synced note value or free milliseconds
    const float t = R (dlyTime);
    float ms;
    if (on (dlySync))
    {
        static const float beats[5] = { 0.25f, 0.5f, 1.f, 2.f, 4.f };
        ms = (float) (60000.0 / bpm) * beats[juce::jmin (4, (int) (t * 5.f))];
    }
    else
        ms = 20.f + t * 980.f;

    dlySm.setTargetValue (juce::jlimit (32.f, (float) (4.4 * sr), ms * 0.001f * (float) sr));
}

void MSCVoxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int nCh = juce::jmin (2, buffer.getNumChannels());
    const int total = buffer.getNumSamples();
    if (nCh <= 0 || total <= 0) return;
    const bool st = nCh > 1;

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 20.0) bpm = *b;

    updateTargets (bpm);

    float* io[2] = { buffer.getWritePointer (0), st ? buffer.getWritePointer (1) : nullptr };
    float inPk = 0.f, outPk = 0.f;

    constexpr int kBlock = 32;
    float dryA[2][kBlock], wetA[2][kBlock], rvA[2][kBlock];
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float fsr = (float) sr;

    for (int start = 0; start < total; start += kBlock)
    {
        const int len = juce::jmin (kBlock, total - start);
        auto V = [&] (int p) { return sm[p].skip (len); };

        // ---------------- per-sub-block parameters ----------------
        const float thr = V (compThresh), ratio = V (compRatio), atkMs = V (compAttack),
                    relMs = V (compRelease), mk = V (compMakeup);
        const float slope = 1.f - 1.f / juce::jmax (1.f, ratio);
        const float atkC = std::exp (-1.f / (0.001f * atkMs * fsr));
        const float relC = std::exp (-1.f / (0.001f * relMs * fsr));

        ch[0].hp.highpass (sr, V (hpf), 0.707);
        ch[0].eqLo.lowShelf (sr, 100.0, V (eqLow));
        ch[0].eqLm.peak (sr, 400.0, 1.0, V (eqLowMid));
        ch[0].eqHm.peak (sr, 2500.0, 1.0, V (eqHighMid));
        ch[0].eqHi.highShelf (sr, 8000.0, V (eqHigh));
        ch[0].air.highShelf (sr, 10000.0, 9.0 * V (air));
        ch[0].pres.peak (sr, 4500.0, 0.9, 6.0 * V (pres));
        ch[0].ds.bandpass (sr, V (dsFreq), 1.5);
        if (st)
        {
            ch[1].hp.copyCoefs (ch[0].hp);     ch[1].eqLo.copyCoefs (ch[0].eqLo);
            ch[1].eqLm.copyCoefs (ch[0].eqLm); ch[1].eqHm.copyCoefs (ch[0].eqHm);
            ch[1].eqHi.copyCoefs (ch[0].eqHi); ch[1].air.copyCoefs (ch[0].air);
            ch[1].pres.copyCoefs (ch[0].pres); ch[1].ds.copyCoefs (ch[0].ds);
        }

        const float dsThr = V (dsThresh), dsAm = V (dsAmt);
        const float drive = V (satDrive), satM = V (satMix);
        const int   type = (int) (R (satType) + 0.5f);
        const float gSat = DSP::fromDb (drive * 30.f);
        const float satComp = 1.f / (1.f + drive * 1.5f);
        const float cRate = V (choRate), cDepth = V (choDepth), cWidth = V (choWidth), cMix = V (choMix);
        const float dFb = V (dlyFb), dMix = V (dlyMix);
        const float preSamp = juce::jmax (1.f, V (revPre) * 0.001f * fsr);
        const bool pingpong = st && R (dlyStereo) > 0.5f;

        // ---------------- loop 1: everything up to the reverb send ----------------
        for (int i = 0; i < len; ++i)
        {
            const int idx = start + i;
            float dry[2] = { 0.f, 0.f }, x[2] = { 0.f, 0.f };
            const float gin = gInSm.getNextValue();

            for (int c = 0; c < nCh; ++c)
            {
                dry[c] = io[c][idx];
                inPk = juce::jmax (inPk, std::abs (dry[c]));
                dryA[c][i] = dry[c];
                x[c] = ch[c].hp.process (dry[c] * gin);   // input gain -> high-pass
            }

            // compressor (stereo-linked, soft knee 6 dB)
            {
                float lvl = 0.f;
                for (int c = 0; c < nCh; ++c) lvl = juce::jmax (lvl, std::abs (x[c]));
                const float db = 20.f * std::log10 (lvl + 1e-9f);
                const float over = db - thr;
                float target;
                if (over <= -3.f)      target = 0.f;
                else if (over >= 3.f)  target = over * slope;
                else { const float t = over + 3.f; target = slope * t * t / 12.f; }
                const float co = target > grDb ? atkC : relC;
                grDb = co * grDb + (1.f - co) * target;
                const float cg = DSP::fromDb (mk - grDb);
                for (int c = 0; c < nCh; ++c) x[c] *= cg;
            }

            // EQ -> air -> presence
            for (int c = 0; c < nCh; ++c)
            {
                float v = x[c];
                v = ch[c].eqLo.process (v);
                v = ch[c].eqLm.process (v);
                v = ch[c].eqHm.process (v);
                v = ch[c].eqHi.process (v);
                v = ch[c].air.process (v);
                v = ch[c].pres.process (v);
                x[c] = v;
            }

            // de-esser (split-band, stereo-linked detector)
            {
                float band[2] = { 0.f, 0.f };
                float e = 0.f;
                for (int c = 0; c < nCh; ++c)
                {
                    band[c] = ch[c].ds.process (x[c]);
                    e = juce::jmax (e, std::abs (band[c]));
                }
                dsEnv = (e > dsEnv) ? dsAtk * dsEnv + (1.f - dsAtk) * e
                                    : dsRel * dsEnv + (1.f - dsRel) * e;
                const float over = 20.f * std::log10 (dsEnv + 1e-9f) - dsThr;
                const float gr = over > 0.f ? juce::jmin (18.f, over * dsAm * 1.2f) : 0.f;
                const float bg = DSP::fromDb (-gr);
                for (int c = 0; c < nCh; ++c) x[c] -= band[c] * (1.f - bg);
            }

            // saturation / distortion
            for (int c = 0; c < nCh; ++c)
            {
                const float u = x[c] * gSat;
                float y;
                switch (type)
                {
                    case 0:  y = std::tanh (u + 0.2f * drive) - std::tanh (0.2f * drive); break;       // tube
                    case 1:  y = std::atan (u * 1.2f) / 1.2f; break;                                    // tape
                    case 2:  y = juce::jlimit (-1.f, 1.f, u); break;                                    // hard clip
                    default: y = (std::tanh (3.f * u + 0.3f) - std::tanh (0.3f)) * 0.5f; break;         // fuzz
                }
                y *= satComp;
                const float d = y - ch[c].dcX + 0.995f * ch[c].dcY;   // DC blocker
                ch[c].dcX = y; ch[c].dcY = d;
                x[c] = x[c] + (d - x[c]) * satM;
            }

            // chorus
            choPhase += cRate / fsr;
            if (choPhase >= 1.f) choPhase -= 1.f;
            for (int c = 0; c < nCh; ++c)
            {
                const float ph = choPhase + (c == 1 ? cWidth * 0.5f : 0.f);
                const float d = (14.f + cDepth * 6.f * std::sin (twoPi * ph)) * 0.001f * fsr;
                const float w = ch[c].cho.read (d);
                ch[c].cho.push (x[c]);
                x[c] = x[c] * (1.f - 0.35f * cMix) + w * cMix * 0.8f;
            }

            // stereo (ping-pong) delay, send style
            {
                const float dt = dlySm.getNextValue();
                float dl[2] = { 0.f, 0.f }, fb[2] = { 0.f, 0.f };
                for (int c = 0; c < nCh; ++c)
                {
                    dl[c] = ch[c].dly.read (dt);
                    ch[c].lpFb += lpA * (dl[c] - ch[c].lpFb);
                    fb[c] = ch[c].lpFb;
                }
                if (pingpong)
                {
                    const float in = 0.5f * (x[0] + x[1]);
                    ch[0].dly.push (in + std::tanh (dFb * fb[1]));
                    ch[1].dly.push (std::tanh (dFb * fb[0]));
                }
                else
                    for (int c = 0; c < nCh; ++c)
                        ch[c].dly.push (x[c] + std::tanh (dFb * fb[c]));

                for (int c = 0; c < nCh; ++c) x[c] += dMix * dl[c];
            }

            // reverb pre-delay + store
            for (int c = 0; c < nCh; ++c)
            {
                rvA[c][i] = ch[c].pre.read (preSamp);
                ch[c].pre.push (x[c]);
                wetA[c][i] = x[c];
            }
        }

        // ---------------- reverb (processed per sub-block) ----------------
        {
            const float size = V (revSize), decay = V (revDecay), damp = V (revDamp);
            const int rt = (int) (R (revType) + 0.5f);
            const float scale = rt == 0 ? 0.75f : (rt == 1 ? 1.0f : 0.9f);
            const float dampAdj = rt == 0 ? 0.15f : (rt == 2 ? -0.15f : 0.f);

            juce::Reverb::Parameters rp;
            rp.roomSize = juce::jlimit (0.1f, 0.98f, (0.25f + 0.4f * size + 0.055f * decay) * scale);
            rp.damping  = juce::jlimit (0.f, 1.f, damp + dampAdj);
            rp.wetLevel = 0.33f;
            rp.dryLevel = 0.f;
            rp.width    = 1.f;
            rp.freezeMode = 0.f;
            reverb.setParameters (rp);

            if (st) reverb.processStereo (rvA[0], rvA[1], len);
            else    reverb.processMono (rvA[0], len);
        }

        // ---------------- loop 2: reverb mix, wet/dry, output gain ----------------
        const float rMix = V (revMix);
        for (int i = 0; i < len; ++i)
        {
            const float m = mixSm.getNextValue();
            const float go = gOutSm.getNextValue();
            for (int c = 0; c < nCh; ++c)
            {
                const float w = wetA[c][i] + rMix * rvA[c][i];
                const float y = (dryA[c][i] * (1.f - m) + w * m) * go;
                io[c][start + i] = y;
                outPk = juce::jmax (outPk, std::abs (y));
                waveAcc = juce::jmax (waveAcc, std::abs (y));
            }
            if (++waveCount >= 256)
            {
                const int p = wavePos.load();
                wave[(size_t) p].store (waveAcc);
                wavePos.store ((p + 1) & 255);
                waveAcc = 0.f; waveCount = 0;
            }
        }
    }

    inPeak.store  (juce::jmax (inPeak.load(),  inPk));
    outPeak.store (juce::jmax (outPeak.load(), outPk));
}

// ---------------------------------------------------------------------------
void MSCVoxAudioProcessor::loadPreset (int idx)
{
    const auto& list = presets();
    idx = juce::jlimit (0, (int) list.size() - 1, idx);
    const auto& pr = list[(size_t) idx];

    for (int i = 0; i < numParams; ++i)
    {
        if (i == inGain || i == outGain || i == master || i == bypass) continue;
        float v = defs()[i].def;
        for (const auto& o : pr.v)
            if ((int) o.id == i) v = o.v;
        if (auto* p = apvts.getParameter (defs()[i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    }

    const bool changed = (idx != presetIndex.load());
    presetIndex.store (idx);
    apvts.state.setProperty ("preset", idx, nullptr);
    if (changed) presetChangeCounter.fetch_add (1);
}

void MSCVoxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr) copyXmlToBinary (*xml, destData);
}

void MSCVoxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        presetIndex.store (juce::jlimit (0, 7, (int) apvts.state.getProperty ("preset", 7)));
    }
}

juce::AudioProcessorEditor* MSCVoxAudioProcessor::createEditor()
{
    return new MSCVoxAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MSCVoxAudioProcessor();
}
