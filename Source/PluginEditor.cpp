#include "PluginEditor.h"

using namespace juce;
using namespace UI;

// ===========================================================================
//  Small drawing helpers (design space = 1536 x 1024)
// ===========================================================================
namespace
{
void starShape (Graphics& g, float cx, float cy, float r, Colour c, bool fill, float rot = 0.f)
{
    Path p;
    p.addStar ({ cx, cy }, 5, r * 0.45f, r, rot);
    g.setColour (c);
    if (fill) g.fillPath (p); else g.strokePath (p, PathStrokeType (2.f));
}

void heart (Graphics& g, float cx, float cy, float s, Colour c, bool broken)
{
    Path p;
    p.startNewSubPath (cx, cy + s * 0.9f);
    p.cubicTo (cx - s * 1.4f, cy + s * 0.1f, cx - s * 0.9f, cy - s * 0.9f, cx, cy - s * 0.3f);
    p.cubicTo (cx + s * 0.9f, cy - s * 0.9f, cx + s * 1.4f, cy + s * 0.1f, cx, cy + s * 0.9f);
    p.closeSubPath();
    g.setColour (c.withAlpha (0.35f));
    g.fillPath (p);
    g.setColour (c);
    g.strokePath (p, PathStrokeType (3.f));
    if (broken)
    {
        Path crack;
        crack.startNewSubPath (cx + s * 0.1f, cy - s * 0.4f);
        crack.lineTo (cx - s * 0.25f, cy);
        crack.lineTo (cx + s * 0.2f, cy + s * 0.25f);
        crack.lineTo (cx - s * 0.1f, cy + s * 0.8f);
        g.setColour (Colours::white.withAlpha (0.85f));
        g.strokePath (crack, PathStrokeType (2.5f));
    }
}

void skull (Graphics& g, float cx, float cy, float r, Colour c)
{
    g.setColour (c);
    g.fillEllipse (cx - r, cy - r, r * 2, r * 1.8f);
    g.fillRect (cx - r * 0.5f, cy + r * 0.6f, r, r * 0.7f);
    g.setColour (Colour (0xff10103a));
    g.fillEllipse (cx - r * 0.55f, cy - r * 0.2f, r * 0.45f, r * 0.5f);
    g.fillEllipse (cx + r * 0.10f, cy - r * 0.2f, r * 0.45f, r * 0.5f);
    g.fillRect (cx - 1.f, cy + r * 0.45f, 2.f, r * 0.3f);
}

void sticker (Graphics& g, const String& txt, float x, float y, float angle, float size, Colour c)
{
    g.saveState();
    g.addTransform (AffineTransform::rotation (angle, x, y));
    g.setFont (scrawl (size));
    g.setColour (c);
    g.drawMultiLineText (txt, (int) x, (int) y, 320);
    g.restoreState();
}

void drawPanel (Graphics& g, Rectangle<float> r, const String& title, bool star = true)
{
    g.setGradientFill (ColourGradient (Colour (0xff181868), r.getX(), r.getY(), Colour (0xff090930), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (Colour (0xff05051a));
    g.drawRoundedRectangle (r.expanded (1.5f), 9.f, 3.f);
    g.setColour (cBlue);
    g.drawRoundedRectangle (r, 8.f, 2.f);
    g.setColour (cBlue.withAlpha (0.3f));
    g.drawRoundedRectangle (r.reduced (4.f), 6.f, 1.f);

    if (title.isNotEmpty())
    {
        if (star) starShape (g, r.getX() + 28.f, r.getY() + 26.f, 12.f, cViolet, true);
        g.setColour (cText);
        g.setFont (tahoma (21.f, true));
        g.drawFittedText (title, Rectangle<int> ((int) r.getX() + (star ? 46 : 12), (int) r.getY() + 10,
                                                 (int) r.getWidth() - (star ? 96 : 24), 32),
                          Justification::centredLeft, 1, 0.7f);
    }
}

// Irregular, hand-made looking lettering: every glyph gets its own rotation/size/offset.
void distortedWord (Graphics& g, const String& w, Point<float> o, float size, Colour fill, int seed)
{
    Random r (seed);
    for (int i = 0; i < w.length(); ++i)
    {
        const float ang = (r.nextFloat() - 0.5f) * 0.5f;
        const float dy = (r.nextFloat() - 0.5f) * size * 0.2f;
        const float sz = size * (0.9f + r.nextFloat() * 0.25f);
        const String ch = w.substring (i, i + 1);

        g.saveState();
        g.addTransform (AffineTransform::rotation (ang).translated (o.x + (float) i * size * 0.68f, o.y + dy));
        g.setFont (punkFont (sz));
        const int s = (int) sz;
        const Rectangle<int> rc (-s, -s, s * 2, s * 2);

        g.setColour (Colour (0xffb45cff).withAlpha (0.45f));
        for (auto off : { Point<int> (-6, 0), Point<int> (6, 0), Point<int> (0, -6), Point<int> (0, 6), Point<int> (5, 5) })
            g.drawText (ch, rc.translated (off.x, off.y), Justification::centred);
        g.setColour (Colours::black);
        for (auto off : { Point<int> (-3, -3), Point<int> (3, -3), Point<int> (-3, 3), Point<int> (3, 3) })
            g.drawText (ch, rc.translated (off.x, off.y), Justification::centred);
        g.setColour (fill);
        g.drawText (ch, rc, Justification::centred);
        g.restoreState();
    }
}

String dbText (double v)  { return String (v, 1) + " dB"; }
String pctText (double v) { return String (roundToInt (v * 100.0)) + "%"; }
} // namespace

// ===========================================================================
MSCVoxAudioProcessorEditor::MSCVoxAudioProcessorEditor (MSCVoxAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    texture = makeTexture();
    auto& s = proc.apvts;

    // ---------------- title bar ----------------
    for (int i = 0; i < (int) MSC::presets().size(); ++i)
        presetBox.addItem (String::formatted ("%02d - ", i + 1) + MSC::presets()[(size_t) i].name, i + 1);
    presetBox.setJustificationType (Justification::centred);
    presetBox.onChange = [this] { select (presetBox.getSelectedId() - 1); };
    add (presetBox, { 610, 10, 320, 32 });

    prevBtn.setComponentID ("arrowL");
    nextBtn.setComponentID ("arrowR");
    prevBtn.onClick = [this] { select ((proc.presetIndex.load() + 7) % 8); };
    nextBtn.onClick = [this] { select ((proc.presetIndex.load() + 1) % 8); };
    add (prevBtn, { 548, 10, 52, 32 });
    add (nextBtn, { 940, 10, 52, 32 });

    saveBtn.setButtonText ("SAVE");
    loadBtn.setButtonText ("LOAD");
    gearBtn.setComponentID ("gear");
    saveBtn.onClick = [this] { saveUserPreset(); };
    loadBtn.onClick = [this] { loadUserPreset(); };
    gearBtn.onClick = [this] { showSettings(); };
    add (saveBtn, { 1020, 10, 86, 32 });
    add (loadBtn, { 1114, 10, 86, 32 });
    add (gearBtn, { 1208, 10, 36, 32 });
    addToggle ("bypass", "BYPASS", { 1252, 10, 92, 32 }, "bypass");

    presetList.onSelect = [this] (int i) { select (i); };
    add (presetList, { 1118, 108, 284, 262 });

    // ---------------- compressor ----------------
    addPower ("compOn", 388, 434);
    addKnob ("compThresh", "THRESH",  dbText, 178, 466);
    addKnob ("compRatio",  "RATIO",   [] (double v) { return String (v, 1) + ":1"; }, 248, 466);
    addKnob ("compAttack", "ATTACK",  [] (double v) { return String (v, v < 10 ? 1 : 0) + " ms"; }, 318, 466);
    addKnob ("compRelease","RELEASE", [] (double v) { return String (roundToInt (v)) + " ms"; }, 386, 466);
    addKnob ("compMakeup", "MAKEUP",  dbText, 262, 582, 70, 94);

    // ---------------- EQ ----------------
    addPower ("eqOn", 722, 434);
    addKnob ("eqLow",     "LOW",      dbText, 487, 462);
    addKnob ("eqLowMid",  "LOW MID",  dbText, 561, 462);
    addKnob ("eqHighMid", "HIGH MID", dbText, 635, 462);
    addKnob ("eqHigh",    "HIGH",     dbText, 710, 462);
    addKnob ("hpf",       "HPF",      [] (double v) { return String (roundToInt (v)) + " Hz"; }, 485, 572, 70, 100);
    eqView = std::make_unique<LiveView> ([this] (Graphics& g, Rectangle<float> b)
    {
        g.setColour (Colour (0xff060626)); g.fillRoundedRectangle (b, 3.f);
        g.setColour (cBlue.withAlpha (0.5f)); g.drawRoundedRectangle (b, 3.f, 1.2f);
        g.setColour (cBlue.withAlpha (0.25f)); g.drawHorizontalLine ((int) b.getCentreY(), b.getX(), b.getRight());
        auto P = [this] (MSC::P id) { return proc.apvts.getRawParameterValue (MSC::defs()[id].id)->load(); };
        const double fs = 48000.0;
        const bool on = P (MSC::eqOn) > 0.5f;
        DSP::Biquad hp, lo, lm, hm, hi;
        hp.highpass (fs, P (MSC::hpf), 0.707);
        lo.lowShelf (fs, 100.0, on ? P (MSC::eqLow) : 0.f);
        lm.peak (fs, 400.0, 1.0, on ? P (MSC::eqLowMid) : 0.f);
        hm.peak (fs, 2500.0, 1.0, on ? P (MSC::eqHighMid) : 0.f);
        hi.highShelf (fs, 8000.0, on ? P (MSC::eqHigh) : 0.f);
        Path line;
        for (int i = 0; i <= 100; ++i)
        {
            const double f = 20.0 * std::pow (1000.0, (double) i / 100.0);
            double db = hp.magnitudeDb (f, fs) + lo.magnitudeDb (f, fs) + lm.magnitudeDb (f, fs)
                      + hm.magnitudeDb (f, fs) + hi.magnitudeDb (f, fs);
            db = jlimit (-18.0, 18.0, db);
            const float x = b.getX() + b.getWidth() * (float) i / 100.f;
            const float y = b.getCentreY() - (float) db / 18.f * b.getHeight() * 0.45f;
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
        }
        Path fillP (line);
        fillP.lineTo (b.getRight(), b.getBottom());
        fillP.lineTo (b.getX(), b.getBottom());
        fillP.closeSubPath();
        g.setColour (cViolet.withAlpha (0.25f)); g.fillPath (fillP);
        g.setColour (cViolet); g.strokePath (line, PathStrokeType (2.f));
    });
    add (*eqView, { 532, 578, 210, 80 });

    // ---------------- air / presence ----------------
    addPower ("airOn", 908, 434);
    addKnob ("air", "AMOUNT", pctText, 856, 470, 123, 190, 18.f);
    addPower ("presOn", 1046, 434);
    addKnob ("pres", "AMOUNT", pctText, 1014, 470, 112, 190, 18.f);

    // ---------------- de-esser ----------------
    addPower ("dsOn", 1262, 434);
    addKnob ("dsFreq",   "FREQ",   [] (double v) { return String (v / 1000.0, 1) + " kHz"; }, 1131, 480);
    addKnob ("dsThresh", "THRESH", dbText, 1199, 480);
    addKnob ("dsAmt",    "AMOUNT", pctText, 1265, 480);
    dsView = std::make_unique<LiveView> ([this] (Graphics& g, Rectangle<float> b)
    {
        // activity display driven by the output level (decorative, not an FFT)
        const float t = (float) (Time::getMillisecondCounterHiRes() * 0.001);
        const float amp = 0.15f + 0.85f * jlimit (0.f, 1.f, (outLevel + 48.f) / 48.f);
        Path p;
        for (int i = 0; i <= 70; ++i)
        {
            const float x = b.getX() + b.getWidth() * (float) i / 70.f;
            const float n = std::sin ((float) i * 1.7f + t * 9.f) * std::sin ((float) i * 0.31f + t * 3.f);
            const float y = b.getBottom() - 4.f - std::abs (n) * b.getHeight() * 0.85f * amp;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (cBlue.withAlpha (0.9f));
        g.strokePath (p, PathStrokeType (1.6f));
    });
    add (*dsView, { 1110, 612, 170, 56 });

    // ---------------- saturation / distortion ----------------
    addPower ("satOn", 386, 700);
    addKnob ("satDrive", "DRIVE", pctText, 190, 736);
    addKnob ("satMix",   "MIX",   pctText, 268, 736);
    satBox.addItemList (StringArray::fromTokens (MSC::defs()[MSC::satType].choices, "|", ""), 1);
    satAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "satType", satBox);
    add (satBox, { 310, 792, 98, 32 });

    // ---------------- reverb ----------------
    addPower ("revOn", 668, 700);
    addKnob ("revSize",  "SIZE",      pctText, 462, 736, 68);
    addKnob ("revDecay", "DECAY",     [] (double v) { return String (v, 1) + " s"; }, 527, 736, 68);
    addKnob ("revDamp",  "DAMPING",   pctText, 593, 736, 68);
    addKnob ("revPre",   "PRE-DELAY", [] (double v) { return String (roundToInt (v)) + " ms"; }, 662, 736, 68);
    revBox.addItemList (StringArray::fromTokens (MSC::defs()[MSC::revType].choices, "|", ""), 1);
    revAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "revType", revBox);
    add (revBox, { 442, 838, 126, 32 });
    revView = std::make_unique<LiveView> ([] (Graphics& g, Rectangle<float> b)
    {
        g.setColour (Colour (0xff060626)); g.fillRoundedRectangle (b, 3.f);
        g.setColour (cViolet);
        const auto c = b.getCentre();
        const auto o = b.reduced (4.f), i = b.reduced (b.getWidth() * 0.32f, b.getHeight() * 0.3f);
        g.drawRect (o, 1.4f);
        g.drawRect (i, 1.4f);
        g.drawLine (o.getX(), o.getY(), i.getX(), i.getY(), 1.2f);
        g.drawLine (o.getRight(), o.getY(), i.getRight(), i.getY(), 1.2f);
        g.drawLine (o.getX(), o.getBottom(), i.getX(), i.getBottom(), 1.2f);
        g.drawLine (o.getRight(), o.getBottom(), i.getRight(), i.getBottom(), 1.2f);
        g.fillEllipse (c.x - 3.f, c.y - 3.f, 6.f, 6.f);
    });
    add (*revView, { 603, 838, 76, 40 });

    // ---------------- delay ----------------
    addPower ("dlyOn", 924, 700);
    delayKnob = addKnob ("dlyTime", "TIME", [this] (double v)
    {
        const bool sync = proc.apvts.getRawParameterValue ("dlySync")->load() > 0.5f;
        if (sync)
        {
            static const char* n[] = { "1/16", "1/8", "1/4", "1/2", "1/1" };
            return String (n[jmin (4, (int) (v * 5.0))]);
        }
        return String (roundToInt (20.0 + v * 980.0)) + " ms";
    }, 765, 736);
    addKnob ("dlyFb",  "FEEDBACK", pctText, 841, 736);
    addKnob ("dlyMix", "MIX",      pctText, 915, 736);
    addToggle ("dlySync", "SYNC", { 733, 842, 76, 34 });
    toggles.back()->onClick = [this] { if (delayKnob != nullptr) delayKnob->repaint(); };
    addToggle ("dlyStereo", "STEREO", { 822, 842, 92, 34 });

    // ---------------- chorus ----------------
    addPower ("choOn", 1262, 700);
    addKnob ("choRate",  "RATE",  [] (double v) { return String (v, 1) + " Hz"; }, 1018, 736);
    addKnob ("choDepth", "DEPTH", pctText, 1095, 736);
    addKnob ("choWidth", "WIDTH", pctText, 1173, 736);
    addKnob ("choMix",   "MIX",   pctText, 1252, 736);
    choView = std::make_unique<LiveView> ([this] (Graphics& g, Rectangle<float> b)
    {
        const float rate = proc.apvts.getRawParameterValue ("choRate")->load();
        const float depth = proc.apvts.getRawParameterValue ("choDepth")->load();
        const float t = (float) (Time::getMillisecondCounterHiRes() * 0.001) * rate * 2.f;
        for (int k = 0; k < 2; ++k)
        {
            Path p;
            for (int i = 0; i <= 80; ++i)
            {
                const float x = b.getX() + b.getWidth() * (float) i / 80.f;
                const float y = b.getCentreY() + std::sin ((float) i * 0.35f + t + (float) k * 1.2f) * b.getHeight() * 0.4f * (0.3f + depth);
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            g.setColour (k == 0 ? cViolet : cBlue);
            g.strokePath (p, PathStrokeType (1.8f));
        }
    });
    add (*choView, { 1040, 846, 190, 36 });

    // ---------------- side + footer ----------------
    addKnob ("inGain",  "INPUT GAIN",  dbText, 68, 704, 84, 92, 12.f);
    addKnob ("outGain", "OUTPUT GAIN", dbText, 1365, 704, 84, 92, 12.f);
    addKnob ("mix",     "WET / DRY",   pctText, 1365, 812, 84, 86, 12.f);
    add (inMeter,  { 40, 452, 24, 224 });
    add (outMeter, { 1337, 452, 24, 224 });

    vocalBtn.setComponentID ("vocal");
    vocalBtn.setButtonText ("VOCAL CHECK");
    vocalBtn.onStateChange = [this] { proc.vocalCheck.store (vocalBtn.isDown()); };   // hold = hear dry signal
    add (vocalBtn, { 88, 914, 212, 62 });

    waveView = std::make_unique<LiveView> ([this] (Graphics& g, Rectangle<float> b)
    {
        g.setColour (Colour (0xff04041a)); g.fillRoundedRectangle (b, 4.f);
        const int pos = proc.wavePos.load();
        const float mid = b.getCentreY(), bw = b.getWidth() / 256.f;
        for (int i = 0; i < 256; ++i)
        {
            float v = proc.wave[(size_t) ((pos + i) & 255)].load();
            v = std::pow (jlimit (0.f, 1.f, v), 0.6f);
            const float h = v * b.getHeight() * 0.48f;
            g.setColour (Colour (0xffa070ff).withAlpha (0.45f + 0.55f * (float) i / 256.f));
            g.fillRect (b.getX() + (float) i * bw, mid - h, jmax (1.f, bw - 0.6f), jmax (1.f, 2.f * h));
        }
    });
    add (*waveView, { 508, 918, 400, 82 });

    addKnob ("master", "MASTER", dbText, 982, 912, 84, 92, 12.f);
    masterSlider.setSliderStyle (Slider::LinearHorizontal);
    masterSlider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    masterAtt = std::make_unique<APVTS::SliderAttachment> (s, "master", masterSlider);
    add (masterSlider, { 1080, 950, 190, 28 });

    // ---------------- window ----------------
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (1536.0 / 1024.0);
    setResizeLimits (576, 384, 1920, 1280);
    setSize (1152, 768);

    // do NOT trigger an animation when the window opens
    lastCounter = proc.presetChangeCounter.load();
    refreshPresetUI (proc.presetIndex.load());

    lastT = Time::getMillisecondCounterHiRes();
    startTimerHz (fps);
}

MSCVoxAudioProcessorEditor::~MSCVoxAudioProcessorEditor()
{
    stopTimer();
    proc.vocalCheck.store (false);
    setLookAndFeel (nullptr);
}

// ---------------------------------------------------------------------------
void MSCVoxAudioProcessorEditor::add (Component& c, Rectangle<int> r)
{
    addAndMakeVisible (c);
    items.push_back ({ &c, r });
}

UI::Knob* MSCVoxAudioProcessorEditor::addKnob (const char* id, const char* name, std::function<String (double)> fmt,
                                               int cx, int top, int w, int h, float font)
{
    knobs.push_back (std::make_unique<UI::Knob> (proc.apvts, id, name, std::move (fmt), font));
    add (*knobs.back(), { cx - w / 2, top, w, h });
    return knobs.back().get();
}

void MSCVoxAudioProcessorEditor::addToggle (const char* id, const String& text, Rectangle<int> r, const char* compId)
{
    auto t = std::make_unique<ToggleButton> (text);
    t->setComponentID (compId);
    btnAtt.push_back (std::make_unique<APVTS::ButtonAttachment> (proc.apvts, id, *t));
    add (*t, r);
    toggles.push_back (std::move (t));
}

void MSCVoxAudioProcessorEditor::addPower (const char* id, int x, int y)
{
    addToggle (id, "", { x, y, 26, 26 }, "power");
}

// ---------------------------------------------------------------------------
void MSCVoxAudioProcessorEditor::resized()
{
    sc = (float) getWidth() / 1536.f;
    for (auto& it : items)
        it.c->setBounds ((it.r.toFloat() * sc).toNearestInt());
    for (auto& k : knobs) k->setScale (sc);

    charRegion = (Rectangle<float> (380.f, 52.f, 720.f, 358.f) * sc).toNearestInt().expanded (3);

    bg = Image (Image::RGB, jmax (1, getWidth()), jmax (1, getHeight()), true);
    Graphics g (bg);
    g.addTransform (AffineTransform::scale (sc));
    paintStatic (g);
}

void MSCVoxAudioProcessorEditor::paint (Graphics& g)
{
    g.drawImageAt (bg, 0, 0);

    // moving parts: only the two characters
    g.addTransform (AffineTransform::scale (sc));
    g.reduceClipRegion (Rectangle<int> (0, 52, 1536, 358));   // crop at the panel line, like the reference

    g.saveState();
    g.addTransform (AffineTransform::scale (1.08f).translated (405.f, 58.f));
    Chars::drawFemale (g, anim.female());
    g.restoreState();

    g.saveState();
    g.addTransform (AffineTransform::scale (1.08f).translated (770.f, 58.f));
    Chars::drawMale (g, anim.male());
    g.restoreState();
}

// ---------------------------------------------------------------------------
void MSCVoxAudioProcessorEditor::timerCallback()
{
    const double now = Time::getMillisecondCounterHiRes();
    const double dt = jlimit (0.0, 0.1, (now - lastT) * 0.001);
    lastT = now;

    // preset-change animation: ONLY when the processor says a different preset was loaded
    const int c = proc.presetChangeCounter.load();
    if (c != lastCounter) { lastCounter = c; anim.trigger(); }

    const int pi = proc.presetIndex.load();
    if (pi != shownPreset) refreshPresetUI (pi);

    anim.update (dt);
    repaint (charRegion);

    const float inDb  = Decibels::gainToDecibels (proc.inPeak.exchange (0.f), -60.f);
    const float outDb = Decibels::gainToDecibels (proc.outPeak.exchange (0.f), -60.f);
    inMeter.push (inDb);
    outMeter.push (outDb);
    outLevel = jmax (outDb, outLevel - 2.f);

    waveView->repaint();
    dsView->repaint();
    choView->repaint();
    if (++frameCount % 3 == 0) eqView->repaint();
}

void MSCVoxAudioProcessorEditor::select (int idx)
{
    proc.loadPreset (idx);
    refreshPresetUI (proc.presetIndex.load());
}

void MSCVoxAudioProcessorEditor::refreshPresetUI (int idx)
{
    shownPreset = idx;
    presetList.selected = idx;
    presetList.repaint();
    presetBox.setSelectedId (idx + 1, dontSendNotification);
}

// ---------------------------------------------------------------------------
void MSCVoxAudioProcessorEditor::saveUserPreset()
{
    chooser = std::make_unique<FileChooser> ("Save MSC VOX preset",
                                             File::getSpecialLocation (File::userDocumentsDirectory), "*.mscvox");
    SafePointer<MSCVoxAudioProcessorEditor> safe (this);
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles
                              | FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const FileChooser& fc)
    {
        if (safe == nullptr) return;
        const auto f = fc.getResult();
        if (f == File()) return;
        if (auto xml = safe->proc.apvts.copyState().createXml())
            xml->writeTo (f.withFileExtension ("mscvox"));
    });
}

