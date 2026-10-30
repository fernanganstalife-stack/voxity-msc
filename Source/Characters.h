#pragma once
#include <JuceHeader.h>
#include <iterator>

// ============================================================================
//  Character animation system (GUI thread only - never touched by audio code)
//
//  Animator  : pure state machine. Produces a Pose for each character every frame.
//  drawFemale/drawMale : layered vector artwork posed by a Pose.
//
//  To replace the artwork with sprite sheets later, keep Animator + Pose as they
//  are and only rewrite drawFemale()/drawMale() to pick a frame from the Pose
//  (e.g. frame = f(pose.hop, pose.pick, ...)).  No other code needs to change.
// ============================================================================
namespace Chars
{
using juce::AffineTransform;
using juce::Colour;
using juce::Graphics;
using juce::Path;
using juce::PathStrokeType;
using juce::Point;

struct Pose
{
    float headRot = 0, headY = 0, bodyX = 0, bodyRot = 0, breath = 1, hair = 0, blink = 0,
          mouth = 0.1f, armA = 0, armB = 0, hop = 0, pick = 0, guitarTilt = 0, fret = 0.5f;
};

class Animator
{
public:
    // Called ONLY when a different preset gets selected. Restarts smoothly if already playing.
    void trigger() { reactClock = 0.0; reacting = true; }

    void update (double dt)
    {
        clock += dt;
        reactClock += dt;
        if (reacting && reactClock > reactLen) reacting = false;

        const float target = reacting ? 1.f : 0.f;
        const float rate = target > weight ? 18.f : 5.f;       // snap in, ease out
        weight += (target - weight) * (1.f - std::exp (-rate * (float) dt));

        stepBlink (fTimer, fPhase, (float) dt);
        stepBlink (mTimer, mPhase, (float) dt);
    }

    Pose female() const
    {
        const float t = (float) clock, w = weight, rt = (float) reactClock;
        Pose p;
        p.headRot = 0.035f * std::sin (t * 1.6f);
        p.headY   = 1.5f * std::sin (t * 2.1f);
        p.bodyX   = 3.f * std::sin (t * 1.2f);
        p.bodyRot = 0.02f * std::sin (t * 2.4f);
        p.breath  = 1.f + 0.012f * std::sin (t * 2.3f);
        p.hair    = 0.5f * std::sin (t * 1.4f);
        p.hop     = 2.f * std::abs (std::sin (t * 2.4f));
        p.mouth   = 0.12f + 0.06f * std::sin (t * 2.4f);
        p.armA    = 0.1f * std::sin (t * 1.8f);
        p.armB    = 0.3f * std::sin (t * 2.4f);
        p.blink   = blinkAmt (fPhase);

        // singing pose near the end of the reaction
        const float sing = w * smooth ((rt - 0.7f) / 0.4f);
        auto L = [w] (float a, float b) { return a + (b - a) * w; };
        p.headRot = L (p.headRot, 0.40f * std::sin (rt * 19.f)) - 0.18f * sing;
        p.hair    = L (p.hair,    1.2f * std::sin (rt * 15.f + 0.8f));
        p.hop     = L (p.hop,     14.f * std::abs (std::sin (rt * 9.5f)));
        p.bodyRot = L (p.bodyRot, 0.09f * std::sin (rt * 9.5f));
        p.bodyX   = L (p.bodyX,   12.f * std::sin (rt * 9.5f));
        p.armB    = L (p.armB,    std::sin (rt * 9.5f + 1.f));
        p.mouth   = L (p.mouth,   0.55f + 0.45f * std::sin (rt * 13.f));
        p.mouth   = p.mouth + (1.f - p.mouth) * sing;
        p.armA    = p.armA + (1.f - p.armA) * sing;
        p.blink   = L (p.blink, 0.f);
        return p;
    }

