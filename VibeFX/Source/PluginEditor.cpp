#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    namespace palette
    {
        const juce::Colour speed { 0xff8b6cff }, lofi { 0xfff2a541 }, filter { 0xff3fb5f2 }, bass { 0xffff5c7a },
                           drive { 0xffff7a45 }, chorus { 0xff4adede }, echo { 0xff5c9dff }, reverb { 0xffb18cff },
                           pan8d { 0xff34d399 }, width { 0xfff472b6 }, fades { 0xfffacc15 }, output { 0xffe7e9ef };
    }
}

VibeFXEditor::VibeFXEditor (VibeFXProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      meters (p.inputMeter, p.outputMeter),
      visualiser (p.inputAnalyser, p.outputAnalyser, [&p] { return p.getSampleRate() > 0.0 ? p.getSampleRate() : 44100.0; })
{
    setLookAndFeel (&lookAndFeel);
    auto& state = audioProcessor.apvts;

    // Header: presets and bypass
    const auto& presets = getFactoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem (presets[(size_t) i].name, i + 1);

    presetBox.onChange = [this] { selectPreset (presetBox.getSelectedId() - 1); };
    previousButton.onClick = [this] { selectPreset ((audioProcessor.getCurrentProgram() + audioProcessor.getNumPrograms() - 1) % audioProcessor.getNumPrograms()); };
    nextButton.onClick = [this] { selectPreset ((audioProcessor.getCurrentProgram() + 1) % audioProcessor.getNumPrograms()); };
    previousButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");

    bypassButton.setClickingTogglesState (true);
    bypassButton.setColour (juce::TextButton::buttonOnColourId, vibe::colours::danger);
    bypassButton.setTooltip ("Compare with the original sound");
    bypassAttachment = std::make_unique<vibe::APVTS::ButtonAttachment> (state, IDs::bypass, bypassButton);

    presetDescription.setFont (vibe::font (12.5f));
    presetDescription.setColour (juce::Label::textColourId, vibe::colours::muted);

    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &previousButton, &nextButton, &bypassButton,
                                                             &presetDescription, &visualiser, &meters })
        addAndMakeVisible (c);

    // Effect modules, in signal-flow order
    auto addCard = [&] (const char* title, const char* subtitle, juce::Colour colour, const char* onId)
    {
        cards.push_back (std::make_unique<vibe::EffectCard> (state, title, subtitle, colour, onId));
        addAndMakeVisible (*cards.back());
        return cards.back().get();
    };

    auto* speed = addCard ("SPEED", "Slowed / sped-up / nightcore", palette::speed, IDs::speedOn);
    speed->addKnob (IDs::speed, "SPEED");

    auto* lofi = addCard ("LO-FI", "Cassette wobble and crackle", palette::lofi, IDs::lofiOn);
    lofi->addKnob (IDs::lofiBits, "BITS");
    lofi->addKnob (IDs::lofiRate, "RATE");
    lofi->addKnob (IDs::lofiWow, "WOW");
    lofi->addKnob (IDs::lofiNoise, "NOISE");
    lofi->addKnob (IDs::lofiTone, "TONE");
    lofi->addKnob (IDs::lofiMix, "MIX");

    auto* filter = addCard ("FILTER", "Telephone, radio, underwater", palette::filter, IDs::filterOn);
    filter->addKnob (IDs::filterCutoff, "CUTOFF");
    filter->addKnob (IDs::filterRes, "RESONANCE");
    filter->addChoice (IDs::filterType);

    auto* bass = addCard ("BASS BOOST", "Extra low end", palette::bass, IDs::bassOn);
    bass->addKnob (IDs::bassGain, "BOOST");
    bass->addKnob (IDs::bassFreq, "FREQ");

    auto* drive = addCard ("DRIVE", "Warm saturation", palette::drive, IDs::driveOn);
    drive->addKnob (IDs::driveAmount, "DRIVE");
    drive->addKnob (IDs::driveMix, "MIX");

    auto* chorus = addCard ("CHORUS", "Shimmer and movement", palette::chorus, IDs::chorusOn);
    chorus->addKnob (IDs::chorusRate, "RATE");
    chorus->addKnob (IDs::chorusDepth, "DEPTH");
    chorus->addKnob (IDs::chorusMix, "MIX");

    auto* echo = addCard ("ECHO", "Repeating delays", palette::echo, IDs::echoOn);
    echo->addKnob (IDs::echoTime, "TIME");
    echo->addKnob (IDs::echoFeedback, "REPEATS");
    echo->addKnob (IDs::echoTone, "TONE");
    echo->addKnob (IDs::echoMix, "MIX");
    echo->addToggle (IDs::echoPingPong, "Ping-pong");

    auto* reverb = addCard ("REVERB", "Room, hall, space", palette::reverb, IDs::reverbOn);
    reverb->addKnob (IDs::reverbSize, "SIZE");
    reverb->addKnob (IDs::reverbDamping, "DAMPING");
    reverb->addKnob (IDs::reverbWidth, "WIDTH");
    reverb->addKnob (IDs::reverbMix, "MIX");

    auto* pan8d = addCard ("8D AUDIO", "Circles around your head", palette::pan8d, IDs::pan8dOn);
    pan8d->addKnob (IDs::pan8dRate, "SPEED");
    pan8d->addKnob (IDs::pan8dDepth, "DEPTH");

    auto* width = addCard ("STEREO WIDTH", "Narrow to extra wide", palette::width, IDs::widthOn);
    width->addKnob (IDs::widthAmount, "WIDTH");

    auto* fades = addCard ("INTRO / OUTRO", "Fade in and fade out", palette::fades, IDs::fadesOn);
    fades->addKnob (IDs::fadeIn, "INTRO");
    fades->addKnob (IDs::fadeOut, "OUTRO");
    fades->addToggle (IDs::outro, "Fade out now");

    auto* output = addCard ("OUTPUT", "Level and dry/wet blend", palette::output, nullptr);
    output->addKnob (IDs::outputGain, "GAIN");
    output->addKnob (IDs::outputMix, "MIX");

    // Signal-flow strip mirrors the module switches
    auto node = [&] (const char* name, juce::Colour colour, const char* id) { return vibe::ChainView::Node { name, colour, state.getParameter (id) }; };
    chain = std::make_unique<vibe::ChainView> (std::vector<vibe::ChainView::Node> {
        node ("Speed", palette::speed, IDs::speedOn), node ("Lo-Fi", palette::lofi, IDs::lofiOn),
        node ("Filter", palette::filter, IDs::filterOn), node ("Bass", palette::bass, IDs::bassOn),
        node ("Drive", palette::drive, IDs::driveOn), node ("Chorus", palette::chorus, IDs::chorusOn),
        node ("Echo", palette::echo, IDs::echoOn), node ("Reverb", palette::reverb, IDs::reverbOn),
        node ("8D", palette::pan8d, IDs::pan8dOn), node ("Width", palette::width, IDs::widthOn),
        node ("Fades", palette::fades, IDs::fadesOn) },
        [this] { return meters.getOutputLevel(); });
    addAndMakeVisible (*chain);

    if (isStandalone())
    {
        playerBar = std::make_unique<vibe::PlayerBar> (audioProcessor.player);
        addAndMakeVisible (*playerBar);
    }

    setResizable (true, true);
    setResizeLimits (1120, 780, 2200, 1500);
    setSize (1320, isStandalone() ? 900 : 850);

    timerCallback();
    startTimerHz (15);
    handleCommandLine();
}

