# MSC VOX – build & install guide (Windows 10/11 x64, FL Studio)

## 0. Folder layout (exactly this)

```
MSC_VOX/
├─ CMakeLists.txt
└─ Source/
   ├─ Parameters.h        all automatable parameters (single table)
   ├─ Presets.h           the 8 factory presets
   ├─ DSP.h               biquad filter + delay line
   ├─ PluginProcessor.h   audio engine (header)
   ├─ PluginProcessor.cpp audio engine: compressor, EQ, air, presence, de-esser,
   │                      saturation/distortion, chorus, stereo delay, reverb, gains, wet/dry, bypass
   ├─ Characters.h        animation state machine + layered artwork for both characters
   ├─ Widgets.h           look-and-feel (XP/punk), knobs, meters, preset list
   ├─ PluginEditor.h
   └─ PluginEditor.cpp    layout, graffiti logo, timer-driven animation
```

## 1. Install the tools (one time)

1. **Visual Studio 2022 Community** (free). In the installer tick **"Desktop development with C++"**.
2. **Git for Windows**: https://git-scm.com/download/win
3. **CMake** (3.22+): https://cmake.org/download/ (tick "Add CMake to PATH").

## 2. Build

Open **PowerShell** in the `MSC_VOX` folder (Shift + right-click in the folder -> "Open in Terminal"):

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The first run downloads JUCE (needs internet, a few minutes). The result is the folder:

```
build\MSCVox_artefacts\Release\VST3\MSC VOX.vst3
```

## 3. Install in FL Studio

1. Copy the whole **`MSC VOX.vst3`** folder to `C:\Program Files\Common Files\VST3\` (needs admin permission).
2. FL Studio -> **Options -> Manage plugins**. Make sure `C:\Program Files\Common Files\VST3` is in the list of
   plugin search paths, then click **Find installed plugins** (scan).
3. In the Mixer, click an empty effect slot -> pick **MSC VOX** (under *Installed -> Effects*; also appears in the plugin database).
4. Put it on your vocal channel and select a preset.

## 4. How the pieces behave

* **Presets** – click the list, use the dropdown, or the `<` `>` arrows. Loading a *different* preset sets every effect
  parameter (input gain, output gain, master and bypass are never changed), and both characters play their ~1.6 s reaction.
  Re-selecting the same preset, moving knobs, or opening the window does **not** trigger the reaction.
  Clicking presets rapidly restarts the reaction smoothly (a blend weight eases in/out, so there is no pop).
* **VOCAL CHECK** – hold it to hear your *unprocessed* vocal (A/B comparison); release to return to the processed sound.
* **Delay SYNC** – when on, the TIME knob snaps to 1/16, 1/8, 1/4, 1/2, 1/1 of the host tempo.
* **Settings (gear)** – animation 24/30 FPS, reset gains.
* **SAVE / LOAD** – store your own settings as `.mscvox` files.
* Every knob, switch and menu is a host-automatable parameter. Plugin state is saved inside the FL Studio project.
* The window is resizable (fixed aspect ratio).

## 5. Architecture notes (audio vs. animation)

* All DSP lives in `PluginProcessor.cpp`. No allocation in `processBlock` (buffers are created in `prepareToPlay`),
  parameters are smoothed (no clicks), mono and stereo are supported, sample-rate changes are handled in `prepareToPlay`.
* The GUI only reads lock-free atomics from the processor (meters, waveform ring buffer, preset-change counter).
* Animation runs on a JUCE `Timer` at 30 (or 24) FPS on the GUI thread only. Only the character region is repainted each frame;
  the rest of the interface is a cached image rendered at each resize.

## 6. Replacing the artwork with real sprite sheets later

The characters are drawn from layered vector parts (`drawFemale` / `drawMale` in `Characters.h`), posed by a `Pose` struct.
That is a faithful-in-spirit stand-in, **not a copy of the illustrations in your reference image** (I can't extract and
cut those out automatically). To use your own art, you need transparent PNG sprite sheets, same frame size for every frame:

| Asset | Content | Suggested frames |
|-------|---------|------------------|
| `female_idle.png`   | breathing, head sway, blink, small dance | 24 |
| `female_react.png`  | head shake, hair whip, dance, final singing pose | 36-48 |
| `male_idle.png`     | breathing, blink, guitar in playing position | 24 |
| `male_react.png`    | aggressive strum, head bang | 36-48 |

Keep `Animator` as it is and rewrite only `drawFemale` / `drawMale` to draw frame `n` from the sheet
(`n` = reaction time x FPS while reacting, idle clock x FPS otherwise). Load the images once in the editor constructor
(`ImageCache` / `ImageFileFormat`, or embed with `juce_add_binary_data`).

## 7. If the build fails

Copy the **first** error message from the console and send it to me - this code was written without access to a compiler,
so a typo or a small API difference is possible. JUCE itself is licensed under AGPLv3/commercial terms; check them before distributing the plugin.
