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

            // This serialise function declaration ( note spelling serialise instead of serialize ) must be included as part of juce::SerialisationTraits in order that the Struct be serialized correctly
            // Juce spells serialise instead of serialize
            static void serialise(Archive& archive, T& t) {
                // We only support a marshallingVersion of 1 right now
                if (archive.getVersion() != 1) {
                    return;
                }

                // The TREMOLO_PLUGIN_NAME var is set in CMakeLists.txt and added to target_compile_definitions, so is available to our C++ code
                std::string pluginName = TREMOLO_PLUGIN_NAME;

                // The archive says which member attributes we want to Serialize to the Var object
                archive(juce::named("pluginName", pluginName));

                archive(juce::named("modulationRateHz", t.rate), juce::named("bypass", t.bypass),
                        juce::named("modulationWaveform", t.waveform));
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

        // juce JSON::write to stream will serialize a Var object into a JSON text stream
        // This is saying we want to Dereference the varObj and write it to the output ( MemoryOutputStream )
        // Once this method finishes, we return control to PluginProcessor.cpp getStateInformation which returns the OutputStream back to the Host - the Host stores this Json text stream as a file in its project
        juce::JSON::writeToStream(output, *varObj, juce::JSON::FormatOptions{}.withSpacing(juce::JSON::Spacing::multiLine).withMaxDecimalPlaces(2));
    }



    juce::Result JsonSerializer::deserialize(juce::InputStream &input,
                                             Parameters &parameters) {

        juce::var parsedResult;
        // juce JSON::parse gets the input stream of raw bytes (Provided by the Host ) as a text string (which we know is Json ), and loads it into our parsedResult Var object
        // result is a juce::Result that just tells you whether calling JSON::parse succeeded or not
        const auto result = juce::JSON::parse(input.readEntireStreamAsString(), parsedResult);
        // juce::Result
        if (result.failed()) {
            return result;
        }

        // Now we convert the parsedResult Var object back into a SerializableParameters object
        // The juce FromVar::convert method returns an std::optional of type SerializableParameters
        const auto parsedParameters = juce::FromVar::convert<SerializableParameters>(parsedResult);
        // Since parsedParameters is an std::optional, we have to check if the convert operation succeeded or not
        if (!parsedParameters.has_value()) {
            return juce::Result::fail("Failed to parse parameters from JSON representation");
        }

        // so now we know that the SerializableParameters object is valid
        // So go ahead with getting the SerializableParameters attributes and writing them back to AudioParameterChoice, AudioParameterFloat, and AudioParameterBool objects
        // The parameters.waveform is an AudioParameterChoice object that has an array of "Sine", "Triangle"
        // So we find the index of it that matches the SerializableParameters waveform string ( e.g. "Sine", or "Triangle)
        const auto modulationWaveformIndex = parameters.waveform.choices.indexOf(parsedParameters->waveform);
        // Now we set the parameters AudioParameterChoice value to the index of the string that we have in the SerializableParameters object
        parameters.waveform = modulationWaveformIndex;

        // The arrow Dereferences parsedParameters and gets the rate
        parameters.rate = parsedParameters->rate;

        parameters.bypass = parsedParameters->bypass;

        return juce::Result::ok();
    }
} // namespace tremolo