void MSCVoxAudioProcessorEditor::loadUserPreset()
{
    chooser = std::make_unique<FileChooser> ("Load MSC VOX preset",
                                             File::getSpecialLocation (File::userDocumentsDirectory), "*.mscvox");
    SafePointer<MSCVoxAudioProcessorEditor> safe (this);
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [safe] (const FileChooser& fc)
    {
        if (safe == nullptr) return;
        const auto f = fc.getResult();
        if (! f.existsAsFile()) return;
        if (auto xml = XmlDocument::parse (f))
            if (xml->hasTagName (safe->proc.apvts.state.getType()))
            {
                safe->proc.apvts.replaceState (ValueTree::fromXml (*xml));
                safe->proc.presetIndex.store (jlimit (0, 7, (int) safe->proc.apvts.state.getProperty ("preset", 7)));
            }
    });
}

void MSCVoxAudioProcessorEditor::showSettings()
{
    PopupMenu m;
    m.addItem (1, "Animation: 30 FPS", true, fps == 30);
    m.addItem (2, "Animation: 24 FPS", true, fps == 24);
    m.addSeparator();
    m.addItem (3, "Reset input / output / master gain");
    m.addItem (4, "About MSC VOX");

    SafePointer<MSCVoxAudioProcessorEditor> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&gearBtn), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        if (r == 1 || r == 2) { safe->fps = (r == 1) ? 30 : 24; safe->startTimerHz (safe->fps); }
        else if (r == 3)
        {
            for (auto id : { "inGain", "outGain", "master" })
                if (auto* p = safe->proc.apvts.getParameter (id))
                    p->setValueNotifyingHost (p->getDefaultValue());
        }
        else if (r == 4)
            AlertWindow::showMessageBoxAsync (MessageBoxIconType::InfoIcon, "MSC VOX",
                                              "MSC VOX v1.0\nPop-punk vocal chain.\nPOP PUNK 4EVER.");
    });
}

