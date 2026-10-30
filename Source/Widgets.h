#pragma once
#include <JuceHeader.h>
#include "Presets.h"

namespace UI
{
using namespace juce;

inline Font tahoma (float h, bool bold = false) { return Font (FontOptions ("Tahoma", h, bold ? Font::bold : Font::plain)); }
inline Font punkFont (float h)                  { return Font (FontOptions ("Impact", h, Font::plain)); }
inline Font scrawl (float h)                    { return Font (FontOptions ("Ink Free", h, Font::bold)); }

inline const Colour cNavy   { 0xff0a0a2c };
inline const Colour cPanel  { 0xff12124a };
inline const Colour cBlue   { 0xff4a6cff };
inline const Colour cViolet { 0xffb45cff };
inline const Colour cPink   { 0xffff5fb0 };
inline const Colour cText   { 0xffe8ddff };

// ---------------------------------------------------------------------------
class MSCLookAndFeel : public LookAndFeel_V4
{
public:
    MSCLookAndFeel()
    {
        setColour (ComboBox::backgroundColourId, cNavy);
        setColour (ComboBox::textColourId, Colours::white);
        setColour (ComboBox::outlineColourId, cBlue);
        setColour (PopupMenu::backgroundColourId, cNavy);
        setColour (PopupMenu::textColourId, Colours::white);
        setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff7a34d8));
        setColour (TextButton::textColourOffId, Colours::white);
        setColour (TextButton::textColourOnId, Colours::white);
    }

    Font getTextButtonFont (TextButton&, int h) override { return tahoma ((float) h * 0.5f, true); }
    Font getComboBoxFont (ComboBox& c) override         { return tahoma ((float) c.getHeight() * 0.5f, true); }
    Font getPopupMenuFont() override                    { return tahoma (16.f, true); }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider&) override
    {
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
        const float d = jmin (b.getWidth(), b.getHeight());
        b = b.withSizeKeepingCentre (d, d);
        const float cx = b.getCentreX(), cy = b.getCentreY();

        g.setColour (Colour (0xff070720));
        g.fillEllipse (b);

        Path track;
        track.addCentredArc (cx, cy, d * 0.5f - 1.f, d * 0.5f - 1.f, 0.f, start, end, true);
        g.setColour (Colour (0xff2a2a66));
        g.strokePath (track, PathStrokeType (2.f));

        const float ang = start + pos * (end - start);
        Path val;
        val.addCentredArc (cx, cy, d * 0.5f - 1.f, d * 0.5f - 1.f, 0.f, start, ang, true);
        g.setColour (cViolet);
        g.strokePath (val, PathStrokeType (2.6f));

        const auto inner = b.reduced (d * 0.1f);
        g.setGradientFill (ColourGradient (Colour (0xff9a7cff), cx - d * 0.2f, b.getY() + d * 0.12f,
                                           Colour (0xff2a1a72), b.getRight(), b.getBottom(), true));
        g.fillEllipse (inner);
        g.setColour (Colour (0xff7a9cff));
        g.drawEllipse (inner, 1.4f);

        Path ptr;
        ptr.addRectangle (-1.6f, -d * 0.40f, 3.2f, d * 0.22f);
        g.setColour (Colours::white);
        g.fillPath (ptr, AffineTransform::rotation (ang).translated (cx, cy));
    }

    void drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           Slider::SliderStyle, Slider&) override
    {
        const float ty = (float) y + (float) h * 0.5f - 5.f;
        Rectangle<float> track ((float) x, ty, (float) w, 10.f);
        g.setColour (Colour (0xff050518));
        g.fillRoundedRectangle (track, 3.f);
        g.setGradientFill (ColourGradient (cViolet, track.getX(), 0.f, cBlue, pos, 0.f, false));
        g.fillRoundedRectangle (track.withRight (pos), 3.f);
        g.setColour (cBlue);
        g.drawRoundedRectangle (track, 3.f, 1.2f);

        Rectangle<float> th (pos - 7.f, ty - 6.f, 14.f, 22.f);
        g.setGradientFill (ColourGradient (Colour (0xffe0d8ff), 0.f, th.getY(), Colour (0xff7a68d8), 0.f, th.getBottom(), false));
        g.fillRoundedRectangle (th, 3.f);
        g.setColour (Colours::black);
        g.drawRoundedRectangle (th, 3.f, 1.4f);
    }

    void drawToggleButton (Graphics& g, ToggleButton& b, bool hover, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const bool on = b.getToggleState();

        if (b.getComponentID() == "power")
        {
            g.setGradientFill (ColourGradient (on ? Colour (0xffa070ff) : Colour (0xff202060), 0.f, r.getY(),
                                               on ? Colour (0xff5a30c0) : Colour (0xff10103a), 0.f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 4.f);
            g.setColour (hover ? Colours::white : cBlue);
            g.drawRoundedRectangle (r, 4.f, 1.4f);
            const auto c = r.getCentre();
            const float rad = r.getWidth() * 0.26f;
            Path arc;
            arc.addCentredArc (c.x, c.y, rad, rad, 0.f, 0.6f, MathConstants<float>::twoPi - 0.6f, true);
            arc.startNewSubPath (c.x, c.y - rad - 1.f);
            arc.lineTo (c.x, c.y - 1.f);
            g.setColour (on ? Colours::white : Colour (0xff8080c0));
            g.strokePath (arc, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
            return;
        }

        const bool isBypass = b.getComponentID() == "bypass";
        const Colour onCol = isBypass ? Colour (0xffe03070) : Colour (0xff7a34d8);
        g.setGradientFill (ColourGradient (on ? onCol.brighter (0.3f) : Colour (0xff1c1c60), 0.f, r.getY(),
                                           on ? onCol.darker (0.4f) : Colour (0xff0c0c34), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (hover ? Colours::white : cBlue);
        g.drawRoundedRectangle (r, 5.f, 1.5f);
        g.setColour (Colours::white);
        g.setFont (tahoma (r.getHeight() * 0.45f, true));
        g.drawText (b.getButtonText(), r.toNearestInt(), Justification::centred);
    }

    void drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const auto id = b.getComponentID();
        const bool vocal = id == "vocal";

        const Colour top = vocal ? Colour (0xffd9a8ff) : Colour (0xff2a2a90);
        const Colour bot = vocal ? Colour (0xff7a34d8) : Colour (0xff0c0c3c);
        g.setGradientFill (ColourGradient (down ? bot : top, 0.f, r.getY(), down ? top : bot, 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.f);
        g.setColour (hover || down ? Colours::white : (vocal ? cPink : cBlue));
        g.drawRoundedRectangle (r, 6.f, vocal ? 2.5f : 1.6f);

        g.setColour (Colours::white);
        const auto c = r.getCentre();
        if (id == "arrowL" || id == "arrowR")
        {
            const float s = r.getHeight() * 0.28f, d = id == "arrowL" ? -1.f : 1.f;
            Path t;
            t.addTriangle (c.x + d * s, c.y, c.x - d * s, c.y - s, c.x - d * s, c.y + s);
            g.setColour (cViolet);
            g.fillPath (t);
        }
        else if (id == "gear")
        {
            Path gear;
            gear.addStar (c, 8, r.getHeight() * 0.22f, r.getHeight() * 0.32f);
            gear.addEllipse (c.x - 4.f, c.y - 4.f, 8.f, 8.f);
            gear.setUsingNonZeroWinding (false);
            g.setColour (Colours::white);
            g.fillPath (gear);
        }
        else if (vocal)
        {
            // little headphones
            Path hp;
            hp.addCentredArc (r.getX() + r.getHeight() * 0.62f, c.y + 4.f, 13.f, 13.f, 0.f, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, true);
            hp.startNewSubPath (r.getX() + r.getHeight() * 0.62f - 13.f, c.y + 4.f);
            hp.lineTo (r.getX() + r.getHeight() * 0.62f - 13.f, c.y + 12.f);
            g.setColour (Colours::white);
            g.strokePath (hp, PathStrokeType (3.f));
        }
    }

    void drawButtonText (Graphics& g, TextButton& b, bool, bool) override
    {
        const auto id = b.getComponentID();
        if (id == "arrowL" || id == "arrowR" || id == "gear") return;
        g.setColour (Colours::white);
        g.setFont (tahoma ((float) b.getHeight() * (id == "vocal" ? 0.34f : 0.46f), true));
        auto area = b.getLocalBounds();
        if (id == "vocal") area.removeFromLeft (area.getHeight());
        g.drawText (b.getButtonText(), area, Justification::centred);
    }

    void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox&) override
    {
        auto r = Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (1.f);
        g.setGradientFill (ColourGradient (Colour (0xff12124a), 0.f, 0.f, Colour (0xff060626), 0.f, (float) h, false));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (cBlue);
        g.drawRoundedRectangle (r, 4.f, 1.5f);
        Path t;
        const float ax = (float) w - 16.f, ay = (float) h * 0.5f;
        t.addTriangle (ax - 5.f, ay - 3.f, ax + 5.f, ay - 3.f, ax, ay + 4.f);
        g.setColour (cViolet);
        g.fillPath (t);
    }

    void positionComboBoxText (ComboBox& c, Label& l) override
    {
        l.setBounds (8, 1, c.getWidth() - 30, c.getHeight() - 2);
        l.setFont (getComboBoxFont (c));
    }
};

// ---------------------------------------------------------------------------
//  Rotary knob with name + value readout underneath
// ---------------------------------------------------------------------------
class Knob : public Component
{
public:
    Knob (AudioProcessorValueTreeState& s, const String& paramID, const String& name,
          std::function<String (double)> formatter, float designFont = 14.f)
        : label (name), fmt (std::move (formatter)), baseFont (designFont)
    {
        slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (slider);
        att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, paramID, slider);
        slider.onValueChange = [this] { repaint(); };
    }

    void setScale (float sc) { fontH = baseFont * sc; resized(); repaint(); }

    void resized() override
    {
        const float w = (float) getWidth(), h = (float) getHeight();
        const float d = jmin (w - 2.f, h - fontH * 2.9f);
        slider.setBounds (Rectangle<float> ((w - d) * 0.5f, 1.f, d, d).toNearestInt());
    }

    void paint (Graphics& g) override
    {
        const float d = (float) slider.getHeight();
        g.setColour (cText);
        g.setFont (tahoma (fontH, true));
        g.drawText (label, 0, (int) (d + 2.f), getWidth(), (int) (fontH * 1.3f), Justification::centred);
        g.setColour (Colour (0xffb9a6ff));
        g.setFont (tahoma (fontH, false));
        g.drawText (fmt (slider.getValue()), 0, (int) (d + 2.f + fontH * 1.3f), getWidth(), (int) (fontH * 1.3f), Justification::centred);
    }

private:
    Slider slider;
    String label;
    std::function<String (double)> fmt;
    float baseFont, fontH = 12.f;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> att;
};

// ---------------------------------------------------------------------------
//  Component that renders through a lambda (waveform, EQ curve, displays)
// ---------------------------------------------------------------------------
class LiveView : public Component
{
public:
    explicit LiveView (std::function<void (Graphics&, Rectangle<float>)> f) : fn (std::move (f))
    {
        setInterceptsMouseClicks (false, false);
    }
    void paint (Graphics& g) override { fn (g, getLocalBounds().toFloat()); }

private:
    std::function<void (Graphics&, Rectangle<float>)> fn;
};

// ---------------------------------------------------------------------------
//  Segmented level meter, -48 .. 0 dB
// ---------------------------------------------------------------------------
class MeterBar : public Component
{
public:
    MeterBar() { setInterceptsMouseClicks (false, false); }

    void push (float newDb)
    {
        db = jmax (newDb, db - 2.2f);
        repaint();
    }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (Colour (0xff04041a));
        g.fillRect (b);
        constexpr int n = 24;
        const float segH = b.getHeight() / (float) n;
        for (int i = 0; i < n; ++i)
        {
            const float segDb = -48.f + 48.f * ((float) i + 0.5f) / (float) n;
            const bool lit = db >= segDb;
            Colour c = segDb > -6.f ? Colour (0xffff3030) : (segDb > -18.f ? Colour (0xffffd030) : Colour (0xff30e060));
            g.setColour (lit ? c : c.darker (0.85f));
            g.fillRect (b.getX() + 1.f, b.getBottom() - ((float) i + 1.f) * segH + 1.f, b.getWidth() - 2.f, segH - 1.5f);
        }
    }

private:
    float db = -60.f;
};

