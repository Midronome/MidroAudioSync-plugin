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

#include "PluginARAPlaybackRenderer.h"

using namespace juce;


MidroAudioSyncPlaybackRenderer::MidroAudioSyncPlaybackRenderer(ARA::PlugIn::DocumentController* documentController) noexcept
    : ARAPlaybackRenderer::ARAPlaybackRenderer(documentController)
{
    tempoMap = std::make_unique<TempoMap> (*documentController->getDocument<ARADocument>());
}




//==============================================================================
void MidroAudioSyncPlaybackRenderer::prepareToPlay (double sampleRateIn, int maximumSamplesPerBlockIn, int numChannelsIn, AudioProcessor::ProcessingPrecision, AlwaysNonRealtime alwaysNonRealtime)
{
    numChannels = numChannelsIn;
    sampleRate = sampleRateIn;
    maximumSamplesPerBlock = (unsigned int)maximumSamplesPerBlockIn;
    useBufferedAudioSourceReader = alwaysNonRealtime == AlwaysNonRealtime::no;
    
    if (outputData != NULL)
        delete[] outputData;
    
    outputData = new float[maximumSamplesPerBlock];
    for (unsigned int i = 0 ; i < maximumSamplesPerBlock ; i++)
        outputData[i] = 0.0f;
    
    TempoMap::setSampleRate(sampleRate);
    
    missingEndOfLowTick = 0;
    missingEndOfHighTick = 0;
    
    // 0.006242976651267 seconds = tick length of a tempo of 400.45 BPM
    minSamplesSinceLastTick = static_cast<unsigned int>(ceil(0.006242976651267 * sampleRate)); // ceil for a tempo < 400.45
    
    // 0.084602368866328 seconds = tick length of a tempo of 29.55 BPM
    maxSamplesSinceLastTick = static_cast<unsigned int>(floor(0.084602368866328 * sampleRate)); // floor for a tempo > 29.55
    samplesSinceLastTick = minSamplesSinceLastTick;
    
    currentTickIndex = 0;
}


MidroAudioSyncPlaybackRenderer::~MidroAudioSyncPlaybackRenderer()
{
    if (outputData != NULL)
        delete[] outputData;
}


//==============================================================================
bool MidroAudioSyncPlaybackRenderer::processBlock (AudioBuffer<float>& buffer,
                                                       AudioProcessor::Realtime realtime,
                                                       const AudioPlayHead::PositionInfo& positionInfo) noexcept
{
    const auto numSamples = (unsigned int)(buffer.getNumSamples());
    jassert (numSamples <= maximumSamplesPerBlock);
    jassert (numChannels == buffer.getNumChannels());
    jassert (realtime == AudioProcessor::Realtime::no || useBufferedAudioSourceReader);
    const auto startTimeInSamples = positionInfo.getTimeInSamples().orFallback (0);
    const auto isPlaying = positionInfo.getIsPlaying();

    bool success = true;
    
    if (!isPlaying && !sendSignalAlways) {
        for (unsigned int i = 0 ; i < numSamples ; i++)
            outputData[i] = 0.0f;
    }
    
    else {
        unsigned int i = 0;
        int64_t nextTick = 0;
        bool lastTickRightBeforeABar = false;
        
        double tickLength = 0.0; // in seconds
        unsigned int barLength = 0;
        tempoMap->getTickAndBarLengthAtPosition(startTimeInSamples, tickLength, barLength);

        
        double nextTickInSeconds = -(((double)(samplesSinceLastTick))/sampleRate); // we start at "minus samplesSinceLastTick" position
        
        
        while (i < numSamples && missingEndOfLowTick > 0) {
            outputData[i++] = MidroAudioSyncPlaybackRenderer::lowTickSamples[LOW_TICK_LENGTH-missingEndOfLowTick];
            missingEndOfLowTick--;
        }
        while (i < numSamples && missingEndOfHighTick > 0) {
            outputData[i++] = MidroAudioSyncPlaybackRenderer::highTickSamples[HIGH_TICK_LENGTH-missingEndOfHighTick];
            missingEndOfHighTick--;
        }
    
    
        while (i < numSamples) {
            
            if (!isPlaying) {
                nextTickInSeconds += tickLength;
                nextTick = static_cast<int64_t>(nextTickInSeconds*sampleRate);
                
                lastTickRightBeforeABar = false;
                if (currentTickIndex >= barLength-1)
                    lastTickRightBeforeABar = true;
            }
            else {
                nextTick = tempoMap->getNextTickPositionInSamples(startTimeInSamples + i, lastTickRightBeforeABar) - startTimeInSamples;
            }
            
            // that last conditon (maxSamplesSinceLastTick) will make sure we always send ticks to a tempo >= 29.55bpm to maintain sync at all times
            while (i < numSamples && i < nextTick && samplesSinceLastTick < maxSamplesSinceLastTick) {
                outputData[i++] = 0.0f;
                samplesSinceLastTick++;
            }
            
            if (i < numSamples) {
                if (samplesSinceLastTick < minSamplesSinceLastTick) { // sending this tick would mean tempo > 400.55bpm => losing sync on the Midronome
                    outputData[i++] = 0.0f; // we ignore this tick and next call to getNextTickPositionInSamples() will send the next tick
                    samplesSinceLastTick++;
                }
                else {
                    currentTickIndex++;
                    if (lastTickRightBeforeABar) {
                        currentTickIndex = 0;
                        int length = HIGH_TICK_LENGTH;
                        samplesSinceLastTick = HIGH_TICK_LENGTH; // that way we do not need to increase it in the missingEndOfXXXTick-- above
                        if (i + HIGH_TICK_LENGTH > numSamples) {
                            missingEndOfHighTick = i + HIGH_TICK_LENGTH - numSamples;
                            length -= missingEndOfHighTick;
                        }
                        
                        for (int j = 0 ; j < length ; j++)
                            outputData[i++] = MidroAudioSyncPlaybackRenderer::highTickSamples[j];
                    }
                    else {
                        int length = LOW_TICK_LENGTH;
                        samplesSinceLastTick = LOW_TICK_LENGTH; // same
                        if (i + LOW_TICK_LENGTH > numSamples) {
                            missingEndOfLowTick = i + LOW_TICK_LENGTH - numSamples;
                            length -= missingEndOfLowTick;
                        }
                        
                        for (int j = 0 ; j < length ; j++)
                            outputData[i++] = MidroAudioSyncPlaybackRenderer::lowTickSamples[j];
                    }
                }
            }
        }
    }
    
    
    for (int c = 0; c < numChannels; c++)
    {
        auto* channelData = buffer.getWritePointer (c);
        for (unsigned int i = 0; i < numSamples; ++i)
            channelData[i] = outputData[i];
    }

    return success;
}