// ---------------------------------------------------------------------------
Image MSCVoxAudioProcessorEditor::makeTexture()
{
    Image t (Image::ARGB, 1536, 1024, true);
    Graphics g (t);
    Random r (42);
    for (int i = 0; i < 3500; ++i)
    {
        g.setColour (Colours::black.withAlpha (r.nextFloat() * 0.35f));
        g.fillRect (r.nextInt (1536), r.nextInt (1024), 1 + r.nextInt (3), 1 + r.nextInt (3));
    }
    for (int i = 0; i < 140; ++i)
    {
        g.setColour ((i % 2 ? Colours::white : Colour (0xffb45cff)).withAlpha (0.05f + r.nextFloat() * 0.07f));
        const float x = r.nextFloat() * 1536.f, y = r.nextFloat() * 1024.f;
        g.drawLine (x, y, x + (r.nextFloat() - 0.5f) * 120.f, y + (r.nextFloat() - 0.5f) * 40.f, 1.f);
    }
    return t;
}

// ===========================================================================
//  Static artwork (rendered once per resize into the cached background image)
// ===========================================================================
void MSCVoxAudioProcessorEditor::paintStatic (Graphics& g)
{
    // base
    g.setGradientFill (ColourGradient (Colour (0xff10104a), 0.f, 0.f, Colour (0xff06061e), 0.f, 1024.f, false));
    g.fillRect (0.f, 0.f, 1536.f, 1024.f);

    // ---------------- stage ----------------
    {
        const Rectangle<float> st (0.f, 52.f, 1536.f, 360.f);
        g.setGradientFill (ColourGradient (Colour (0xff3030c0), 780.f, 60.f, Colour (0xff12103c), 780.f, 410.f, false));
        g.fillRect (st);
        g.setGradientFill (ColourGradient (Colour (0x88b45cff), 780.f, 230.f, Colours::transparentBlack, 1300.f, 230.f, true));
        g.fillRect (st);

        // brick wall
        Random r (7);
        g.setColour (Colour (0x30000020));
        for (float y = 62.f; y < 410.f; y += 26.f)
        {
            g.drawHorizontalLine ((int) y, 0.f, 1536.f);
            const float off = ((int) (y / 26.f) % 2) ? 0.f : 30.f;
            for (float x = off; x < 1536.f; x += 60.f + r.nextFloat() * 20.f)
                g.drawVerticalLine ((int) x, y, y + 26.f);
        }

        // spotlight beams
        g.setColour (Colours::white.withAlpha (0.07f));
        g.fillPath (Chars::poly ({ { 330, 52 }, { 420, 52 }, { 640, 410 }, { 380, 410 } }));
        g.fillPath (Chars::poly ({ { 700, 52 }, { 780, 52 }, { 900, 410 }, { 640, 410 } }));
        g.setColour (Colour (0xffb45cff).withAlpha (0.10f));
        g.fillPath (Chars::poly ({ { 900, 52 }, { 980, 52 }, { 1100, 410 }, { 860, 410 } }));

        // speaker stack behind the guitarist
        g.setColour (Colour (0xff0a0a22));
        g.fillRect (990.f, 310.f, 100.f, 100.f);
        g.setColour (cBlue.withAlpha (0.6f));
        g.drawRect (990.f, 310.f, 100.f, 100.f, 2.f);
        g.drawEllipse (1005.f, 325.f, 70.f, 70.f, 2.f);
        g.drawEllipse (1030.f, 350.f, 20.f, 20.f, 2.f);

        // crown doodle + stars + floor shadows
        Path crown = Chars::poly ({ { 822, 118 }, { 832, 92 }, { 845, 108 }, { 858, 86 }, { 870, 108 }, { 882, 92 }, { 892, 118 } });
        g.setColour (cViolet.withAlpha (0.7f));
        g.strokePath (crown, PathStrokeType (2.f));
        starShape (g, 512.f, 90.f, 18.f, cViolet, false, 0.3f);
        starShape (g, 440.f, 178.f, 30.f, cViolet, false, 0.2f);
        starShape (g, 905.f, 150.f, 14.f, cPink, true);
        starShape (g, 1010.f, 80.f, 12.f, cViolet, true);
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (440.f, 392.f, 260.f, 22.f);
        g.fillEllipse (800.f, 392.f, 300.f, 22.f);
    }

    // ---------------- title bar (Windows XP style) ----------------
    {
        g.setGradientFill (ColourGradient (Colour (0xff3a6aff), 0.f, 0.f, Colour (0xff0c1c80), 0.f, 52.f, false));
        g.fillRect (0.f, 0.f, 1536.f, 52.f);
        g.setColour (Colour (0xff04041a));
        g.drawHorizontalLine (51, 0.f, 1536.f);
        g.setColour (Colours::white.withAlpha (0.25f));
        g.drawHorizontalLine (1, 0.f, 1536.f);

        starShape (g, 34.f, 26.f, 14.f, Colours::white, false);
        g.setColour (Colour (0xffe8e0ff));
        g.setFont (tahoma (30.f, true));
        g.drawText ("MSC VOX", 62, 6, 300, 40, Justification::centredLeft);

        // decorative window buttons
        for (int i = 0; i < 3; ++i)
        {
            const Rectangle<float> b (1420.f + (float) i * 38.f, 10.f, 32.f, 32.f);
            g.setColour (i == 2 ? Colour (0xffe03050) : Colour (0xff3a5cf0));
            g.fillRoundedRectangle (b, 5.f);
            g.setColour (Colours::white);
            g.drawRoundedRectangle (b, 5.f, 1.6f);
            if (i == 0) g.fillRect (b.getX() + 8.f, b.getBottom() - 11.f, 14.f, 3.f);
            if (i == 1) g.drawRect (b.reduced (9.f), 2.f);
            if (i == 2) { g.drawLine (b.getX() + 9.f, b.getY() + 9.f, b.getRight() - 9.f, b.getBottom() - 9.f, 3.f);
                          g.drawLine (b.getRight() - 9.f, b.getY() + 9.f, b.getX() + 9.f, b.getBottom() - 9.f, 3.f); }
        }
    }

    // ---------------- big graffiti logo ----------------
    distortedWord (g, "MSC", { 150.f, 150.f }, 140.f, Colour (0xfff0e8ff), 11);
    distortedWord (g, "VOX", { 170.f, 280.f }, 135.f, Colour (0xffc060ff), 23);
    g.setColour (Colour (0xffb45cff));
    for (int i = 0; i < 9; ++i)   // paint drips
        g.fillRoundedRectangle (150.f + (float) i * 28.f, 322.f, 4.f, 14.f + (float) ((i * 37) % 30), 2.f);

    // ---------------- stickers & doodles ----------------
    sticker (g, "POP\nPUNK\n4EVER", 14.f, 128.f, -0.25f, 30.f, cViolet);
    skull (g, 52.f, 250.f, 22.f, Colour (0xffe0d8ff));
    {
        g.saveState();
        g.addTransform (AffineTransform::rotation (-0.2f, 30.f, 330.f));
        g.setColour (Colour (0xffdde8ff));
        g.fillRoundedRectangle (30.f, 300.f, 150.f, 56.f, 4.f);
        g.setColour (Colour (0xff2a3a90));
        g.setFont (tahoma (32.f, true));
        g.drawText ("myspace", 62, 304, 118, 48, Justification::centredLeft);
        g.setColour (Colour (0xff4a6cff));
        g.fillEllipse (38.f, 312.f, 12.f, 12.f);
        g.fillEllipse (48.f, 316.f, 12.f, 12.f);
        g.restoreState();
    }
    sticker (g, "MUSIC\nSAVES\nMY LIFE", 1020.f, 270.f, -0.22f, 28.f, cViolet);
    heart (g, 1035.f, 380.f, 14.f, cViolet, false);

    sticker (g, "GOOD\nGIRLS\nLISTEN\nTO BAD\nMUSIC", 1436.f, 215.f, -0.15f, 32.f, Colour (0xffc8b0ff));
    heart (g, 1478.f, 100.f, 24.f, Colour (0xffd8b0ff), true);
    starShape (g, 1484.f, 372.f, 22.f, Colour (0xffd8b0ff), false);
    heart (g, 1478.f, 470.f, 24.f, cPink, true);
    starShape (g, 1470.f, 570.f, 26.f, cViolet, false);
    sticker (g, "EMO\nKIDS\nNEVER\nDIE", 1440.f, 650.f, -0.15f, 34.f, cPink);
    g.setColour (cViolet);
    g.drawEllipse (1470.f, 810.f, 34.f, 34.f, 3.f);
    g.fillEllipse (1479.f, 820.f, 4.f, 4.f);
    g.fillEllipse (1491.f, 820.f, 4.f, 4.f);

    sticker (g, "I\nMUSIC", 20.f, 850.f, -0.2f, 34.f, cViolet);
    heart (g, 52.f, 838.f, 14.f, cPink, false);
    starShape (g, 45.f, 940.f, 20.f, cViolet, true);
    sticker (g, "BETTER\nFASTER\nLOUDER", 326.f, 940.f, -0.22f, 28.f, cViolet);
    starShape (g, 470.f, 925.f, 14.f, cViolet, false);
    skull (g, 1356.f, 950.f, 26.f, Colour (0xffd8d0ff));
    starShape (g, 1470.f, 960.f, 34.f, cViolet, true, 0.2f);

    // ---------------- preset window ----------------
    drawPanel (g, { 1105.f, 62.f, 310.f, 322.f }, "PRESETS", false);
    g.setColour (Colour (0xff04041c));
    g.fillRoundedRectangle (1114.f, 104.f, 292.f, 270.f, 4.f);
    for (int i = 0; i < 2; ++i)
    {
        const Rectangle<float> b (1340.f + (float) i * 32.f, 70.f, 26.f, 24.f);
        g.setColour (i ? Colour (0xffe03070) : Colour (0xff6a50e0));
        g.fillRoundedRectangle (b, 4.f);
        g.setColour (Colours::white);
        if (i == 0) g.fillRect (b.getX() + 6.f, b.getBottom() - 8.f, 14.f, 3.f);
        else { g.drawLine (b.getX() + 7.f, b.getY() + 6.f, b.getRight() - 7.f, b.getBottom() - 6.f, 2.5f);
               g.drawLine (b.getRight() - 7.f, b.getY() + 6.f, b.getX() + 7.f, b.getBottom() - 6.f, 2.5f); }
    }

    // ---------------- control panels ----------------
    drawPanel (g, { 135.f, 422.f, 290.f, 255.f }, "COMPRESSOR");
    drawPanel (g, { 435.f, 422.f, 325.f, 255.f }, "EQ");
    drawPanel (g, { 778.f, 422.f, 157.f, 255.f }, "AIR / BRIGHTNESS");
    drawPanel (g, { 945.f, 422.f, 138.f, 255.f }, "PRESENCE");
    drawPanel (g, { 1093.f, 422.f, 207.f, 255.f }, "DE-ESSER");
    drawPanel (g, { 135.f, 688.f, 280.f, 200.f }, "SATURATION / DISTORTION");
    drawPanel (g, { 427.f, 688.f, 278.f, 200.f }, "REVERB");
    drawPanel (g, { 715.f, 688.f, 245.f, 200.f }, "DELAY");
    drawPanel (g, { 972.f, 688.f, 328.f, 200.f }, "CHORUS");

    // I/O meter panels + scales
    drawPanel (g, { 18.f, 420.f, 100.f, 380.f }, "", false);
    drawPanel (g, { 1315.f, 420.f, 105.f, 380.f }, "", false);
    drawPanel (g, { 1315.f, 808.f, 105.f, 92.f }, "", false);
    g.setColour (cText);
    g.setFont (tahoma (22.f, true));
    g.drawText ("IN", 18, 426, 100, 26, Justification::centred);
    g.drawText ("OUT", 1315, 426, 105, 26, Justification::centred);
    g.setFont (tahoma (14.f, false));
    {
        const int db[] = { 0, -6, -12, -18, -24, -30, -36, -48 };
        for (int i = 0; i < 8; ++i)
        {
            const float y = 452.f + 224.f * (1.f - (float) (db[i] + 48) / 48.f);
            g.setColour (cText.withAlpha (0.8f));
            g.drawText (String (db[i]), 68, (int) y - 8, 40, 16, Justification::centredLeft);
            g.drawText (String (db[i]), 1364, (int) y - 8, 40, 16, Justification::centredLeft);
        }
    }

    g.setColour (cText);
    g.setFont (tahoma (14.f, true));
    g.drawText ("TYPE", 310, 772, 98, 18, Justification::centred);
    g.drawText ("MAKEUP", 300, 630, 90, 16, Justification::centredLeft);

    // footer
    drawPanel (g, { 497.f, 908.f, 420.f, 100.f }, "", false);
    drawPanel (g, { 933.f, 908.f, 362.f, 100.f }, "", false);
    g.setColour (cText);
    g.setFont (tahoma (20.f, true));
    g.drawText ("MASTER", 1080, 920, 120, 24, Justification::centredLeft);

    // distressed texture over everything static
    g.drawImage (texture, Rectangle<float> (0.f, 0.f, 1536.f, 1024.f));
}

