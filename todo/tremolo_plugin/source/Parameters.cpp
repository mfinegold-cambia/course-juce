
namespace tremolo {

  // Anoymous namespace inside the tremolo namespace, is accessible to everything in the tremolo namespace
  // Anonmous namespace is a good place to put local helper functions
  namespace {

    // Helper method to be used by all of our other create param functions in this anonyomous namespace
    // The parameter comes in as a unique pointer whose ownership has been transferred to parameter in this function
    auto& addParameterToProcessor(juce::AudioProcessor& processor, auto parameter) {
      // Here we Dereference the paramter Pointer, which gives us the reference to the pointer
      auto& parameterReference = *parameter;
      // Add the juce AudioParameter pointer to the juce AudioProcessor
      // The release method is in the std::unique_pointer, it will automatically release/clean up the pointer
      processor.addParameter(parameter.release());
      //auto& parameterReference = *parameter;
      // We return the reference to the pointer
      return parameterReference;
    }

    juce::AudioParameterFloat& createModulationRateParameter(juce::AudioProcessor& processor) {
      constexpr auto versionHint = 1;

      // Create a unique pointer on the Stack to a new AudioParameterFloat instance on the Heap and populate the object
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

      // parameter is a unique pointer so if we just pass parameter to addParameterToProcessor, it will be COPIED which means we will have 2 pointers, and a Unique pointer must have only 1 owner
      // So we use std::move to Transfer ownership of the parameter pointer to the function we're calling ( addParameterToProcessor )
      // std::move says we are giving up ownership of the unique parameter pointer in this function, and Transferring ownership to the function we're calling
      return addParameterToProcessor(processor, std::move(parameter));
    }


    juce::AudioParameterBool& createBypassParameter(juce::AudioProcessor& processor) {
      constexpr auto versionHint = 1;

      // Create a unique pointer on the Stack to a new AudioParameterBool instance on the Heap and populate the object
      // So make_unique creates 2 objects, a pointer var on the Stack and an object instance on the Heap
      auto parameter = std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "bypass", versionHint },
        "Bypass",
        false);

      // parameter is a unique pointer so if we just pass parameter to addParameterToProcessor, it will be copied which means we will have 2 pointers and 1 will be leaked
      // So we use std::move to Transfer ownership of the parameter pointer to the function we're calling ( addParameterToProcessor )
      // std::move says we are giving up ownership of the unique parameter pointer in this function, and Transferring ownership to the function we're calling
      return addParameterToProcessor(processor, std::move(parameter));

    }

  } // end namespace general

   // Parameters is a Struct
  // This is the Struct's Constructor definition, not the Struct definition ( which is in Parameters.h )
Parameters::Parameters(juce::AudioProcessor& processor)

  // Note that the Colon here is the Constructor inititalizer symbol instead of an Inheritance symbol since it's after the Constructor, not class
  // When PluginProcessor.h is created, it creates an instance of the Parameters.h Struct, and sets its modulation rate param to the AudioParameterFloat for modulatation rate
  : rate { createModulationRateParameter(processor) },
    bypass{ createBypassParameter(processor)}
{

}
}  // namespace tremolo
