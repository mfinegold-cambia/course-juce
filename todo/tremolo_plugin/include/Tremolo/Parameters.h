#pragma once

namespace tremolo {

struct Parameters {

  // explicit is used on single arg constructors and is a good practice
  // This is the Constructor definition for the Parameters Struct
  explicit Parameters(juce::AudioProcessor&);

  juce::AudioParameterFloat& rate;
  juce::AudioParameterBool& bypass;
  juce::AudioParameterChoice& waveform;

  // We do this to avoid any Copies being made of the juce::AudioProcessor ref, since it is a ref and we could create dangling refs by accident
  // This deletes the Copy constructor and copy assignment operator
  JUCE_DECLARE_NON_COPYABLE(Parameters)
  // This prevents accidental moves of this class
  JUCE_DECLARE_NON_MOVEABLE(Parameters)
};
}  // namespace tremolo
