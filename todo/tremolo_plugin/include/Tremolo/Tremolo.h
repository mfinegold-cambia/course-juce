#pragma once

namespace tremolo {
class Tremolo {
public:

  enum class LfoWaveform : size_t {
    sine = 0,
    triangle = 1,
  };

  Tremolo() {
    // Sets the freq in Hz, the true means that we can immediately change the freq ( without smoothing between old and new val )
    // We set it to a low freq of 5hz since this is an LFO oscillator
    // Frequency is how fast the Sine wave repeats itself, how fast the phase changes
    // At low freqs, Phase changes slowly
    for (auto& lfo : lfos) {
      lfo.setFrequency(5.0f, true);
    }
  }

  // Juce passes in the expectedMaxFramesPerBlock, which is the Maximum number of samples you can expect to get in the AudioBuffer, but not necessarily the actual number of samples in the AudioBuffer
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
    // We must call the juce Oscillator prepare method before we try to get the juce Oscillator to generate any samples
    for (auto& lfo : lfos) {
      lfo.prepare(processSpec);
    }

  }

  void setLfoWaveform(LfoWaveform waveform) {
    // jassert is a Juce assertion that will only run on Debug builds
    jassert(waveform == LfoWaveform::sine || waveform == LfoWaveform::triangle);
    lfoToSet = waveform;
  }

  // Host calls process when it has a new Block of samples to pass for processing
  // buffer.getNumSamples() is usually 512, that means the common Block size passed from the DAW to our plugin is usually 512 samples
  void process(juce::AudioBuffer<float>& buffer) noexcept {
    //DBG("Number of samples is " << buffer.getNumSamples());
    // Only Update the LFO waveform type once at beginning of the process method, which is when we get a new Block of samples but haven't started processing them yet
    updateLfoWaveform();

    // for each frame
    // std::views::iota just counts from 0 to the max num ( int ) of samples in buffer
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
        // We pass 0 since the juce Oscillator class adds its value to whatever val we pass in
        // juce Oscillator processSample advances the internal phase forward by one step, and adds the sample val (a float number between -1 and 1 ) to the generated output
        //   juce Oscillator processSample is what we need to call to actually generate samples that make up the waveform
        //   These samples are data points that are at the peak, trough or ramp of the waveform
        // Generate the LFO value (call juce Oscillator processSample ) on the Currently selected LFO ( either Sine or Triangle )
        const auto lfoValue = getNextLfoValue();
        // constexpr means the variable value is calculated at compile time ( not runtime ).  We can do this by initializing it with the floating point literal like this
        // The constexpr can optimize the compiler
        // The modulation depth is the amount of Amplitude that the LFO waveform should have
        constexpr auto modulationDepth = 0.4f;
        // The LFO val is the modulationDepth of the LFO ( amplitude ) multiplied by the sample val of the Lfo
        const auto modulationValue = (modulationDepth * lfoValue) + 1;


      // for each channel sample in the frame
      for (const auto channelIndex :std::views::iota(0, buffer.getNumChannels())) {
        // get the input sample
        // The inputSample value will always be 0 if the Host is not passing any audio in the AudioBuffer, there are 512 values of 0 in a Block passed from the Host
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        // This is where we Change the inputSample from the Host by multiplying it by the LFO sample val
        const auto outputSample = inputSample * modulationValue;

        // set the output sample
        // Put the calculated outputSample on the AudioBuffer in this position.
        // This overwrites whatever sample is already in this position in the AudioBuffer ( overwrites whatever sample the Host provided in this position )
        buffer.setSample(channelIndex, frameIndex, outputSample);
      }
    }
  }

  void reset() noexcept {
    // juce Oscillator has a reset method to reset the internal state of the oscillator after it is used
    for (auto& lfo : lfos) {
      lfo.reset();
    }
  }

private:
  // This private method can be static since it is not dependent on a Tremolo class instance
  // It has to be static since the function is being referenced in the lfos std::array below
  static float triangle(float phase) {
    // twoPi is a static constant provided by the Juce MathConstants Struct, which is  2 * 3.14
    const auto ft = phase / juce::MathConstants<float>::twoPi;
    return 4.f * std::abs(ft - std::floor(ft + 0.5f)) - 1.f;
  }

  float getNextLfoValue() {
    return lfos[juce::toUnderlyingType(currentLfo)].processSample(0.f);
  }

  void updateLfoWaveform() {
    if (currentLfo != lfoToSet) {
      currentLfo = lfoToSet;
    }
  }

  // We inititalize the juce::Oscillator to a lambda function ( the juce Oscillator constructor takes a function )
  // Lambda is an anonymous function
  // std::sin takes a float val for phase ( angle in radians from 0 to 2Pi), and returns a float sin val for that phase with amplitude in range form -1 to 1
  // An LFO is a periodic low freq ( < 20hz ) Oscillator that modulates the amplitude of an input signal to create fast changes in volume of signal
  // Here we create a Sine LFO since we are passing in a std::sin function
  // This is an example of using the JUCE dsp audio components instead of writing an oscillator from scratch in C++
  // The juce Oscillator constructor accepts a function, so we can pass in a Lambda function tha expects a float and returns a float
  // The phase represents the position ( float val between 0 and 2pii ) of the oscillator in 1 period of the waveform - the data point is a float phase val in the 0 to 2Pii range
  // As each float radian is supplied to std::sin, it writes out a portion of the Sine wave, eventually completing writing 1 period of the Sine Wave
  //juce::dsp::Oscillator<float> lfo { [] (auto phase) { return std::sin(phase); } };

  // std::array is the C++ version of the C raw array
  // It is a fixed size container with a size that must be known at compile time ( unlike a std::vector )
  // It is stored on the Stack, not the Heap ( like a std::vector )
  // It's a Template object that takes the type of juce Oscillator, and a size ( of 1 here, unsigned integer )
  std::array<juce::dsp::Oscillator<float>, 2u> lfos {
    // Sine oscillator
    juce::dsp::Oscillator<float> { [](auto phase) { return std::sin(phase);}},
    // Triangle oscillator
    // Since juce Oscillator takes a function as the param to inititalize it, we provide the triangle function to it
    juce::dsp::Oscillator<float> { triangle }
  };

  LfoWaveform currentLfo = LfoWaveform::sine;
  LfoWaveform lfoToSet = currentLfo;


};
}  // namespace tremolo
