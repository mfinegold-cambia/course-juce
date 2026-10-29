#pragma once

namespace tremolo {
/**
 * Class facilitating transition to and from bypassed state over a single block.
 *
 * It allows 2 things:
 *  - detecting that a bypass state transition takes place
 *  - fade-in and fade-out of dry (unprocessed) and wet (processed) buffers
 *    according to the transition type
 *
 * Use the following code in the processBlock() method of your plugin processor:
 *
 * @code
 * //...
 *
 * bypassTransitionSmoother.setBypass(parameters.bypassed);
 *
 * if (bypassTransitionSmoother.isTransitioning()) {
 *   bypassTransitionSmoother.setDryBuffer(buffer);
 *
 *   yourAudioEffectClassInstance.process(buffer);
 *
 *   bypassTransitionSmoother.mixToWetBuffer(buffer);
 *
 *   return;
 * }
 *
 * // avoid processing if bypassed, process otherwise...
 * @endcode
 *
 * Alternatively, you can unconditionally call setDryBuffer() and
 * mixToWetBuffer(), like this:
 *
 * @code
 * //...
 *
 * bypassTransitionSmoother.setBypass(parameters.bypassed);
 *
 * if (parameters.bypassed && !bypassTransitionSmoother.isTransitioning()) {
 *   // avoid processing if the plugin is bypassed
 *   return;
 * }
 *
 * bypassTransitionSmoother.setDryBuffer(buffer);
 * yourAudioEffectClassInstance.process(buffer);
 * bypassTransitionSmoother.mixToWetBuffer(buffer);
 *
 * // no more processing necessary
 * @endcode
 *
 * Remember to call prepare() in prepareToPlay(),
 * setBypassForced() in setStateInformation(), and reset() in
 * releaseResources().
 */
class BypassTransitionSmoother {
public:

  // Since this Constructor only has 1 parameter, we mark it as explicit, as a best practice
  // We inititalize the crossfade length to 10ms
  explicit BypassTransitionSmoother(double crossfadeLengthSecondsValue = 0.01)
      : crossfadeLengthSeconds{crossfadeLengthSecondsValue} {

    // juce jassert method to error out if we pass in a negative crossfadeLengthSeconds value
    jassert(0.0 < crossfadeLengthSeconds);
  }


  void prepare(const juce::dsp::ProcessSpec& spec) {

    sampleRateHz = spec.sampleRate;

    // Inititalize the new AudioBuffer with number of channels, and max number of samples
    // ProcessSpec passes the num samples and num channels as uint32 so we cast them to int since AudioBuffer setSize requires int
    dryBuffer.setSize(static_cast<int>(spec.numChannels),
                      static_cast<int>(spec.maximumBlockSize));

    // juce LinearSmoothedValue reset method takes the sample rate and the crossfade length in seconds
    // it calculates how many audio samples are needed to complete a transition over the time period of crossfadeLengthSeconds
    // crossfadeLengthSeconds is inititalized to 0.01 ( 15 ms )
    dryGain.reset(spec.sampleRate, crossfadeLengthSeconds);
    // juce LinearSmoothedValue reset method takes the sample rate and the crossfade length in seconds
    // it calculates how many audio samples are needed to complete a transition over the time period of crossfadeLengthSeconds
    wetGain.reset(spec.sampleRate, crossfadeLengthSeconds);
  }


