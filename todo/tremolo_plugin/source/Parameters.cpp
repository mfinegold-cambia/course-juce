
namespace tremolo {

  // general namespace inside the tremolo namespace
  namespace {
    juce::AudioParameterFloat& createModulationRateParameter(juce::AudioProcessor& processor) {
      constexpr auto versionHint = 1;
      auto parameter = std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "modulation.rate", versionHint } ,
        "Modulation Rate",
        juce::NormalisableRange{ 0.1f, 20.f, 0.01f, 0.4f},
        0.5f,
        juce::AudioParameterFloatAttributes{}.withLabel(" Hz"));

      // Add the juce AudioParameterFloat pointer to the juce AudioProcessor
      processor.addParameter(parameter.release());
      // Get a reference from the pointer
      auto& parameterReference = *parameter;
      return parameterReference;
    }
  } // end namespace general

   // Parameters is a Struct
  // This is the Constructor definition, not the Struct definition ( which is in Parameters.h )
Parameters::Parameters(juce::AudioProcessor& processor)

  // Note that the Colon here is the Constructor inititalizer symbol instead of an Inheritance symbol since it's after the Constructor, not class
  // When PluginProcessor.h is created, it creates an instance of the Parameters.h Struct, and sets its modulation rate param to the AudioParameterFloat for modulatation rate
  : rate { createModulationRateParameter(processor) }
{

}
}  // namespace tremolo
