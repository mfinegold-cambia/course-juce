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


  void setBypass(bool bypass) noexcept {
    const auto current = dryGain.getCurrentValue();
    const auto target = bypass ? 1.0f : 0.0f;
    const auto duration = crossfadeLengthSeconds * std::abs(target - current);

    dryGain.reset(sampleRateHz, duration);
    wetGain.reset(sampleRateHz, duration);

    dryGain.setCurrentAndTargetValue(current);
    dryGain.setTargetValue(target);

    wetGain.setCurrentAndTargetValue(1.0f - current);
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
      dryBuffer.copyFrom(channel, 0, buffer, channel, 0,
                         buffer.getNumSamples());
    }
    dryGain.applyGain(dryBuffer, buffer.getNumSamples());
  }

  void mixToWetBuffer(juce::AudioBuffer<float>& buffer) noexcept {
    jassert(buffer.getNumSamples() <= dryBuffer.getNumSamples());
    jassert(buffer.getNumChannels() <= dryBuffer.getNumChannels());

    wetGain.applyGain(buffer, buffer.getNumSamples());
    for (const auto channel : std::views::iota(0, buffer.getNumChannels())) {
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
  juce::LinearSmoothedValue<float> dryGain{0.f};
  // Create an instance of juce LinearSmoothedValue and initialize it to 1.f
  juce::LinearSmoothedValue<float> wetGain{1.f};

  // Create a new instance of juce AudioBuffer
  juce::AudioBuffer<float> dryBuffer;
};
}  // namespace tremolo
