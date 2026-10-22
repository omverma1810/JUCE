-- Headless REAPER test: load Gain PoC (VST3 and AU), set gain to -6 dB, render, log results.
local dir = "/Users/espirittech/Desktop/JUCE/demo/"
local log = io.open(dir .. "reaper_test_results.txt", "w")
local function p(s) log:write(s .. "\n") end

p("REAPER " .. reaper.GetAppVersion())

local function testFormat(fxName, outName)
  reaper.Main_OnCommand(40859, 0) -- new project tab
  reaper.InsertTrackAtIndex(0, true)
  local tr = reaper.GetTrack(0, 0)
  reaper.InsertMedia(dir .. "sine_1k_-12dBFS.wav", 0)

  local fx = reaper.TrackFX_AddByName(tr, fxName, false, -1)
  if fx < 0 then p(fxName .. ": FAILED to load"); return end
  local _, name = reaper.TrackFX_GetFXName(tr, fx, "")
  local _, pname = reaper.TrackFX_GetParamName(tr, fx, 0, "")
  reaper.TrackFX_SetParamNormalized(tr, fx, 0, (-6 + 60) / 72) -- -6 dB on a -60..+12 range
  local _, fmt = reaper.TrackFX_GetFormattedParamValue(tr, fx, 0, "")
  p(string.format("%s: loaded as '%s', param 0 '%s' = %s", fxName, name, pname, fmt))

  reaper.GetSetProjectInfo_String(0, "RENDER_FILE", dir, true)
  reaper.GetSetProjectInfo_String(0, "RENDER_PATTERN", outName, true)
  reaper.GetSetProjectInfo(0, "RENDER_BOUNDSFLAG", 1, true) -- entire project
  reaper.GetSetProjectInfo_String(0, "RENDER_FORMAT", "ZXZhdxgAAA==", true) -- WAV 24-bit
  reaper.Main_OnCommand(42230, 0) -- render with current settings, auto-close
  p("  rendered " .. outName .. ".wav")
  reaper.Main_OnCommand(40860, 0) -- close project tab
end

testFormat("VST3:Gain PoC (Espirit Tech)", "reaper_vst3_minus6dB")
testFormat("AU:Espirit Tech: Gain PoC", "reaper_au_minus6dB")
log:close()
reaper.Main_OnCommand(40004, 0) -- quit
