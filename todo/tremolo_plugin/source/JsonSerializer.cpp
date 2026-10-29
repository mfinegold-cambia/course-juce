namespace tremolo {

    namespace {
        struct SerializableParameters {

            float rate;
            bool bypass;
            juce::String waveform;

            // the juce ToVar::convert method requires that we declare a marshallingVersion variable in this Struct that we want to serialize
            // constexpr means this value is known at compiler time ( compiler optimiziation )
            static constexpr auto marshallingVersion = 1;

            template<typename Archive, typename T>

            static void serialize(Archive &archive, T &t) {
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
        // const on Parameter arg means that the parameters ref can NOT be used to modify the Parameters object it refers to
        SerializableParameters from(const tremolo::Parameters &parameters) {
            return {
                // C++ 20 Designated Inititalizers allows you to create a new instance of a Struct and inititalize its members by name like this without referring to Struct name
                // Struct members must be inititalized in the same order they appear in the Struct
                .rate = parameters.rate.get(),
                .bypass = parameters.bypass.get(),
                // getCurrentChoiceName returns the name of the currently selected item in AudioParameterChoice
                .waveform = parameters.waveform.getCurrentChoiceName(),
            };
        }

    } // end anonymous namespace



    // Takes a reference to our Parameters object and an empty juce MemoryOutputStream that we want to write to
    // OutputStram is the parent base class that MemoryOutputStream inherits from
    // Note that we don't preface this method name with static here, we must only do that in the JsonSerializer.h file
    void JsonSerializer::serialize(const Parameters &parameters, juce::OutputStream &output) {

        // Here the from method create a new instance of a SerializableParameters Struct and copies the Parameters object values into it
        const auto serializableParameters = from(parameters);

        // juce ToVar::convert tries to serialize the SerializableParameters object to a juce Var variable using the serialize method in the SerializableParameters object
        // juce ToVar::convert requires thaat the SerializableParameters object has a serialize method and a marhsallingVersion variable
        // ToVar::convert returns an std::optional<var> ( meaning it may not be able to convert it so you have to check for not null )
        const auto varObj = juce::ToVar::convert(serializableParameters);

        // The varObj may be a std::optional<var> so we need to check if it successfully converted and has a value
        // A std::optional has a has_value method to check if value is present
        if (!varObj.has_value()) {
            // varObj may not have a value if convert failed
            return;
        }

        juce::JSON::writeToStream(output, *varObj, juce::JSON::FormatOptions{}.withSpacing(juce::JSON::Spacing::multiLine).withMaxDecimalPlaces(2));
    }



    juce::Result JsonSerializer::deserialize(juce::InputStream &input,
                                             Parameters &parameters) {
        juce::ignoreUnused(input, parameters);

        // TODO: deserialize parameters from the JSON input stream

        return juce::Result::fail("not implemented");
    }
} // namespace tremolo
