# Gain PoC — JUCE 8 Audio Plugin

A minimal gain plugin built with JUCE 8 and CMake, used as a proof of concept for the
VST3 / AU / Standalone build, validation and DAW-loading pipeline on macOS.

## Features
- Single **Gain** parameter, −60 dB (silence) to +12 dB, smoothed (20 ms) to avoid zipper noise
- Mono and stereo bus layouts
- Host automation and state save/restore via `AudioProcessorValueTreeState`
- Formats: **VST3**, **AU**, **Standalone**
- **Universal binary** (arm64 + x86_64), macOS 11.0+

## Requirements
- macOS with Xcode (tested with Xcode 26.6)
- CMake ≥ 3.22 (`brew install cmake`)

## Build
```bash
git clone --recurse-submodules https://github.com/omverma1810/JUCE.git
cd JUCE
cmake -B build -G Xcode
cmake --build build --config Release
```

Artefacts land in `build/GainPlugin_artefacts/Release/` and the AU/VST3 are copied to
`~/Library/Audio/Plug-Ins/Components` and `~/Library/Audio/Plug-Ins/VST3`.

## Validation
| Test | Result |
|---|---|
| `auval -strict -v aufx Gn01 Esp1` (arm64) | AU VALIDATION SUCCEEDED |
| `arch -x86_64 auval -strict -v aufx Gn01 Esp1` (Rosetta) | AU VALIDATION SUCCEEDED |
| pluginval 1.0.4, strictness 10 — VST3 | SUCCESS |
| pluginval 1.0.4, strictness 10 — AU | SUCCESS |

Full logs are in [`validation/`](validation/).

## DAW test — REAPER 7.82 (macOS arm64)
[`demo/reaper_gain_test.lua`](demo/reaper_gain_test.lua) is a ReaScript that loads the plugin
through REAPER's API, sets Gain to −6 dB, renders a −12 dBFS 1 kHz test tone
([`demo/sine_1k_-12dBFS.wav`](demo/sine_1k_-12dBFS.wav)) and logs the result.

| Format | Loaded by REAPER as | Gain | Output peak |
|---|---|---|---|
| VST3 | `VST3: Gain PoC (Espirit Tech)` | −6.0 dB | −18.00 dBFS |
| AU | `AU: Gain PoC (Espirit Tech)` | −6.0 dB | −18.00 dBFS |

Both formats attenuate the signal by exactly 6 dB. Rendered files and the log are in
[`demo/`](demo/).

To rerun it (uses a separate REAPER instance, so an open REAPER session is untouched):
```bash
/Applications/REAPER.app/Contents/MacOS/REAPER -newinst -nosplash -new "$PWD/demo/reaper_gain_test.lua"
```
The script writes to an absolute path, so edit `dir` at the top of the script if the repo lives elsewhere.

## Plugin identifiers
| Field | Value |
|---|---|
| Manufacturer code | `Esp1` |
| Plugin code | `Gn01` |
| Bundle ID | `com.espirittech.gainplugin` |

## Not yet covered
- Logic Pro / GarageBand testing of the AU
- Developer ID code signing and notarization
- AAX (requires Avid developer account, AAX SDK and PACE signing)
