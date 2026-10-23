#!/bin/bash
# Double-click to open Vibe FX with the demo song loaded on the "Slowed + Reverb" preset.
# Press Play in the app to start.

cd "$(dirname "$0")/.." || exit 1
APP="build/VibeFX/VibeFX_artefacts/Release/Standalone/Vibe FX.app"

if [ ! -d "$APP" ]; then
    echo "Vibe FX isn't built yet, building it now (the first build takes a few minutes)..."
    cmake -B build -G Xcode && cmake --build build --config Release --target VibeFX_All -- -quiet || exit 1
fi

open "$APP" --args --load "$PWD/demo/demo_track.wav" --preset "Slowed + Reverb"
