/*
    ==============================================================================

    This file is part of the MidroAudioSync plugin, a plugin for Digital Audio
    Workstations (DAW) whose purpose is to synchronize DAWs with the Midronome
    (more info on <https://www.midronome.com/>).
 
    Copyright © 2023 - Simon Lasnier

    The MidroAudioSync plugin is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The MidroAudioSync plugin is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    the MidroAudioSync plugin. If not, see <https://www.gnu.org/licenses/>.
 
    ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginARAPlaybackRenderer.h"

using namespace juce;




MidroAudioSyncAudioProcessor::MidroAudioSyncAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

MidroAudioSyncAudioProcessor::~MidroAudioSyncAudioProcessor()
{
}


const String MidroAudioSyncAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidroAudioSyncAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MidroAudioSyncAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MidroAudioSyncAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MidroAudioSyncAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidroAudioSyncAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MidroAudioSyncAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MidroAudioSyncAudioProcessor::setCurrentProgram (int index)
{
}

const String MidroAudioSyncAudioProcessor::getProgramName (int index)
{
    return {};
}

void MidroAudioSyncAudioProcessor::changeProgramName (int index, const String& newName)
{
}


void MidroAudioSyncAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    prepareToPlayForARA (sampleRate, samplesPerBlock, getMainBusNumOutputChannels(), getProcessingPrecision());
}

void MidroAudioSyncAudioProcessor::releaseResources()
{
    releaseResourcesForARA();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidroAudioSyncAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MidroAudioSyncAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

	if (!processBlockForARA (buffer, isRealtime(), getPlayHead()))
		processBlockBypassed (buffer, midiMessages);
}


bool MidroAudioSyncAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* MidroAudioSyncAudioProcessor::createEditor()
{
    return new MidroAudioSyncAudioProcessorEditor (*this);
}


void MidroAudioSyncAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    if (!isBoundToARA())
        return;
    
    MidroAudioSyncPlaybackRenderer *renderer = dynamic_cast<MidroAudioSyncPlaybackRenderer*>(getPlaybackRenderer());
    
    if (!renderer)
        return;
    
    destData.reset();
    destData.setSize(sizeof(double)+sizeof(char));
    
    char *data = (char*)destData.getData();
    *((double*)data) = renderer->getTempoMapDelay();
    char sendSignalAlways = 0;
    if (renderer->getSendSignalAlways())
        sendSignalAlways = 1;
    *(data+sizeof(double)) = sendSignalAlways;
}

void MidroAudioSyncAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (!isBoundToARA())
        return;
    
    MidroAudioSyncPlaybackRenderer *renderer = dynamic_cast<MidroAudioSyncPlaybackRenderer*>(getPlaybackRenderer());
    
    if (!renderer)
        return;
    
    if (sizeInBytes < (sizeof(double)+sizeof(char)))
        return;
    
    double delay = *((double*)data);
    bool sendSignalAlways = false;
    if (*(((char*)data)+sizeof(double)) != 0)
        sendSignalAlways = true;
    
    renderer->setTempoMapDelay(delay);
    renderer->setSendSignalAlways(sendSignalAlways);
}


AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidroAudioSyncAudioProcessor();
}
