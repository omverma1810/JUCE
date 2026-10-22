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

## Plugin identifiers
| Field | Value |
|---|---|
| Manufacturer code | `Esp1` |
| Plugin code | `Gn01` |
| Bundle ID | `com.espirittech.gainplugin` |

## Not yet covered
- REAPER and Logic Pro manual testing
- Developer ID code signing and notarization
- AAX (requires Avid developer account, AAX SDK and PACE signing)
