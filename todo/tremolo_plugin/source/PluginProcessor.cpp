
namespace tremolo {
PluginProcessor::PluginProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

const juce::String PluginProcessor::getName() const {
  return TREMOLO_PLUGIN_NAME;
}

bool PluginProcessor::acceptsMidi() const {
  return false;
}

bool PluginProcessor::producesMidi() const {
  return false;
}

bool PluginProcessor::isMidiEffect() const {
  return false;
}

double PluginProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int PluginProcessor::getNumPrograms() {
  // Some hosts don't cope very well if you tell them there are 0 programs, so
  // this should be at least 1, even if you're not really implementing programs.
  return 1;
}

int PluginProcessor::getCurrentProgram() {
  return 0;
}

void PluginProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void PluginProcessor::changeProgramName(int index,
                                        const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

// Juce passes in the expectedMaxFramesPerBlock, which is the Maximum number of samples you can expect to get in the AudioBuffer, but not necessarily the actual number of samples in the AudioBuffer
void PluginProcessor::prepareToPlay(double sampleRate,
                                    int expectedMaxFramesPerBlock) {
  // Use this method as the place to do any pre-playback
  // initialization that you need, e.g., allocate memory.

  tremolo.prepare(sampleRate, expectedMaxFramesPerBlock);

  // You can use Aggregate Inititalizion for a Struct in curly braces - you don't have to explicilty create the ProcessSpec Struct instance first before setting its values
  bypassTransitionSmoother.prepare( {
    .sampleRate = sampleRate,
    // cast expectedMaxFramesPerBlock from an int to the juce::unit32 needed by ProcessSpec
    .maximumBlockSize = static_cast<juce::uint32>(expectedMaxFramesPerBlock),
    // Here we use juce jmax to select whichever is bigger, intput channels or output channels ( but typically they are the same number )
    .numChannels = static_cast<juce::uint32>(
      juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels())),
  });

}


void PluginProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
  tremolo.reset();
  bypassTransitionSmoother.reset();
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }

  // This checks if the input layout matches the output layout
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) {
    return false;
  }

  return true;
}


// This is always getting called repeatedly with a AudioBuffer of 512 samples even if Host is not Sending audio
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;
  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();



  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (const auto channelToClear : std::views::iota(totalNumInputChannels, totalNumOutputChannels)) {
    buffer.clear(channelToClear, 0, buffer.getNumSamples());
  }

  //DBG(" num of samples in Audiobuffer is " << buffer.getNumSamples());

  // PluginProcessor.h has access to both the Tremelo.h class and the Parameters Struct
  // Here we take the latest Parameters val for modulation rate and pass it to tremolo.setModulationRate which actually updates the juce Oscillator value
  // The Parameter rate is an AudioParameterFloat& reference.  The juce AudioParameterFloat class has a get() method that returns the current selected value of it in the GUI
   tremolo.setModulationRate(parameters.rate.get());
   // parameters.waveform is the ref to the AudioParameterChoice object, which is a choice of Sine or Triangle LFO on the GUI, so we get the Index of what the User selected
   // We are able to cast the AudioParameterChoice index value to our LfoWaveform Enum since indexes match ( we ordered them as Sine, Triangle in both )
   tremolo.setLfoWaveform(static_cast<Tremolo::LfoWaveform>(parameters.waveform.getIndex()));


  // We pass the current Bypass param value on the GUI to the bypassTransitionSmoother setBypass method
   bypassTransitionSmoother.setBypass(parameters.bypass.get());

   // If the Bypass On/Off button in GUI is set to On and Bypass transitioning is done, then we short circuit this function and don't do our Tremolo processing
   if (parameters.bypass.get() && !bypassTransitionSmoother.isTransitioning()) {
     return;
   }

  bypassTransitionSmoother.setDryBuffer(buffer);

  // apply  tremolo
  tremolo.process(buffer);

  bypassTransitionSmoother.mixToWetBuffer(buffer);
}

bool PluginProcessor::hasEditor() const {
  return false;
}

// This function will be called to create an instance of the editor
juce::AudioProcessorEditor* PluginProcessor::createEditor() {
  return nullptr;
}


// Host calls this when it wants the Plugin to Save its State, usually it's when the user Saves the DAW project or closes the project, but varies by DAW host
// Host passes a reference to an empty juce MemoryBlock, and we are supposed to populate it with our Plugin State ( as a raw byte array )
// After we populate the MemoryBlock, the DAW host takes the populated MemoryBlock destData and stores the data in its DAW project file ( we don't store it in our Plugin )
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {

  // Creates an OutputStream for writing into the destData MemoryBlock
  // The MemoryOutputStream at this points as the empty MemoryBlock, the "true" means we will Append data to the existing MemoryBlock ( instead of overwriting what's in there )
  juce::MemoryOutputStream memoryOutputStream(destData, true);

  // We pass a reference to the Parameters Struct and the MemoryOutputStream we created ( which is an concrete impl of the abstract juce OutputStream )
  // Note that serialize is a static method so we can directoly instead of having to create an instance of JsonSerializer first
  JsonSerializer::serialize(parameters, memoryOutputStream);
}


  // Host calls this when it wants our Plugin to load and restore its State
  // Host passes in a pointer to a block of raw Bytes in memory that is our Json State parameters
  // void* is a pointer to a block of raw Bytes in Memory, it doesn't know what type of data those bytes are ( doesn't know if it's float, int, or other type )
  // But we know that this block represents the JSON state info of our State params
  // Since it's const, we can't modify the memory it points to
  // sizeInBytes is the size of the bytes in Memory that the data  points to
void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {

  juce::MemoryInputStream memoryInputStream{ data, static_cast<size_t>(sizeInBytes), false };

  // We pass the MemoryInputStream containing bytes representing the Json State parameters and get back a juce::Result telling us whether Deserialization succeeded
  const auto result = JsonSerializer::deserialize(memoryInputStream , parameters);

  if (result.failed()) {
    DBG(result.getErrorMessage());
  }

  bypassTransitionSmoother.setBypassForced(parameters.bypass.get());

}

// getBypassParameter is a method in the juce AudioProcessor class that we declared in PluginProcessor.h,
// here we are overriding it to tell JUCE that we are implementing our own Bypass param and don't need the JUCE provided one
// This returns our AudioProcessorBool that we created to control the Bypass state
juce::AudioProcessorParameter* PluginProcessor::getBypassParameter() const {
  // We return a reference to our Bypass AudioParameterBool object that is in the AudioProcessor.  It has the current state of the On/Off Bypass button in the GUI
  return &parameters.bypass;
}


}  // namespace tremolo




// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new tremolo::PluginProcessor();
}