  // Here we get the current Bypass boolean value from the GUI called from PluginProcessor.cpp
  void setBypass(bool bypass) noexcept {

    // dryGain juce LinearSmoothedValue.getCurrentValue gets the current value of the Ramp ( which is 0.f to start )
    const auto current = dryGain.getCurrentValue();

    // If Bypass is selected, we start the target var to 1.0f
    const auto target = bypass ? 1.0f : 0.0f;

    // crossfadeLengthSeconds by defauilt is set to 0.01  ( 10 millis)
    // std::abs just get the Absolute value of the number, which just makes a negative number positive, that's all it does, like if a num is -42, it returns 42
    // duration is set to 0.01 ( 10 milis here ) since target is 1 and current is  0   ( 0.01 * 1 = 0.01  )
    const auto duration = crossfadeLengthSeconds * std::abs(target - current);

    // Just sets the juce LinearSmoothedValue to the sample rate and duration of  0.01   ( 10 millis )
    dryGain.reset(sampleRateHz, duration);
    wetGain.reset(sampleRateHz, duration);

    // Dry
    // sets juce LinearSmoothedValue current and target value to 0.0f
    dryGain.setCurrentAndTargetValue(current);
    // sets juce LinearSmoothedValue target to 1
    dryGain.setTargetValue(target);

    // Wet
    // sets juce LinearSmoothedValue current and target value to 1.0f
    wetGain.setCurrentAndTargetValue(1.0f - current);
    // sets juce LinearSmoothedValue target to 0
    wetGain.setTargetValue(1.0f - target);
  }


  void setBypassForced(bool bypass) noexcept {
    dryGain.setCurrentAndTargetValue(bypass ? 1.0f : 0.0f);
    wetGain.setCurrentAndTargetValue(1.0f - dryGain.getTargetValue());
  }

  [[nodiscard]] bool isTransitioning() const noexcept {
    return dryGain.isSmoothing() || wetGain.isSmoothing();
  }


  void setDryBuffer(const juce::AudioBuffer<float>& buffer) noexcept {

    jassert(buffer.getNumSamples() <= dryBuffer.getNumSamples());
    jassert(buffer.getNumChannels() <= dryBuffer.getNumChannels());


    for (const auto channel : std::views::iota(0, buffer.getNumChannels())) {
      // dryBuffer is the new AudioBuffer we created below
      // AudioBuffer copyFrom copies all the samples from the juce AudioBuffer into our own dryBuffer AudioBuffer
      // args are  channel, dest start sample 0, buffer to copy from, channel, source start sample 0, number of source samples to copy
      // So we populate dryBuffer Audiobuffer with all the samples from the source Audiobuffer
      dryBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());
    }

    // Applies a smoothed gain to all of the samples in our dryBuffer AudioBuffer ( which were copied from the juce Audiobuffer )
    dryGain.applyGain(dryBuffer, buffer.getNumSamples());
  }

  // After this method completes, our juce AudioBuffer now has dry and wet samples mixed together in right proportions to prevent large Discontinuities in waves when Bypass switch is flipped back and forth
  void mixToWetBuffer(juce::AudioBuffer<float>& buffer) noexcept {

    jassert(buffer.getNumSamples() <= dryBuffer.getNumSamples());
    jassert(buffer.getNumChannels() <= dryBuffer.getNumChannels());

    // Applies a smoothed gain to all of the samples in our wetBuffer AudioBuffer
    wetGain.applyGain(buffer, buffer.getNumSamples());

    for (const auto channel : std::views::iota(0, buffer.getNumChannels())) {
      // here we Add samples from our dryBuffer Audiobuffer into our live juce AudioBuffer
      // We add samples to whatever is already in our live juce AudioBuffer, we do not completely overwrite it
      buffer.addFrom(channel, 0, dryBuffer, channel, 0, buffer.getNumSamples());
    }
  }


  void reset() noexcept {

    setBypassForced(false);
    // Clears all samples out of our all channels of our dry AudioBuffer
    dryBuffer.clear();
  }

private:

  double crossfadeLengthSeconds = 0.0;
  double sampleRateHz = 0.0;

  // Create an instance of juce LinearSmoothedValue and initialize it to 0.f
  // juce LinearSmoothedValue ramps a numeric value linearly from an old value to a new target value over a specified duration
  juce::LinearSmoothedValue<float> dryGain{0.f};
  // Create an instance of juce LinearSmoothedValue and initialize it to 1.f
  juce::LinearSmoothedValue<float> wetGain{1.f};

  // Create a new instance of juce AudioBuffer
  juce::AudioBuffer<float> dryBuffer;
};
}  // namespace tremolo
