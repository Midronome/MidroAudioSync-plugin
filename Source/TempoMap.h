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


class TempoMap : public juce::ARAMusicalContext::Listener,
                 public juce::ARADocument::Listener
{
public:
    TempoMap (juce::ARADocument& document);

    ~TempoMap() override;

    
    void didAddMusicalContextToDocument (juce::ARADocument*, juce::ARAMusicalContext* musicalContext) override;

    void willDestroyMusicalContext (juce::ARAMusicalContext* musicalContext) override;
    
    void doUpdateMusicalContextContent (juce::ARAMusicalContext* musicalContext, juce::ARAContentUpdateScopes scopeFlags) override;
    
    bool getTickAndBarLengthAtPosition(int64_t currentPos, double& tickLength, uint& barLength);
    
    int64_t getNextTickPositionInSamples(int64_t currentPos, bool& lastTickRightBeforeABar);
    
    
    // negative or positive delay in seconds
    void setDelay(double delay) { _delay = delay; }
    double getDelay() { return _delay; }
    
    
    static void setSampleRate(double sr) {
        TempoMap::sampleRate = sr;
        TempoMap::halfASampleLength = 1.0/(sr*2.0);
    }
    
    
    
    static double sampleRate; // initialized in TempoMap.cpp (because static)
    static double halfASampleLength;
    
    
private:
    
    void selectMusicalContext (juce::ARAMusicalContext* newSelectedMusicalContext);
    
    
    void rebuildTickMap();
    
    
    
    // Helper functions to compare double (time in seconds) with a precision of 0.5/sampleRate (e.g. about 10us for 48kHz)
    static bool sampleScaleLessThan(double a, double b) {
        return (a < b && !sampleScaleEquals(a, b));
    }
    static bool sampleScaleEquals(double a, double b) {
        return (abs(a - b) <= halfASampleLength); // if the time diff is half a sample or less they are considered equal
    }
    
    
    
    struct TickMapElement {
        double startPosition; // start position in seconds
        double tickLength; // tick length in seconds
        uint barLength; // bar length in amount of ticks
        uint tickOffset; // in case this tick is not at the beginning of a bar, amount of ticks past the beginning of the bar
        
        
        TickMapElement()
            : startPosition(0.0), tickLength(0.0), barLength(0), tickOffset(0)
        {
        }
        
        TickMapElement(double startPosition, double tickLength = 0, uint barLength = 0, uint tickOffset = 0)
            : startPosition(startPosition), tickLength(tickLength), barLength(barLength), tickOffset(tickOffset)
        {
        }
        
        friend bool operator< (const TickMapElement& elt1, const TickMapElement& elt2) {
            return sampleScaleLessThan(elt1.startPosition, elt2.startPosition);
        }
    };

    juce::ARADocument& _araDocument;
    juce::ARAMusicalContext* _selectedMusicalContext = nullptr;
    std::vector<TickMapElement> _tickMap;
    double _delay = 0.0;
};

 
