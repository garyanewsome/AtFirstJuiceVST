#pragma once

#include <JuceHeader.h>

//==============================================================================
class AtFirstJuiceSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

struct VoiceParams {
    std::atomic<float>* wave1 = nullptr;
    std::atomic<float>* wave2 = nullptr;
    std::atomic<float>* detune = nullptr;
    std::atomic<float>* mix = nullptr;
    std::atomic<float>* attack = nullptr;
    std::atomic<float>* decay = nullptr;
    std::atomic<float>* sustain = nullptr;
    std::atomic<float>* release = nullptr;
};

//==============================================================================
class AtFirstJuiceVoice : public juce::SynthesiserVoice
{
public:
    AtFirstJuiceVoice(const VoiceParams& params);

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    
    // Real-Time Safe audio render block (no heap allocs, sys calls, or locks)
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    double angle1 = 0.0;
    double delta1 = 0.0;
    double angle2 = 0.0;
    double delta2 = 0.0;
    double level = 0.0;
    juce::ADSR adsr; // Basic ADSR managed by faders
    VoiceParams voiceParams;
};

//==============================================================================
class AtFirstJuiceAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AtFirstJuiceAudioProcessor();
    ~AtFirstJuiceAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // ValueTreeState for atomic parameter access (Thread-Safe & Real-Time Safe)
    juce::AudioProcessorValueTreeState apvts;
    
    // Tracks on-screen piano clicks
    juce::MidiKeyboardState keyboardState;

private:
    juce::Synthesiser synth;
    juce::dsp::LadderFilter<float> filter;
    juce::dsp::Oscillator<float> lfo;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    std::atomic<float>* masterVolumeParam = nullptr;
    std::atomic<float>* filterCutoff = nullptr;
    std::atomic<float>* filterResonance = nullptr;
    std::atomic<float>* lfoRate = nullptr;
    std::atomic<float>* lfoDepth = nullptr;
    std::atomic<float>* distortionDepth = nullptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AtFirstJuiceAudioProcessor)
};
