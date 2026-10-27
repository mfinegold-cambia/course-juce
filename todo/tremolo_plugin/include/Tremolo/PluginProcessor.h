#pragma once

namespace tremolo {
class PluginProcessor : public juce::AudioProcessor {
public:
  PluginProcessor();

  void prepareToPlay(double sampleRate, int expectedMaxFramesPerBlock) override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void releaseResources() override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  // The const after params says we CAN'T Change the argument object in this function, we are saying this method is READ-ONLY
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  // getBypassParameter is a method in the juce AudioProcessor class
  // here we are overriding it to tell JUCE that we are implementing our own Bypass param and don't need the JUCE provided one
  // This returns our AudioProcessorBool that controls the Bypass state
  // The const after it says this function is READ-ONLY, we will not be changing any state
  juce::AudioProcessorParameter* getBypassParameter() const override;

private:
  // Parameters constructor takes a ref to the juce::AudioProcessor
  // Here we Dereference this AudioProcessor class pass it to the Parameters Struct constructor as a reference
  Parameters parameters{*this};

  BypassTransitionSmoother bypassTransitionSmoother;

  Tremolo tremolo;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
}  // namespace tremolo
