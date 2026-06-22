# MultiBandEQ

A multi-band parametric equalizer audio plugin built with [JUCE](https://juce.com).
It starts with three bands and lets you add or remove bands on the fly (up to eight),
each with a selectable filter type, and shows a live frequency-response curve.

Builds as **Standalone**, **VST3**, and **AU**.

## Features

- **3 bands by default**, expandable to **8** via the *Add Band* / *Remove Band* buttons.
- **5 filter types per band:** Low Cut, Low Shelf, Peak, High Shelf, High Cut.
- Per-band **Frequency** (20 Hz–20 kHz), **Gain** (±24 dB), and **Q** (0.1–10) controls.
- **Live frequency-response curve** that reflects the combined effect of all enabled bands.
- **Full state save/restore** via JUCE's `AudioProcessorValueTreeState`, so settings persist
  in DAW sessions and presets and are fully automatable by the host.
- Stereo processing with efficient IIR filters (`juce::dsp`).

## How "adding bands" works

Plugin formats (VST3/AU) require a fixed parameter layout for host automation and preset
recall, so parameters cannot be created at runtime. MultiBandEQ instead pre-declares a fixed
maximum of 8 bands, each with an on/off switch. *Add Band* enables the next dormant band and
*Remove Band* disables the last active one. This keeps automation and saved state stable while
still feeling like you are adding and removing bands.

## Project layout

```
source/
  EqConstants.h             Shared config: band count, filter-type enum, parameter-ID helpers
  PluginProcessor.{h,cpp}   DSP: APVTS parameter layout, per-band IIR filters, state save/restore
  PluginEditor.{h,cpp}      Main editor: response curve, band strips, add/remove buttons
  BandComponent.{h,cpp}     One control strip per band (on/off, type, freq/gain/Q)
  ResponseCurveComponent.{h,cpp}  Live magnitude-response display
modules/                    JUCE and cmake-includes submodules
CMakeLists.txt              Build configuration
```

## Building

### Prerequisites

- A C++20 compiler (Xcode/Clang on macOS, MSVC on Windows, GCC/Clang on Linux)
- [CMake](https://cmake.org) ≥ 3.23
- On Linux: `webkit2gtk-4.0` and the usual JUCE Linux dependencies

### Clone (with submodules)

JUCE and the CMake helpers are git submodules, so clone recursively:

```bash
git clone --recurse-submodules <repo-url>
cd MultiBandEQ
# already cloned without submodules?
git submodule update --init --recursive
```

### Configure and build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Where the artefacts land

After a successful build the plugin/app bundles are under:

```
build/MultiBandEQ_artefacts/<Config>/
  Standalone/MultiBandEQ.app      (or .exe)
  VST3/MultiBandEQ.vst3
  AU/MultiBandEQ.component         (macOS only)
```

The build also copies the VST3/AU into your user plugin folder
(`COPY_PLUGIN_AFTER_BUILD` is enabled) so DAWs pick them up automatically.

## Usage

1. Launch the standalone app, or load the VST3/AU in your DAW.
2. Each band strip has an on/off toggle, a filter-type menu, and Freq / Gain / Q knobs.
   (Gain has no effect on the Low Cut and High Cut types.)
3. Use **Add Band** / **Remove Band** to change how many bands are active.
4. Watch the curve at the top update as you adjust the controls.

## Tech stack

- **JUCE** audio plugin framework
- **`juce_dsp`** IIR filters (`ProcessorDuplicator` for stereo)
- **`AudioProcessorValueTreeState`** for parameters, automation, and state

## License

See [LICENSE](LICENSE).
