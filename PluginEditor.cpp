#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
AtFirstJuiceAudioProcessorEditor::AtFirstJuiceAudioProcessorEditor(
    AtFirstJuiceAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      midiKeyboard(p.keyboardState,
                   juce::MidiKeyboardComponent::horizontalKeyboard) {
  // A quick local helper function to set up our sliders elegantly
  auto setupSlider =
      [this](
          juce::Slider &s,
          std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
              &att,
          const juce::String &id) {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        att = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, id, s);
        addAndMakeVisible(s);
      };

  setupSlider(waveformSlider, waveformAttachment, "WAVEFORM");
  setupSlider(osc2WaveformSlider, osc2WaveformAttachment, "OSC2_WAVEFORM");
  setupSlider(detuneSlider, detuneAttachment, "OSC2_DETUNE");
  setupSlider(mixSlider, mixAttachment, "OSC_MIX");
  setupSlider(attackSlider, attackAtt, "ATTACK");
  setupSlider(decaySlider, decayAtt, "DECAY");
  setupSlider(sustainSlider, sustainAtt, "SUSTAIN");
  setupSlider(releaseSlider, releaseAtt, "RELEASE");
  setupSlider(cutoffSlider, cutoffAtt, "CUTOFF");
  setupSlider(resonanceSlider, resAtt, "RESONANCE");
  setupSlider(lfoRateSlider, lfoRateAtt, "LFO_RATE");
  setupSlider(lfoDepthSlider, lfoDepthAtt, "LFO_DEPTH");
  setupSlider(distortionSlider, distortionAtt, "DISTORTION");
  setupSlider(masterVolSlider, masterVolAttachment, "MASTER_VOL");

  randomizeButton.setButtonText("Randomize");
  randomizeButton.onClick = [this] {
    for (auto *param : audioProcessor.getParameters()) {
      if (auto *rangedId =
              dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
        if (rangedId->paramID != "MASTER_VOL" &&
            rangedId->paramID != "OSC2_DETUNE") {
          float randomVal = juce::Random::getSystemRandom().nextFloat();
          rangedId->setValueNotifyingHost(randomVal);
        }
      }
    }
  };
  addAndMakeVisible(randomizeButton);

  addAndMakeVisible(midiKeyboard);

  // Widen to fit 14 parameter columns side-by-side!
  setSize(1170, 420);

  setAlwaysOnTop(false);
}

AtFirstJuiceAudioProcessorEditor::~AtFirstJuiceAudioProcessorEditor() {}

//==============================================================================
void AtFirstJuiceAudioProcessorEditor::paint(juce::Graphics &g) {
  auto baseColor =
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);

  // Create a subtle horizontal gradient: warm pink-gray to 'lemonade'
  // yellow-gray
  juce::ColourGradient gradient(
      baseColor.overlaidWith(juce::Colours::deeppink.withAlpha(0.04f)), 0.0f,
      0.0f, baseColor.overlaidWith(juce::Colours::yellow.withAlpha(0.08f)),
      static_cast<float>(getWidth()), 0.0f, false);

  g.setGradientFill(gradient);
  g.fillAll();

  g.setColour(juce::Colours::white);
  g.setFont(24.0f);
  g.drawText("AtFirstJuice", 20, 10, 400, 30, juce::Justification::left, true);

  g.setFont(14.0f);

  int sliderWidth = 70;
  int spacing = 10;
  int startX = 25;
  int yOff = 80;

  auto drawLabel = [&](const juce::String &text, int x) {
    g.drawFittedText(text, x, yOff - 25, sliderWidth, 20,
                     juce::Justification::centred, 1);
  };

  // Dynamically draw the labels safely mapped identically to resized()
  drawLabel("Osc 1", startX);
  drawLabel("Osc 2", startX + (sliderWidth + spacing) * 1);
  drawLabel("Mix", startX + (sliderWidth + spacing) * 2);
  drawLabel("Attack", startX + (sliderWidth + spacing) * 3);
  drawLabel("Decay", startX + (sliderWidth + spacing) * 4);
  drawLabel("Sustain", startX + (sliderWidth + spacing) * 5);
  drawLabel("Release", startX + (sliderWidth + spacing) * 6);
  drawLabel("Cutoff", startX + (sliderWidth + spacing) * 7);
  drawLabel("Res", startX + (sliderWidth + spacing) * 8);
  drawLabel("LFO Hz", startX + (sliderWidth + spacing) * 9);
  drawLabel("LFO Dpt", startX + (sliderWidth + spacing) * 10);
  drawLabel("Drive", startX + (sliderWidth + spacing) * 11);
  drawLabel("Detune", startX + (sliderWidth + spacing) * 12);
  drawLabel("Vol", startX + (sliderWidth + spacing) * 13);
}

void AtFirstJuiceAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  // Place piano keyboard at bottom
  midiKeyboard.setBounds(area.removeFromBottom(80));

  randomizeButton.setBounds(getWidth() - 110, 10, 100, 30);

  int sliderWidth = 70;
  int spacing = 10;
  int startX = 25;
  int yOff = 80;
  int height = 220;

  // Horizontally arrange all parameters mapping the exact paint() logic layout
  // coordinates
  waveformSlider.setBounds(startX, yOff, sliderWidth, height);
  osc2WaveformSlider.setBounds(startX + (sliderWidth + spacing) * 1, yOff,
                               sliderWidth, height);
  mixSlider.setBounds(startX + (sliderWidth + spacing) * 2, yOff, sliderWidth,
                      height);
  attackSlider.setBounds(startX + (sliderWidth + spacing) * 3, yOff,
                         sliderWidth, height);
  decaySlider.setBounds(startX + (sliderWidth + spacing) * 4, yOff, sliderWidth,
                        height);
  sustainSlider.setBounds(startX + (sliderWidth + spacing) * 5, yOff,
                          sliderWidth, height);
  releaseSlider.setBounds(startX + (sliderWidth + spacing) * 6, yOff,
                          sliderWidth, height);
  cutoffSlider.setBounds(startX + (sliderWidth + spacing) * 7, yOff,
                         sliderWidth, height);
  resonanceSlider.setBounds(startX + (sliderWidth + spacing) * 8, yOff,
                            sliderWidth, height);
  lfoRateSlider.setBounds(startX + (sliderWidth + spacing) * 9, yOff,
                          sliderWidth, height);
  lfoDepthSlider.setBounds(startX + (sliderWidth + spacing) * 10, yOff,
                           sliderWidth, height);
  distortionSlider.setBounds(startX + (sliderWidth + spacing) * 11, yOff,
                             sliderWidth, height);
  detuneSlider.setBounds(startX + (sliderWidth + spacing) * 12, yOff,
                         sliderWidth, height);
  masterVolSlider.setBounds(startX + (sliderWidth + spacing) * 13, yOff,
                            sliderWidth, height);
}