    Pose male() const
    {
        const float t = (float) clock, w = weight, rt = (float) reactClock;
        Pose p;
        p.headRot    = 0.03f * std::sin (t * 1.5f + 1.f);
        p.headY      = 1.2f * std::sin (t * 2.0f);
        p.bodyX      = 2.f * std::sin (t * 1.1f + 0.5f);
        p.bodyRot    = 0.015f * std::sin (t * 1.9f);
        p.breath     = 1.f + 0.012f * std::sin (t * 2.2f + 0.7f);
        p.hair       = 0.4f * std::sin (t * 1.3f);
        p.pick       = 0.15f * std::sin (t * 2.0f);
        p.fret       = 0.5f + 0.5f * std::sin (t * 0.8f);
        p.guitarTilt = 0.01f * std::sin (t * 1.4f);
        p.hop        = 1.f * std::abs (std::sin (t * 2.0f));
        p.blink      = blinkAmt (mPhase);

        auto L = [w] (float a, float b) { return a + (b - a) * w; };
        p.pick       = L (p.pick, std::sin (rt * 30.f));              // aggressive strumming
        p.headRot    = L (p.headRot, 0.30f * std::sin (rt * 12.f));   // head banging
        p.headY      = L (p.headY, 8.f * std::sin (rt * 12.f));
        p.bodyRot    = L (p.bodyRot, 0.06f * std::sin (rt * 12.f));
        p.bodyX      = L (p.bodyX, 6.f * std::sin (rt * 12.f));
        p.guitarTilt = L (p.guitarTilt, 0.06f * std::sin (rt * 12.f + 0.5f));
        p.hop        = L (p.hop, 6.f * std::abs (std::sin (rt * 12.f)));
        p.fret       = L (p.fret, 0.5f + 0.5f * std::sin (rt * 6.f));
        p.hair       = L (p.hair, std::sin (rt * 12.f + 0.5f));
        p.blink      = L (p.blink, 0.f);
        return p;
    }

private:
    static float smooth (float x) { x = juce::jlimit (0.f, 1.f, x); return x * x * (3.f - 2.f * x); }

    void stepBlink (float& timer, float& phase, float dt)
    {
        if (phase >= 0.f)
        {
            phase += dt / 0.16f;
            if (phase >= 1.f) { phase = -1.f; timer = 2.f + rng.nextFloat() * 3.f; }
        }
        else
        {
            timer -= dt;
            if (timer <= 0.f) phase = 0.f;
        }
    }
    static float blinkAmt (float phase)
    {
        return phase < 0.f ? 0.f : std::sin (phase * juce::MathConstants<float>::pi);
    }

