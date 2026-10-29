namespace tremolo {

    namespace {

        struct SerializableParameters {
            float rate;
            bool bypass;
            juce::String waveform;

            // constexpr means this value is known at compiler time ( compiler optimiziation )
            static constexpr auto marshallingVersion = 1;

            template <typename Archive, typename T>

            static void serialize(Archive& archive, T& t) {
                // We only support a marshallingVersion of 1 right now
                if (archive.getVersion() != 1) {
                    return;
                }
                // The TREMOLO_PLUGIN_NAME var is set in CMakeLists.txt and added to target_compile_definitions, so is available to our C++ code
                archive(juce::named("pluginName", TREMOLO_PLUGIN_NAME));

                archive(juce::named("modulationRateHz", t.rate()), juce::named("bypass", t.bypass()),
                    juce::named("modulationWaveform", t.waveform()));
            }

        }; // end Struct


        // Get the current value of the GUI params ( modulation rate, bypass, waveform ) and populate the SerializableParameters Struct with it
        SerializableParameters from(const tremolo::Parameters& parameters) {
            return {
                .rate = parameters.rate.get(),
                .bypass = parameters.bypass.get(),
                // getCurrentChoiceName returns the name of the currently selected item in AudioParameterChoice
                .waveform = parameters.waveform.getCurrentChoiceName(),
            };
        }

    } // end anonymous namespace


void JsonSerializer::serialize(const Parameters& parameters,
                               juce::OutputStream& output) {

        // parametersToSerialize is a SerializableParameters Struct
    const auto parametersToSerialize = from(parameters);

        // ToVar::convert returns an std::optional ( meaning it may not be able to convert it so you have to check for not null )
    const auto json = juce::ToVar::convert(parametersToSerialize);
     // This json std::optional may be null so check it
    if (!json.has_value()) {
        return;
    }

    juce::JSON::writeToStream(output, *json, juce::JSON::FormatOptions{}.withSpacing(juce::JSON::Spacing::multiLine).withMaxDecimalPlaces(2));
}


juce::Result JsonSerializer::deserialize(juce::InputStream& input,
                                         Parameters& parameters) {
    juce::ignoreUnused(input, parameters);

    // TODO: deserialize parameters from the JSON input stream

    return juce::Result::fail("not implemented");
}

}  // namespace tremolo
