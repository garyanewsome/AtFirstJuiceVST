#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AtFirstJuiceVoice::AtFirstJuiceVoice(const VoiceParams& p)
    : voiceParams(p)
{
    // Basic envelope to prevent clicking on noteOn/noteOff
    juce::ADSR::Parameters params;
    params.attack = 0.01f;
    params.decay = 0.1f;
    params.sustain = 0.8f;
    params.release = 0.1f;
    adsr.setParameters(params);
}

bool AtFirstJuiceVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<AtFirstJuiceSound*>(sound) != nullptr;
}

void AtFirstJuiceVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
    adsr.setSampleRate(getSampleRate());
    adsr.noteOn();
    
    angle1 = 0.0;
    angle2 = 0.0;
    level = velocity;
    
    auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    delta1 = (cyclesPerSecond / getSampleRate()) * 2.0 * juce::MathConstants<double>::pi;
}

void AtFirstJuiceVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
    }
}

void AtFirstJuiceVoice::pitchWheelMoved(int) {}
void AtFirstJuiceVoice::controllerMoved(int, int) {}

void AtFirstJuiceVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    int wave1Choice = voiceParams.wave1 != nullptr ? static_cast<int>(voiceParams.wave1->load()) : 0;
    int wave2Choice = voiceParams.wave2 != nullptr ? static_cast<int>(voiceParams.wave2->load()) : 0;
    float detuneVal = voiceParams.detune != nullptr ? voiceParams.detune->load() : 0.0f;
    float mixVal = voiceParams.mix != nullptr ? voiceParams.mix->load() : 0.5f;

    // Calculate delta for Osc 2 based on detune (in semitones)
    // frequency_ratio = 2 ^ (detune / 12)
    double detuneRatio = std::pow(2.0, detuneVal / 12.0);
    delta2 = delta1 * detuneRatio;

    juce::ADSR::Parameters adsrParams;
    adsrParams.attack = voiceParams.attack != nullptr ? voiceParams.attack->load() : 0.01f;
    adsrParams.decay = voiceParams.decay != nullptr ? voiceParams.decay->load() : 0.1f;
    adsrParams.sustain = voiceParams.sustain != nullptr ? voiceParams.sustain->load() : 0.8f;
    adsrParams.release = voiceParams.release != nullptr ? voiceParams.release->load() : 0.1f;
    adsr.setParameters(adsrParams);

    auto getSample = [](int choice, double angle) {
        if (choice == 0) // Sine
            return static_cast<float>(std::sin(angle));
        else if (choice == 1) // Saw
            return static_cast<float>((angle / juce::MathConstants<double>::pi) - 1.0);
        else if (choice == 2) // Triangle
            return static_cast<float>((2.0 / juce::MathConstants<double>::pi) * std::asin(std::sin(angle)));
        return 0.0f;
    };

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float env = adsr.getNextSample();
        
        float s1 = getSample(wave1Choice, angle1);
        float s2 = getSample(wave2Choice, angle2);
        
        // Linear mix: 0.0 = Osc 1 only, 1.0 = Osc 2 only
        float currentSample = (s1 * (1.0f - mixVal) + s2 * mixVal) * level * env;

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample(channel, startSample + sample, currentSample);
        }

        angle1 += delta1;
        while (angle1 >= 2.0 * juce::MathConstants<double>::pi) angle1 -= 2.0 * juce::MathConstants<double>::pi;
        
        angle2 += delta2;
        while (angle2 >= 2.0 * juce::MathConstants<double>::pi) angle2 -= 2.0 * juce::MathConstants<double>::pi;

        if (!adsr.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

//==============================================================================
AtFirstJuiceAudioProcessor::AtFirstJuiceAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                           .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initializes sine wave formula for our LFO component natively
    lfo.initialise([](float x) { return std::sin(x); });

    synth.addSound(new AtFirstJuiceSound());
    
    VoiceParams waveParams;
    waveParams.wave1 = apvts.getRawParameterValue("WAVEFORM");
    waveParams.wave2 = apvts.getRawParameterValue("OSC2_WAVEFORM");
    waveParams.detune = apvts.getRawParameterValue("OSC2_DETUNE");
    waveParams.mix = apvts.getRawParameterValue("OSC_MIX");
    waveParams.attack = apvts.getRawParameterValue("ATTACK");
    waveParams.decay = apvts.getRawParameterValue("DECAY");
    waveParams.sustain = apvts.getRawParameterValue("SUSTAIN");
    waveParams.release = apvts.getRawParameterValue("RELEASE");
    
    // Add polyphonic voices
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new AtFirstJuiceVoice(waveParams));
        
    masterVolumeParam = apvts.getRawParameterValue("MASTER_VOL");
    filterCutoff = apvts.getRawParameterValue("CUTOFF");
    filterResonance = apvts.getRawParameterValue("RESONANCE");
    lfoRate = apvts.getRawParameterValue("LFO_RATE");
    lfoDepth = apvts.getRawParameterValue("LFO_DEPTH");
    distortionDepth = apvts.getRawParameterValue("DISTORTION");
}