    static constexpr double reactLen = 1.6;
    double clock = 0.0, reactClock = 100.0;   // large start value: no reaction at start-up
    bool reacting = false;
    float weight = 0.f;
    float fTimer = 1.5f, fPhase = -1.f, mTimer = 3.0f, mPhase = -1.f;
    juce::Random rng { 1234 };
};

// ---------------------------------------------------------------------------
//  Drawing helpers
// ---------------------------------------------------------------------------
inline const Colour ink { 0xff0a0614 };

inline Path poly (std::initializer_list<Point<float>> pts)
{
    Path p;
    bool first = true;
    for (auto& q : pts)
    {
        if (first) { p.startNewSubPath (q); first = false; }
        else p.lineTo (q);
    }
    p.closeSubPath();
    return p;
}

inline void fillInk (Graphics& g, const Path& p, Colour fill, float lw = 2.4f)
{
    g.setColour (fill);
    g.fillPath (p);
    g.setColour (ink);
    g.strokePath (p, PathStrokeType (lw, PathStrokeType::mitered));
}

// two-segment limb with ink outline
inline void limb (Graphics& g, Point<float> a, Point<float> b, Point<float> c, float w, Colour col)
{
    Path p;
    p.startNewSubPath (a);
    p.lineTo (b);
    p.lineTo (c);
    g.setColour (ink);
    g.strokePath (p, PathStrokeType (w + 4.f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (col);
    g.strokePath (p, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));
}

inline void fist (Graphics& g, Point<float> c, float r, Colour col)
{
    g.setColour (col);
    g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
    g.setColour (ink);
    g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 2.f);
}

inline void studdedBand (Graphics& g, Point<float> c, float w, float h)
{
    g.setColour (Colour (0xff15101f));
    g.fillRoundedRectangle (c.x - w / 2, c.y - h / 2, w, h, 2.f);
    g.setColour (ink);
    g.drawRoundedRectangle (c.x - w / 2, c.y - h / 2, w, h, 2.f, 1.5f);
    g.setColour (juce::Colours::silver);
    for (float x = c.x - w / 2 + 3.f; x < c.x + w / 2 - 2.f; x += 5.f)
        g.fillEllipse (x - 1.2f, c.y - 1.2f, 2.4f, 2.4f);
}

// ---------------------------------------------------------------------------
//  FEMALE SINGER  (local space: 300 x 340)
// ---------------------------------------------------------------------------
inline void drawFemale (Graphics& g, const Pose& p)
{
    const Colour skin (0xfff7d9c4), skinSh (0xffdcae96), blonde (0xfff6d86e), blondeSh (0xffc8a23c),
                 pink (0xfff0407e), black (0xff15101f), navy (0xff1c1236);
    const float s = p.hair;

    g.saveState();
    g.addTransform (AffineTransform::translation (p.bodyX, -p.hop));
    g.addTransform (AffineTransform::rotation (p.bodyRot, 150.f, 330.f));

    const auto headT = AffineTransform::translation (0.f, p.headY).rotated (p.headRot, 150.f, 160.f);

    // ---- back hair (swings) ----
    {
        g.saveState();
        g.addTransform (headT);
        struct HP { float x, y, k; };
        static const HP hl[] = { { 108, 40, 0 }, { 82, 95, 2 }, { 66, 150, 6 }, { 56, 215, 16 },
                                 { 80, 195, 12 }, { 86, 248, 14 }, { 104, 200, 10 }, { 114, 150, 2 } };
        Path back;
        bool first = true;
        for (auto& h : hl)
        {
            const Point<float> q (h.x + s * h.k, h.y);
            if (first) { back.startNewSubPath (q); first = false; } else back.lineTo (q);
        }
        for (int i = (int) std::size (hl) - 1; i >= 0; --i)
            back.lineTo (300.f - hl[i].x + s * hl[i].k, hl[i].y);
        back.closeSubPath();
        fillInk (g, back, blondeSh);
        fillInk (g, poly ({ { 60 + s * 10, 150 }, { 72 + s * 10, 146 }, { 70 + s * 16, 208 }, { 57 + s * 16, 214 } }), pink, 1.6f);
        fillInk (g, poly ({ { 240 + s * 10, 150 }, { 228 + s * 10, 146 }, { 230 + s * 16, 208 }, { 243 + s * 16, 214 } }), pink, 1.6f);
        g.restoreState();
    }

    // ---- legs / belt ----
    fillInk (g, poly ({ { 98, 262 }, { 202, 262 }, { 232, 336 }, { 68, 336 } }), navy);
    fillInk (g, poly ({ { 96, 258 }, { 204, 258 }, { 206, 270 }, { 94, 270 } }), black);
    g.setColour (juce::Colours::silver);
    for (float x = 104.f; x < 200.f; x += 14.f) g.fillEllipse (x - 2.f, 262.f, 4.f, 4.f);

    // ---- torso (breathing) ----
    g.saveState();
    g.addTransform (AffineTransform::scale (1.f, p.breath, 150.f, 215.f));
    fillInk (g, poly ({ { 116, 205 }, { 184, 205 }, { 182, 262 }, { 118, 262 } }), skin);
    fillInk (g, poly ({ { 110, 162 }, { 190, 162 }, { 200, 205 }, { 100, 205 } }), black);
    Path star;
    star.addStar ({ 150.f, 185.f }, 5, 6.f, 14.f);
    g.setColour (juce::Colours::white);
    g.fillPath (star);
    g.restoreState();

    // ---- free arm (left side of picture) ----
    {
        const Point<float> hand (88.f + p.armB * 16.f, 272.f);
        limb (g, { 112, 172 }, { 86.f + p.armB * 6.f, 222.f }, hand, 13.f, skin);
        fist (g, hand, 8.f, skin);
        studdedBand (g, { hand.x, hand.y - 14.f }, 22.f, 13.f);
    }

    // ---- neck + head ----
    fillInk (g, poly ({ { 138, 140 }, { 162, 140 }, { 164, 166 }, { 136, 166 } }), skinSh);
    {
        g.saveState();
        g.addTransform (headT);

        fillInk (g, poly ({ { 112, 62 }, { 188, 62 }, { 194, 104 }, { 174, 146 }, { 150, 158 }, { 126, 146 }, { 106, 104 } }), skin);
        g.setColour (skinSh.withAlpha (0.5f));
        g.fillPath (poly ({ { 106, 104 }, { 126, 146 }, { 134, 112 } }));

        // eyes
        const float eh = 13.f * (1.f - p.blink) + 1.5f;
        for (float ex : { 131.f, 169.f })
        {
            const float ey = 100.f;
            g.setColour (juce::Colours::white);
            g.fillEllipse (ex - 11.f, ey - eh / 2, 22.f, eh);
            if (eh > 5.f)
            {
                const float ih = juce::jmin (11.f, eh);
                g.setColour (Colour (0xff2f86f0));
                g.fillEllipse (ex - 5.5f, ey - ih / 2, 11.f, ih);
                g.setColour (ink);
                g.fillEllipse (ex - 2.5f, ey - 2.5f, 5.f, 5.f);
            }
            g.setColour (ink);
            g.drawLine (ex - 13.f, ey - eh / 2, ex + 12.f, ey - eh / 2, 3.5f);   // eyeliner
        }
        g.setColour (ink);
        g.drawLine (115.f, 82.f, 143.f, 92.f, 3.5f);     // angry brows
        g.drawLine (185.f, 82.f, 157.f, 92.f, 3.5f);
        g.drawLine (150.f, 112.f, 147.f, 124.f, 1.6f);   // nose

        // mouth (singing)
        const float mh = 3.f + p.mouth * 20.f;
        g.setColour (Colour (0xff3a0a1e));
        g.fillEllipse (141.f, 133.f, 20.f, mh);
        if (mh > 10.f)
        {
            g.setColour (pink);
            g.fillEllipse (146.f, 133.f + mh * 0.55f, 12.f, mh * 0.4f);
        }
        g.setColour (ink);
        g.drawEllipse (141.f, 133.f, 20.f, mh, 1.8f);

        // choker
        g.setColour (black);
        g.fillRect (138.f, 152.f, 24.f, 5.f);

        // hair on top: cap, side-swept bangs, streaks
        Path cap;
        cap.startNewSubPath (104, 84);  cap.lineTo (108, 40);  cap.lineTo (130, 18);  cap.lineTo (160, 14);
        cap.lineTo (190, 30);           cap.lineTo (198, 84);  cap.lineTo (186, 66);  cap.lineTo (176, 92);
        cap.lineTo (160, 62);           cap.lineTo (140, 88);  cap.lineTo (124, 64);  cap.lineTo (114, 96);
        cap.closeSubPath();
        fillInk (g, cap, blonde);
        fillInk (g, poly ({ { 104, 84 }, { 120, 60 }, { 150, 50 }, { 130, 96 }, { 116, 80 }, { 108, 122 } }), blonde);
        fillInk (g, poly ({ { 176, 38 }, { 187, 42 }, { 197, 100 }, { 185, 92 } }), pink, 1.6f);
        fillInk (g, poly ({ { 196, 84 }, { 214.f + s * 6.f, 66 }, { 204, 100 } }), blonde, 1.6f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawLine (130.f, 30.f, 160.f, 26.f, 3.f);

        g.restoreState();
    }

    // ---- microphone arm (in front of everything) ----
    {
        const Point<float> hand (186.f, 160.f - p.armA * 8.f);
        limb (g, { 190, 172 }, { 226.f, 214.f - p.armA * 10.f }, hand, 13.f, skin);

        Path cable;
        cable.startNewSubPath (hand.x + 4.f, hand.y + 8.f);
        cable.quadraticTo (262.f, 230.f, 250.f, 334.f);
        g.setColour (ink);
        g.strokePath (cable, PathStrokeType (2.5f));

        g.setColour (ink);
        g.drawLine (hand.x, hand.y, hand.x - 14.f, hand.y - 22.f, 11.f);
        g.setColour (Colour (0xff2a2438));
        g.drawLine (hand.x, hand.y, hand.x - 14.f, hand.y - 22.f, 7.f);
        const Point<float> ball (hand.x - 18.f, hand.y - 30.f);
        g.setColour (Colour (0xff9a9ab0));
        g.fillEllipse (ball.x - 10.f, ball.y - 10.f, 20.f, 20.f);
        g.setColour (ink);
        g.drawEllipse (ball.x - 10.f, ball.y - 10.f, 20.f, 20.f, 2.f);
        g.drawLine (ball.x - 7.f, ball.y - 3.f, ball.x + 7.f, ball.y - 3.f, 1.f);
        g.drawLine (ball.x - 7.f, ball.y + 3.f, ball.x + 7.f, ball.y + 3.f, 1.f);

        fist (g, hand, 8.f, skin);
        studdedBand (g, { hand.x + 12.f, hand.y + 12.f }, 22.f, 13.f);
    }

    g.restoreState();
}

// ---------------------------------------------------------------------------
//  MALE GUITARIST  (local space: 300 x 340)
// ---------------------------------------------------------------------------
inline void drawMale (Graphics& g, const Pose& p)
{
    const Colour skin (0xfff0d0bc), skinSh (0xffd4a48c), black (0xff15101f), navy (0xff1c1236),
                 hairC (0xff8a3cf0), pink (0xfff0407e), purple (0xff7a34d8);
    const float s = p.hair;

    g.saveState();
    g.addTransform (AffineTransform::translation (p.bodyX, -p.hop));
    g.addTransform (AffineTransform::rotation (p.bodyRot, 150.f, 330.f));

    // legs
    fillInk (g, poly ({ { 96, 292 }, { 204, 292 }, { 212, 340 }, { 88, 340 } }), navy);

    // torso + skull print
    g.saveState();
    g.addTransform (AffineTransform::scale (1.f, p.breath, 150.f, 230.f));
    fillInk (g, poly ({ { 104, 164 }, { 196, 164 }, { 206, 296 }, { 94, 296 } }), black);
    g.setColour (juce::Colours::white);
    g.fillEllipse (124.f, 200.f, 26.f, 24.f);
    g.fillRect (131.f, 220.f, 12.f, 8.f);
    g.setColour (black);
    g.fillEllipse (130.f, 208.f, 6.f, 7.f);
    g.fillEllipse (141.f, 208.f, 6.f, 7.f);
    g.restoreState();

    // neck + strap
    fillInk (g, poly ({ { 138, 142 }, { 162, 142 }, { 164, 168 }, { 136, 168 } }), skinSh);
    g.setColour (Colour (0xff0f0a1c));
    g.drawLine (186.f, 164.f, 112.f, 284.f, 9.f);

    // ---- guitar ----
    g.saveState();
    g.addTransform (AffineTransform::rotation (p.guitarTilt, 150.f, 270.f));
    {
        Path neck;
        neck.startNewSubPath (155.f, 266.f);
        neck.lineTo (292.f, 178.f);
        g.setColour (ink);
        g.strokePath (neck, PathStrokeType (13.f));
        g.setColour (Colour (0xff3b2433));
        g.strokePath (neck, PathStrokeType (9.f));

        g.setColour (juce::Colours::white.withAlpha (0.5f));
        for (int k = 1; k <= 8; ++k)
        {
            const float u = (float) k / 9.f;
            const float x = 155.f + 137.f * u, y = 266.f - 88.f * u;
            g.drawLine (x - 2.5f, y - 3.9f, x + 2.5f, y + 3.9f, 1.f);
        }

        fillInk (g, poly ({ { 286, 170 }, { 306, 160 }, { 312, 176 }, { 294, 190 } }), purple);

        Path body = poly ({ { 86, 300 }, { 128, 236 }, { 168, 252 }, { 150, 282 }, { 196, 318 }, { 146, 326 } });
        fillInk (g, body, Colour (0xff2a1450));
        g.setColour (pink.withAlpha (0.8f));
        g.strokePath (body, PathStrokeType (1.4f, PathStrokeType::mitered));
        g.setColour (Colour (0xffc0c0d0));
        g.fillPath (poly ({ { 132, 262 }, { 148, 254 }, { 152, 261 }, { 136, 270 } }));
        g.fillPath (poly ({ { 120, 280 }, { 136, 272 }, { 140, 279 }, { 124, 288 } }));
    }
    g.restoreState();

    // ---- picking arm (strums) ----
    {
        const Point<float> hand (136.f + p.pick * 10.f, 280.f + p.pick * 22.f);
        limb (g, { 110, 172 }, { 92.f + p.pick * 4.f, 232.f }, hand, 13.f, skin);
        studdedBand (g, { hand.x - 8.f, hand.y - 18.f }, 22.f, 12.f);
        fist (g, hand, 8.f, skin);
        g.setColour (pink);
        g.fillPath (poly ({ { hand.x + 4, hand.y + 4 }, { hand.x + 12, hand.y + 2 }, { hand.x + 8, hand.y + 12 } }));
    }

    // ---- fretting arm (slides along neck) ----
    {
        const float u = 0.55f + 0.2f * p.fret;
        const Point<float> fh (155.f + 137.f * u, 266.f - 88.f * u);
        limb (g, { 190, 172 }, { 214.f, 248.f }, fh, 13.f, skin);
        fist (g, fh, 8.f, skin);
    }

    // ---- head ----
    {
        g.saveState();
        g.addTransform (AffineTransform::translation (0.f, p.headY).rotated (p.headRot, 150.f, 168.f));

        fillInk (g, poly ({ { 112, 60 }, { 188, 60 }, { 192, 108 }, { 170, 150 }, { 150, 158 }, { 130, 150 }, { 108, 108 } }), skin);
        g.setColour (skinSh.withAlpha (0.5f));
        g.fillPath (poly ({ { 108, 108 }, { 130, 150 }, { 136, 114 } }));

        const float eh = 9.f * (1.f - p.blink) + 1.5f;
        for (float ex : { 132.f, 168.f })
        {
            g.setColour (juce::Colours::white);
            g.fillEllipse (ex - 10.f, 102.f - eh / 2, 20.f, eh);
            if (eh > 5.f)
            {
                g.setColour (Colour (0xff3a8ff0));
                g.fillEllipse (ex - 4.f, 102.f - eh / 2 + 0.5f, 8.f, eh - 1.f);
                g.setColour (ink);
                g.fillEllipse (ex - 2.f, 100.f, 4.f, 4.f);
            }
        }
        g.setColour (ink);
        g.drawLine (114.f, 86.f, 143.f, 96.f, 4.f);       // angry brows
        g.drawLine (186.f, 86.f, 157.f, 96.f, 4.f);
        g.drawLine (150.f, 114.f, 147.f, 126.f, 1.6f);

        Path mouth;
        mouth.startNewSubPath (140.f, 140.f);
        mouth.quadraticTo (150.f, 134.f, 160.f, 140.f);
        g.strokePath (mouth, PathStrokeType (2.4f));

        g.setColour (black);
        g.fillRect (136.f, 152.f, 28.f, 5.f);
        g.setColour (juce::Colours::silver);
        g.fillEllipse (148.f, 152.f, 4.f, 4.f);

        // spiky purple hair (tips sway)
        struct HP { float x, y, k; };
        static const HP hp[] = {
            { 104, 96, 0 }, { 96, 62, 1 }, { 88, 40, 4 }, { 110, 48, 1 }, { 108, 14, 5 }, { 128, 38, 1 },
            { 140, 2, 6 }, { 152, 34, 1 }, { 172, -4, 6 }, { 176, 34, 1 }, { 200, 12, 5 }, { 196, 50, 1 },
            { 214, 46, 4 }, { 196, 84, 1 }, { 192, 100, 0 }, { 184, 70, 0 }, { 170, 58, 0 }, { 150, 66, 0 },
            { 128, 60, 0 }, { 114, 70, 0 } };
        Path hair;
        bool first = true;
        for (auto& h : hp)
        {
            const Point<float> q (h.x + s * h.k, h.y);
            if (first) { hair.startNewSubPath (q); first = false; } else hair.lineTo (q);
        }
        hair.closeSubPath();
        fillInk (g, hair, hairC);
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.drawLine (130.f, 40.f, 150.f, 36.f, 3.f);

        g.restoreState();
    }

    g.restoreState();
}
} // namespace Chars