// ---------------------------------------------------------------------------
//  Clickable list of the eight presets
// ---------------------------------------------------------------------------
class PresetList : public Component
{
public:
    std::function<void (int)> onSelect;
    int selected = 7;

    void paint (Graphics& g) override
    {
        const auto& list = MSC::presets();
        const float rowH = (float) getHeight() / (float) list.size();
        for (size_t i = 0; i < list.size(); ++i)
        {
            Rectangle<float> r (0.f, rowH * (float) i, (float) getWidth(), rowH);
            const bool sel = (int) i == selected;
            if (sel)
            {
                g.setGradientFill (ColourGradient (Colour (0xffc080ff), 0.f, r.getY(), Colour (0xff8a3cf0), 0.f, r.getBottom(), false));
                g.fillRoundedRectangle (r.reduced (2.f, 1.f), 3.f);
                Path star;
                star.addStar ({ r.getRight() - 22.f, r.getCentreY() }, 5, 5.f, 11.f);
                g.setColour (Colour (0xff10104a));
                g.fillPath (star);
            }
            else
            {
                g.setColour (Colour (0xff2a2a80));
                g.drawHorizontalLine ((int) r.getBottom() - 1, 6.f, (float) getWidth() - 6.f);
            }
            g.setColour (sel ? Colours::white : cText);
            g.setFont (tahoma (rowH * 0.5f, true));
            g.drawText (String::formatted ("%02d - ", (int) i + 1) + list[i].name,
                        r.toNearestInt().withTrimmedLeft (12), Justification::centredLeft);
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        const int n = (int) MSC::presets().size();
        const int i = jlimit (0, n - 1, (int) ((float) e.y / ((float) getHeight() / (float) n)));
        if (onSelect) onSelect (i);
    }
    MouseCursor getMouseCursor() override { return MouseCursor::PointingHandCursor; }
};
} // namespace UI