AtFirstJuiceAudioProcessor::~AtFirstJuiceAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AtFirstJuiceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("WAVEFORM", 1), "OSC 1 Wave", juce::StringArray{"Sine", "Saw", "Triangle"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("OSC2_WAVEFORM", 1), "OSC 2 Wave", juce::StringArray{"Sine", "Saw", "Triangle"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("OSC2_DETUNE", 1), "Detune", -12.0f, 12.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("OSC_MIX", 1), "OSC Mix", 0.0f, 1.0f, 0.5f));
        
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("MASTER_VOL", 1), "Master Vol", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ATTACK", 1), "Attack", 0.001f, 2.0f, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("DECAY", 1), "Decay", 0.001f, 2.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("SUSTAIN", 1), "Sustain", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("RELEASE", 1), "Release", 0.001f, 4.0f, 0.1f));
        
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CUTOFF", 1), "Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("RESONANCE", 1), "Resonance", 0.0f, 1.0f, 0.0f));
        
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("LFO_RATE", 1), "LFO Rate Hz", 0.1f, 20.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("LFO_DEPTH", 1), "LFO Depth", 0.0f, 1.0f, 0.0f));
        
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("DISTORTION", 1), "Drive", 1.0f, 10.0f, 1.0f));
        
    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String AtFirstJuiceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AtFirstJuiceAudioProcessor::acceptsMidi() const
{
    return true;
}

bool AtFirstJuiceAudioProcessor::producesMidi() const
{
    return false;
}

bool AtFirstJuiceAudioProcessor::isMidiEffect() const
{
    return false;
}

double AtFirstJuiceAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AtFirstJuiceAudioProcessor::getNumPrograms()
{
    return 1;
}

int AtFirstJuiceAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AtFirstJuiceAudioProcessor::setCurrentProgram(int /*index*/) {}

const juce::String AtFirstJuiceAudioProcessor::getProgramName(int /*index*/)
{
    return {};
}

void AtFirstJuiceAudioProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/) {}

//==============================================================================
void AtFirstJuiceAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Inform synthesiser of sample rate changes
    synth.setCurrentPlaybackSampleRate(sampleRate);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    filter.prepare(spec);
    // Standard LadderFilter Mode to LPF12 (12dB per oct Low-Pass)
    filter.setMode(juce::dsp::LadderFilterMode::LPF12);
    
    lfo.prepare(spec);
}

void AtFirstJuiceAudioProcessor::releaseResources()
{
}

bool AtFirstJuiceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void AtFirstJuiceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
        
    // Merges incoming clicks from the UI keyboard into the buffer stream
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
        
    // Render the active synth voices
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    // Evaluate LFO against target cutoff state
    float currentResonance = filterResonance != nullptr ? filterResonance->load() : 0.0f;
    float currentCutoff = filterCutoff != nullptr ? filterCutoff->load() : 20000.0f;
    float currentLfoRate = lfoRate != nullptr ? lfoRate->load() : 1.0f;
    float currentLfoDepth = lfoDepth != nullptr ? lfoDepth->load() : 0.0f;
    
    lfo.setFrequency(currentLfoRate);
    filter.setResonance(currentResonance);
    
    // Evaluate 1 LFO step per block for performance, scaling target cutoff depth smoothly
    float lfoVal = lfo.processSample(0.0f);
    // Modulate cutoff frequency logarithmically around active user position using mathematical depth constraints
    float modCutoff = currentCutoff * (1.0f - (currentLfoDepth * 0.5f * (lfoVal + 1.0f))); 
    if (modCutoff < 20.0f) modCutoff = 20.0f;
    
    filter.setCutoffFrequencyHz(modCutoff);
    
    // Process block completely across filter DSP framework naturally
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);
    
    // Apply soft-clipping analog waveshaper naturally before the main volume envelope
    float drive = distortionDepth != nullptr ? distortionDepth->load() : 1.0f;
    if (drive > 1.0f)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                // Divide the hyperbolic tangent logically by its max offset threshold to prevent volume spikes
                channelData[sample] = static_cast<float>(std::tanh(channelData[sample] * drive) / std::tanh(drive));
            }
        }
    }

    // Apply master volume using thread-safe, non-blocking atomic fetch
    float volume = masterVolumeParam != nullptr ? masterVolumeParam->load() : 0.5f;
    buffer.applyGain(volume);
}

//==============================================================================
bool AtFirstJuiceAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AtFirstJuiceAudioProcessor::createEditor()
{
    return new AtFirstJuiceAudioProcessorEditor(*this);
}

//==============================================================================
void AtFirstJuiceAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Serialize parameter values
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AtFirstJuiceAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Deserialize parameter values
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AtFirstJuiceAudioProcessor();
}
