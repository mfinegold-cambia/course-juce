
namespace tremolo {

  // Anoymous namespace inside the tremolo namespace, is accessible to everything in the tremolo namespace
  // Anonmous namespace is a good place to put local helper functions
  namespace {

    // auto& addParameterToProcessor(juce::AudioProcessor& processor, auto parameter) {
    //   // Dereference the parameter unique pointer, this gets the Reference of the unique pointer
    //   auto& result = *parameter;
    //   // This adds the juce::AudioParameterFloat& to the AudioProcessor
    //   processor.addParameter(parameter.release());
    //   // Return the Reference to the unique pointer
    //   return result;
    // }
    //
    // juce::AudioParameterFloat& createModulationRateParameter(juce::AudioProcessor& processor) {
    //   constexpr auto versionHint = 1;
    //   return addParameterToProcessor(
    //       processor,
    //       std::make_unique<juce::AudioParameterFloat>(
    //           juce::ParameterID{"modulation.rate", versionHint}, "Modulation rate",
    //           juce::NormalisableRange<float>{0.1f, 20.f, 0.01f, 0.4f}, 5.f,
    //           juce::AudioParameterFloatAttributes{}.withLabel("Hz")));
    // }

    juce::AudioParameterFloat& createModulationRateParameter(juce::AudioProcessor& processor) {
      constexpr auto versionHint = 1;

      // Create a unique pointer on the Stack to a new AudioParameterFlow instance on the Heap and populate the object
      // So make_unique creates 2 objects, a pointer var on the Stack and an object instance on the Heap
      auto parameter = std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "modulation.rate", versionHint } ,
        "Modulation Rate",
        // Start Range, End Range, Interval of scrolling through range, skew factor
        juce::NormalisableRange{ 0.1f, 20.f, 0.01f, 0.4f},
        // Default param value when GUI appears on screen
        0.5f,
        // Just adding a "Hz" string to end of value on GUI
        juce::AudioParameterFloatAttributes{}.withLabel("Hz"));

      // Here we Dereference the paramter Pointer, which gives us the reference to the pointer
      auto& parameterReference = *parameter;
      // Add the juce AudioParameterFloat pointer to the juce AudioProcessor
      // The release method is in the std::unique_pointer, it will automatically release/clean up the pointer
      processor.addParameter(parameter.release());
      // Here we Dereference the paramter Pointer, which gives us the reference to the pointer
      //auto& parameterReference = *parameter;
      // We return the reference to the pointer
      return parameterReference;
    }

  } // end namespace general

   // Parameters is a Struct
  // This is the Struct's Constructor definition, not the Struct definition ( which is in Parameters.h )
Parameters::Parameters(juce::AudioProcessor& processor)

  // Note that the Colon here is the Constructor inititalizer symbol instead of an Inheritance symbol since it's after the Constructor, not class
  // When PluginProcessor.h is created, it creates an instance of the Parameters.h Struct, and sets its modulation rate param to the AudioParameterFloat for modulatation rate
  : rate { createModulationRateParameter(processor) }
{

}
}  // namespace tremolo
