-- Headless REAPER test: load Vibe FX (VST3), render the demo track through every factory preset.
local dir = "/Users/espirittech/Desktop/JUCE/demo/"
local outDir = dir .. "vibefx_presets/"
local log = io.open(outDir .. "reaper_vibefx_results.txt", "w")
local function p(s) log:write(s .. "\n") end

p("REAPER " .. reaper.GetAppVersion())

reaper.InsertTrackAtIndex(0, true)
local tr = reaper.GetTrack(0, 0)
reaper.SetOnlyTrackSelected(tr)
reaper.SetEditCurPos(0, false, false)
reaper.InsertMedia(dir .. "demo_track.wav", 0)

local fx = reaper.TrackFX_AddByName(tr, "VST3:Vibe FX (Espirit Tech)", false, -1)
if fx < 0 then p("FAILED to load Vibe FX"); log:close(); reaper.Main_OnCommand(40004, 0); return end

local _, fxName = reaper.TrackFX_GetFXName(tr, fx, "")
local _, numPresets = reaper.TrackFX_GetPresetIndex(tr, fx)
p(string.format("Loaded '%s' with %d parameters and %d presets", fxName, reaper.TrackFX_GetNumParams(tr, fx), numPresets))

reaper.GetSetProjectInfo_String(0, "RENDER_FILE", outDir, true)
reaper.GetSetProjectInfo_String(0, "RENDER_FORMAT", "ZXZhdxAAAA==", true) -- WAV 16-bit
reaper.GetSetProjectInfo(0, "RENDER_BOUNDSFLAG", 0, true)                 -- custom range
reaper.GetSetProjectInfo(0, "RENDER_STARTPOS", 0, true)
reaper.GetSetProjectInfo(0, "RENDER_ENDPOS", 20, true)

for i = 0, numPresets - 1 do
  reaper.TrackFX_SetPresetByIndex(tr, fx, i)
  local _, presetName = reaper.TrackFX_GetPreset(tr, fx, "")
  local fileName = string.format("%02d_%s", i + 1, presetName:gsub("[^%w]+", "_"):gsub("_+$", ""))
  reaper.GetSetProjectInfo_String(0, "RENDER_PATTERN", fileName, true)
  reaper.Main_OnCommand(42230, 0) -- render with current settings, auto-close
  p(string.format("preset %2d: %-22s -> %s.wav", i + 1, presetName, fileName))
end

-- Save the project (ready to open for a live demo); this also avoids the "save changes?" prompt on quit
reaper.TrackFX_SetPresetByIndex(tr, fx, 1) -- open on "Slowed + Reverb"
reaper.Main_SaveProjectEx(0, outDir .. "VibeFX_demo.rpp", 0)
p("saved VibeFX_demo.rpp")
log:close()
reaper.Main_OnCommand(40004, 0) -- quit
