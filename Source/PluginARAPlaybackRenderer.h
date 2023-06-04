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


#pragma once

#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "TempoMap.h"




//==============================================================================
/**
*/
class MidroAudioSyncPlaybackRenderer  : public juce::ARAPlaybackRenderer
{
public:
    //==============================================================================
    explicit MidroAudioSyncPlaybackRenderer(ARA::PlugIn::DocumentController* documentController) noexcept;
    
    ~MidroAudioSyncPlaybackRenderer();

    //==============================================================================
    void prepareToPlay (double sampleRate,
                        int maximumSamplesPerBlock,
                        int numChannels,
                        juce::AudioProcessor::ProcessingPrecision,
                        AlwaysNonRealtime alwaysNonRealtime) override;
    

    //==============================================================================
    bool processBlock (juce::AudioBuffer<float>& buffer,
                       juce::AudioProcessor::Realtime realtime,
                       const juce::AudioPlayHead::PositionInfo& positionInfo) noexcept override;
    
    
    
    void setTempoMapDelay(double delay) { tempoMap->setDelay(delay); }
    double getTempoMapDelay() { return tempoMap->getDelay(); }
    
    void setSendSignalAlways(bool val) { sendSignalAlways = val; }
    bool getSendSignalAlways() { return sendSignalAlways; }
    
private:
        
    //==============================================================================
    double sampleRate = 44100.0;
    int maximumSamplesPerBlock = 4096;
    int numChannels = 1;
    bool useBufferedAudioSourceReader = true;
    
    bool sendSignalAlways = false;
    
    std::unique_ptr<TempoMap> tempoMap;
    
    float* outputData = NULL;
    
    
    uint missingEndOfLowTick = 0;
    uint missingEndOfHighTick = 0;
    
    uint samplesSinceLastTick = 0; // to avoid sending two ticks "too close" to each other
    uint minSamplesSinceLastTick = 0;
    uint maxSamplesSinceLastTick = 0;
    
    uint currentTickIndex = 0;
    
    
    
    #define TICK_HEIGHT         0.35f
    #define BAR_TICK_HEIGHT     0.95f
    #define HIGH_TICK_LENGTH    26
    #define LOW_TICK_LENGTH     13


    static constexpr const float highTickSamples[HIGH_TICK_LENGTH] = {
        TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT,
        TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT,
        TICK_HEIGHT,
        TICK_HEIGHT + ((1.0f/5.0f)*(BAR_TICK_HEIGHT-TICK_HEIGHT)),
        TICK_HEIGHT + ((2.0f/5.0f)*(BAR_TICK_HEIGHT-TICK_HEIGHT)),
        TICK_HEIGHT + ((3.0f/5.0f)*(BAR_TICK_HEIGHT-TICK_HEIGHT)),
        TICK_HEIGHT + ((4.0f/5.0f)*(BAR_TICK_HEIGHT-TICK_HEIGHT)),
        BAR_TICK_HEIGHT,
        BAR_TICK_HEIGHT,
        BAR_TICK_HEIGHT,
        BAR_TICK_HEIGHT - ((1.0f/9.0f)*BAR_TICK_HEIGHT),
        BAR_TICK_HEIGHT - ((2.0f/9.0f)*BAR_TICK_HEIGHT),
        BAR_TICK_HEIGHT - ((3.0f/9.0f)*BAR_TICK_HEIGHT),
        BAR_TICK_HEIGHT - ((4.0f/9.0f)*BAR_TICK_HEIGHT),
        BAR_TICK_HEIGHT - ((5.0f/9.0f)*BAR_TICK_HEIGHT),
        BAR_TICK_HEIGHT - ((6.0f/9.0f)*BAR_TICK_HEIGHT),
        BAR_TICK_HEIGHT - ((7.0f/9.0f)*BAR_TICK_HEIGHT),
        BAR_TICK_HEIGHT - ((8.0f/9.0f)*BAR_TICK_HEIGHT)
    };

    static constexpr const float lowTickSamples[LOW_TICK_LENGTH] = {
        TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT,
        TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT, TICK_HEIGHT,
        TICK_HEIGHT,
        TICK_HEIGHT - ((1.0f/3.0f)*TICK_HEIGHT),
        TICK_HEIGHT - ((2.0f/3.0f)*TICK_HEIGHT)
    };
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidroAudioSyncPlaybackRenderer)
};
