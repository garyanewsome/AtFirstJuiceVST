#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class AtFirstJuiceAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AtFirstJuiceAudioProcessorEditor (AtFirstJuiceAudioProcessor&);
    ~AtFirstJuiceAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AtFirstJuiceAudioProcessor& audioProcessor;

    juce::Slider waveformSlider, osc2WaveformSlider, detuneSlider, mixSlider;
    juce::Slider masterVolSlider, attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Slider cutoffSlider, resonanceSlider, lfoRateSlider, lfoDepthSlider, distortionSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> waveformAttachment, osc2WaveformAttachment, detuneAttachment, mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAtt, decayAtt, sustainAtt, releaseAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAtt, resAtt, lfoRateAtt, lfoDepthAtt, distortionAtt;

    juce::MidiKeyboardComponent midiKeyboard;

    juce::TextButton randomizeButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtFirstJuiceAudioProcessorEditor)
};
