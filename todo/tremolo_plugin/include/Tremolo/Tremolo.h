#pragma once

namespace tremolo {
class Tremolo {
public:
  Tremolo() {
    // Sets the freq in Hz, the true means that we can immediately change the freq ( without smoothing between old and new val )
    lfo.setFrequency(5.0f, true);
  }

  void prepare(double sampleRate, int expectedMaxFramesPerBlock) {

    // ProcessSpec is a Struct that we need to pass to the juce Oscillator prepare method
    const juce::dsp::ProcessSpec processSpec {
      .sampleRate = sampleRate,
      // the ProcessSpec maximumBlockSize takes a uint32 val so we have to cast to it
      .maximumBlockSize = static_cast<uint32_t>(expectedMaxFramesPerBlock),
      .numChannels = 1u
    };
    // the juce Oscillator prepare method takes a ProcesSpec Struct to give the Oscillator context info before we use it
    // the juce Oscillator prepare method prepares the Oscillator with contextual info before it's used
    lfo.prepare(processSpec);
  }

  void process(juce::AudioBuffer<float>& buffer) noexcept {
    // for each frame
    // std::views::iota just counts from 0 to the max num ( int ) of samples in buffer
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
        // We pass 0 since the juce Oscillator class adds its value to whatever val we pass in
        const auto lfoValue = lfo.processSample(0.f);

      // TODO: calculate the modulation value

      // for each channel sample in the frame
      for (const auto channelIndex :std::views::iota(0, buffer.getNumChannels())) {
        // get the input sample
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        // TODO: modulate the sample
        const auto outputSample = inputSample;

        // set the output sample
        buffer.setSample(channelIndex, frameIndex, outputSample);
      }
    }
  }

  void reset() noexcept {
    // juce Oscillator has a reset method to reset the internal state of the oscillator after it is used
    lfo.reset();
  }

private:

  // We inititalize the juce::Oscillator to a lambda function ( the juce Oscillator constructor takes a function )
  // Lambda is an anonymous function
  // std::sin takes a float val for phase ( angle in radians from 0 to 2Pi), and returns a float sin val for that phase with amplitude in range form -1 to 1
  // An LFO is a periodic low freq ( < 20hz ) Oscillator that modulates the amplitude of an input signal to create fast changes in volume of signal
  // Here we create a Sine LFO since we are passing in a std::sin function
  // This is an example of using the JUCE dsp audio components instead of writing an oscillator from scratch in C++
  // The juce Oscillator constructor accepts a function, so we can pass in a Lambda function tha expects a float and returns a float
  juce::dsp::Oscillator<float> lfo { [] (auto phase) { return std::sin(phase); } };
};
}  // namespace tremolo