VibeFXEditor::~VibeFXEditor()
{
    setLookAndFeel (nullptr);
}

bool VibeFXEditor::isStandalone() const
{
    return audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone;
}

void VibeFXEditor::selectPreset (int index)
{
    if (index < 0 || (index == audioProcessor.getCurrentProgram() && index == shownProgram))
        return;

    audioProcessor.setCurrentProgram (index);
    audioProcessor.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
    timerCallback();
}

void VibeFXEditor::timerCallback()
{
    for (auto& card : cards)
        card->refresh();

    if (const auto program = audioProcessor.getCurrentProgram(); program != shownProgram)
    {
        shownProgram = program;
        presetBox.setSelectedId (program + 1, juce::dontSendNotification);
        presetDescription.setText (getFactoryPresets()[(size_t) program].description, juce::dontSendNotification);
    }
}

void VibeFXEditor::paint (juce::Graphics& g)
{
    g.fillAll (vibe::colours::background);

    auto header = headerArea.toFloat();
    auto logo = header.removeFromLeft (230.0f);

    g.setFont (vibe::font (28.0f, true));
    g.setGradientFill (juce::ColourGradient (vibe::colours::accent, logo.getX(), 0.0f,
                                             vibe::colours::accent2, logo.getX() + 150.0f, 0.0f, false));
    g.drawText ("VIBE FX", logo.removeFromTop (34.0f), juce::Justification::bottomLeft);

    g.setColour (vibe::colours::muted);
    g.setFont (vibe::font (11.5f));
    g.drawText ("Creator effects rack  |  Espirit Tech", logo, juce::Justification::topLeft);

    g.setFont (vibe::font (11.0f, true));
    g.drawText ("PRESET", presetBox.getBounds().translated (0, -16).withHeight (14).toFloat(), juce::Justification::left);
}

void VibeFXEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);

    headerArea = bounds.removeFromTop (56);
    {
        auto header = headerArea;
        header.removeFromLeft (240);
        bypassButton.setBounds (header.removeFromRight (110).withSizeKeepingCentre (110, 36));
        auto presetRow = header.removeFromLeft (420).withTrimmedTop (16);
        previousButton.setBounds (presetRow.removeFromLeft (36).withSizeKeepingCentre (36, 34));
        presetRow.removeFromLeft (6);
        nextButton.setBounds (presetRow.removeFromRight (36).withSizeKeepingCentre (36, 34));
        presetRow.removeFromRight (6);
        presetBox.setBounds (presetRow.withSizeKeepingCentre (presetRow.getWidth(), 34));
        presetDescription.setBounds (header.reduced (16, 0).withTrimmedTop (16));
    }

    bounds.removeFromTop (12);
    chain->setBounds (bounds.removeFromTop (46));
    bounds.removeFromTop (10);

    auto visuals = bounds.removeFromTop (juce::roundToInt (bounds.getHeight() * 0.3));
    meters.setBounds (visuals.removeFromRight (170));
    visuals.removeFromRight (12);
    visualiser.setBounds (visuals);
    bounds.removeFromTop (12);

    if (playerBar != nullptr)
    {
        playerBar->setBounds (bounds.removeFromTop (50));
        bounds.removeFromTop (12);
    }

    // 6 x 2 grid of effect cards
    constexpr int columns = 6, gap = 12;
    const auto rows = ((int) cards.size() + columns - 1) / columns;
    const auto cardWidth = (bounds.getWidth() - gap * (columns - 1)) / columns;
    const auto cardHeight = (bounds.getHeight() - gap * (rows - 1)) / rows;

    for (int i = 0; i < (int) cards.size(); ++i)
        cards[(size_t) i]->setBounds (bounds.getX() + (i % columns) * (cardWidth + gap),
                                      bounds.getY() + (i / columns) * (cardHeight + gap),
                                      cardWidth, cardHeight);
}

bool VibeFXEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    return playerBar != nullptr && files.size() == 1;
}

void VibeFXEditor::filesDropped (const juce::StringArray& files, int, int)
{
    if (playerBar != nullptr && ! files.isEmpty())
        playerBar->loadFile (juce::File (files[0]));
}

// Standalone-only command-line options, used to script demos and capture screenshots:
//   --load <audio file>  --preset <name|index>  --play  --mute  --snapshot <png> [--snapshot-delay <ms>]
void VibeFXEditor::handleCommandLine()
{
    if (! isStandalone())
        return;

    const auto args = juce::JUCEApplicationBase::getCommandLineParameterArray();
    auto valueAfter = [&] (const juce::String& flag)
    {
        const auto index = args.indexOf (flag);
        return index >= 0 && index + 1 < args.size() ? args[index + 1].unquoted() : juce::String();
    };

    if (const auto file = valueAfter ("--load"); file.isNotEmpty() && playerBar != nullptr)
        playerBar->loadFile (juce::File (file));

    if (const auto preset = valueAfter ("--preset"); preset.isNotEmpty())
    {
        const auto& presets = getFactoryPresets();
        auto index = preset.containsOnly ("0123456789") ? preset.getIntValue() : -1;

        for (int i = 0; i < (int) presets.size() && index < 0; ++i)
            if (presets[(size_t) i].name.equalsIgnoreCase (preset))
                index = i;

        selectPreset (index);
    }

    audioProcessor.muteOutput = args.contains ("--mute");

    if (args.contains ("--play"))
        audioProcessor.player.play();

    if (const auto snapshot = valueAfter ("--snapshot"); snapshot.isNotEmpty())
    {
        const auto delay = args.contains ("--snapshot-delay") ? valueAfter ("--snapshot-delay").getIntValue() : 3000;
        juce::Component::SafePointer<VibeFXEditor> safeThis (this);

        juce::Timer::callAfterDelay (delay, [safeThis, snapshot]
        {
            if (safeThis != nullptr)
            {
                const auto image = safeThis->createComponentSnapshot (safeThis->getLocalBounds(), true, 2.0f);
                juce::File file (snapshot);
                file.deleteFile();

                if (juce::FileOutputStream stream (file); stream.openedOk())
                    juce::PNGImageFormat().writeImageToStream (image, stream);
            }

            juce::JUCEApplicationBase::quit();
        });
    }
}
