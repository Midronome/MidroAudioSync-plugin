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

#include <JuceHeader.h>
#include "TempoMap.h"


double TempoMap::sampleRate = 44100.0;
double TempoMap::halfASampleLength = 1.0/(44100.0*2.0);


using namespace juce;


TempoMap::TempoMap (ARADocument& document)
    : _araDocument (document)
{
    if (_araDocument.getMusicalContexts().size() > 0)
        selectMusicalContext (_araDocument.getMusicalContexts().front());
    
    _araDocument.addListener (this);
}

TempoMap::~TempoMap()
{
    _araDocument.removeListener (this);
    selectMusicalContext (nullptr);
}

    
void TempoMap::didAddMusicalContextToDocument (ARADocument*, ARAMusicalContext* musicalContext)
{
    if (_selectedMusicalContext == nullptr)
        selectMusicalContext (musicalContext);
}

void TempoMap::willDestroyMusicalContext (ARAMusicalContext* musicalContext)
{
    if (_selectedMusicalContext == musicalContext)
        selectMusicalContext (nullptr);
}

void TempoMap::doUpdateMusicalContextContent (ARAMusicalContext* musicalContext, ARAContentUpdateScopes scopeFlags)
{
    if (_selectedMusicalContext != musicalContext)
        selectMusicalContext (musicalContext);
    else if (scopeFlags.affectTimeline())
        rebuildTickMap();
}

bool TempoMap::getTickAndBarLengthAtPosition(int64_t currentPos, double& tickLength, uint& barLength) { // tick length in seconds
    if (_tickMap.empty())
        return false;
    
    auto it = std::upper_bound(_tickMap.begin(), _tickMap.end(), TickMapElement(((double)currentPos) / sampleRate));
    if (it != _tickMap.begin()) // if we are already on the first element we stay, otherwise we take the one before
        it--;
    
    tickLength = it->tickLength;
    barLength = it->barLength;
    
    return true;
}

int64_t TempoMap::getNextTickPositionInSamples(int64_t currentPos, bool& lastTickRightBeforeABar)
{
    if (_tickMap.empty())
        return 0;
    
    double currentPosInTime = ((double)currentPos) / sampleRate;
    currentPosInTime -= _delay; // negative delay = currentPosInTime moves positively = tempo map shifted negatively (to the left)
    
    auto it = std::upper_bound(_tickMap.begin(), _tickMap.end(), TickMapElement(currentPosInTime));
    
    if (it != _tickMap.begin()) // if we are already on the first element we stay, otherwise we take the one before
        it--;
    
    
    double tickPos = it->startPosition;
    uint tickIdx = it->tickOffset;
    
    if (sampleScaleLessThan(currentPosInTime, tickPos)) { // this could happen with a positive delay or when doing pre-roll (currentPos < 0)
        while (sampleScaleLessThan(currentPosInTime, tickPos - it->tickLength)) {
            tickPos -= it->tickLength; // we go backwards to send the signal during pre-roll or before a positive delay
            if (tickIdx == 0)
                tickIdx = it->barLength-1;
            else
                tickIdx--;
        }
    }
    else {
        while (sampleScaleLessThan(tickPos, currentPosInTime)) {
            tickPos += it->tickLength;
            tickIdx++;
            if (tickIdx == it->barLength)
                tickIdx = 0;
        }
    }
    
    lastTickRightBeforeABar = (tickIdx == (it->barLength-1));
    
    return static_cast<int64_t>(round((tickPos + _delay) * sampleRate)); // since the tempo map has not been shifted, we add the delay at the end
}




void TempoMap::selectMusicalContext (ARAMusicalContext* newSelectedMusicalContext)
{
    if (auto oldContext = std::exchange (_selectedMusicalContext, newSelectedMusicalContext);
        oldContext != _selectedMusicalContext)
    {
        if (oldContext != nullptr)
            oldContext->removeListener (this);

        if (_selectedMusicalContext != nullptr)
            _selectedMusicalContext->addListener (this);
    }
    
    rebuildTickMap();
}



void TempoMap::rebuildTickMap()
{
    if (_selectedMusicalContext != nullptr)
    {
        // see documentation: ARA_SDK/ARA_Library/html_docs/group___model___timeline.html
        const ARA::PlugIn::HostContentReader<ARA::kARAContentTypeTempoEntries> tempoReader (_selectedMusicalContext);
        const ARA::PlugIn::HostContentReader<ARA::kARAContentTypeBarSignatures> barSigReader (_selectedMusicalContext);
        
        if (tempoReader && barSigReader && tempoReader.getEventCount() > 1 && barSigReader.getEventCount() > 0)
        {
            
#ifdef DEBUG
            std::stringstream stream;
            stream.precision(6);
            stream << std::fixed << "\n\n";
            for (ARA::ARAInt32 i = 0 ; i < tempoReader.getEventCount() ; i++)
                stream  << "tempo:       time=" << tempoReader[i].timePosition << "     quarter=" << tempoReader[i].quarterPosition << "\n";
            stream << "\n\n";
            
            for (ARA::ARAInt32 i = 0 ; i < barSigReader.getEventCount() ; i++)
                stream  << "barSig:   quarter=" << barSigReader[i].position << "      " << barSigReader[i].numerator << "/" << barSigReader[i].denominator << "\n";
                            
            std::cout << "\n--------------------------------\n------ ORG DATA: -----\n" << stream.str() << "\n\n";
#endif
            
            
            
            /* -------- STEP 1 ---------
             * we build a temporary vector of time signature changes containing {quarterPosition, barLength (in ticks)}
             * we also check they all are on a bar -> if not we "quantize" them
             */
            struct TimeSigChange { uint quarterPosition; uint barLength; };
            std::vector<TimeSigChange> timeSigChanges;
            uint previousQuartersPerBar = 0;
            uint previousQuarterPos = 0;
            for (ARA::ARAInt32 i = 0 ; i < barSigReader.getEventCount() ; i++) {
                uint quartersPerBar = (4 * barSigReader[i].numerator) / barSigReader[i].denominator;
                uint quarterPos = static_cast<uint>(round(barSigReader[i].position));
                
                if (quartersPerBar == 0)
                    quartersPerBar = 1; // in case of very tiny time signatures like 1/8 or 1/16, etc => we change it to 1/4
                
                if (previousQuartersPerBar != 0) {
                    uint remainder = (quarterPos - previousQuarterPos) % previousQuartersPerBar;
                    if (remainder != 0)
                        quarterPos += previousQuartersPerBar - remainder; // we quantize to the next bar
                }
                
                timeSigChanges.push_back({quarterPos, 24 * quartersPerBar});
                
                previousQuartersPerBar = quartersPerBar;
                previousQuarterPos = quarterPos;
            }
            
            
            
            /* -------- STEP 2 ---------
             * we build another temporary array of the tempo changes, also adapting them
             *     -> if a tempo change is between 2 ticks, it needs to be made into 2 tempo changes so they are on the ticks
             */
            struct TempoChange { double timePosition; double tickLength; };
            std::vector<TempoChange> tempoChanges;
            double nextPosition = tempoReader[0].timePosition;
            for (ARA::ARAInt32 i = 0 ; i < tempoReader.getEventCount()-1 ; i++) {
                
                /*
                 * Math:
                 *   diff(xxxPos)        = tempoEntries[i+1].xxxPosition - tempoEntries[i].xxxPosition
                 *   quartersPerSecond   = diff(quarterPos) / diff(timePos)
                 *   quarterLength       = diff(timePos) / diff(quarterPos) = 1/quartersPerSecond
                 *   tickLength          = quarterLength / 24
                 *   quarterPos(timePos) = (timePos - timePosBeginning)       / quarterLength + quarterPosBeginning
                 *   timePos(quarterPos) = (quarterPos - quarterPosBeginning) * quarterLength + timePosBeginning
                 */
                
                
                double tickLength = ((tempoReader[i+1].timePosition - tempoReader[i].timePosition)
                                        / (tempoReader[i+1].quarterPosition - tempoReader[i].quarterPosition))
                                        / 24.0;
                
                double currentPosition = nextPosition;
                
                tempoChanges.push_back({currentPosition, tickLength});
                
                if (i == tempoReader.getEventCount()-2) // if i+1 is the last tempoEntry, then we do not care about nextPosition, we're done
                    break;
                
                nextPosition = tempoReader[i+1].timePosition;
                
                double lastTickPos = currentPosition;
                while (sampleScaleLessThan(lastTickPos, nextPosition))
                    lastTickPos += tickLength;
                
                // Testing in Studio One 4, it seems all tempo changes are at least 200ms apart, so we never have multiple tempo changes between 2 ticks (83ms at 30bpm)
                // maybe other DAWs can make more tempo changes, then the code below would not work...
                if (!sampleScaleEquals(lastTickPos, nextPosition)) {
                    // that means the tempo change does not fall "on" a tick, so we make this into 2 tempo changes, both on ticks
                    
                    // this one tick tempo change will have a tickLength of partially this tempo and partially the next tempo
                    double nextTickLength = ((tempoReader[i+2].timePosition - tempoReader[i+1].timePosition)
                                            / (tempoReader[i+2].quarterPosition - tempoReader[i+1].quarterPosition))
                                            / 24.0;
                    double percentOfNextTickLength = (lastTickPos - nextPosition) / tickLength;
                    double newTickLength = (nextTickLength * percentOfNextTickLength) + (tickLength * (1 - percentOfNextTickLength));
                    
                    tempoChanges.push_back({lastTickPos-tickLength, newTickLength});
                    
                    nextPosition = lastTickPos-tickLength+newTickLength;
                }
            }
            
            
            
            /* -------- STEP 3 ---------
             * we now have:
             *      -> a list of tempo changes which all are on a tick
             *      -> a list of time signature changes which are all on a bar
             * so we can finally build our _tickMap
             */
            
            _tickMap.clear();
            int timeSigChangeIdx = 0;
            double tickPos = 0; // we assume tempoChanges[0].timePosition = 0
            uint tickIdx = 0; // same
            uint tickOffset = 0;
            
            for (int i = 0 ; i < tempoChanges.size() ; i++) {
                
                double nextTempoChangePos = tempoChanges[i].timePosition;
                
                // if this is not the first tempo change, we check if there were any time signature changes since the last tempo change
                if (!_tickMap.empty()) {
                    TickMapElement lastElt = _tickMap.back();
                    
                    while (sampleScaleLessThan(tickPos, nextTempoChangePos)) {
                        if (timeSigChangeIdx+1 < timeSigChanges.size()) {
                            while (sampleScaleLessThan(tickPos, nextTempoChangePos) && tickIdx < (timeSigChanges[timeSigChangeIdx+1].quarterPosition)*24) {
                                tickPos += lastElt.tickLength;
                                tickIdx++;
                                tickOffset++;
                                if (tickOffset == lastElt.barLength)
                                    tickOffset = 0;
                            }
                            
                            if (sampleScaleLessThan(tickPos, nextTempoChangePos)) {
                                // this means we have tickIdx == 24*timeSigChanges[timeSigChangeIdx+1].quarterPosition
                                // => we add a change for the new time signature here
                                
                                timeSigChangeIdx++;
                                
                                TickMapElement newElt;
                                newElt.startPosition = tickPos;
                                newElt.tickLength = lastElt.tickLength;
                                newElt.tickOffset = 0; // = tickIdx
                                newElt.barLength = timeSigChanges[timeSigChangeIdx].barLength;
                                
                                _tickMap.push_back(newElt);
                                lastElt = _tickMap.back();
                            }
                        }
                        else { // if we are here it means there are no more time sig changes, we loop to update tickPos and tickOffset
                            tickPos += lastElt.tickLength;
                            tickIdx++;
                            tickOffset++;
                            if (tickOffset == lastElt.barLength)
                                tickOffset = 0;
                        }
                    }
                }
                
                
                TickMapElement elt;
                elt.startPosition = nextTempoChangePos; // should be = tickPos at this point
                elt.tickLength = tempoChanges[i].tickLength;
                elt.tickOffset = tickOffset;
                
                // if both conditions: (microPrecisionLessThan(tickPos, nextTempoChangePos) && tickIdx < (timeSigChanges[timeSigChangeIdx+1].quarterPosition)*24)
                // in the loop above become false at the same time, then it means we have a time signature change precisely on this tempo change
                if (timeSigChangeIdx+1 < timeSigChanges.size() && tickIdx == (timeSigChanges[timeSigChangeIdx+1].quarterPosition)*24)
                    timeSigChangeIdx++;
                
                elt.barLength = timeSigChanges[timeSigChangeIdx].barLength;
                
                _tickMap.push_back(elt);
            }
            
            
            
            /* -------- STEP 4 ---------
             * in case there are time signature changes after the last tempo change, we add them
             */
            while (timeSigChangeIdx+1 < timeSigChanges.size()) {
                TickMapElement lastElt = _tickMap.back(); // at this point we know there is at least one element in _tickMap
                
                while (tickIdx < (timeSigChanges[timeSigChangeIdx+1].quarterPosition)*24) {
                    tickPos += lastElt.tickLength;
                    tickIdx++;
                }
                
                // this means we have tickIdx*24 == timeSigChanges[timeSigChangeIdx+1].quarterPosition
                // => we add a change for the new time signature here
                
                timeSigChangeIdx++;
                
                TickMapElement newElt;
                newElt.startPosition = tickPos;
                newElt.tickLength = lastElt.tickLength;
                newElt.tickOffset = 0; // = tickOffset (if it had been updated)
                newElt.barLength = timeSigChanges[timeSigChangeIdx].barLength;
                
                _tickMap.push_back(newElt);
                lastElt = _tickMap.back();
            }
            
            
            
            //_tickMap.push_back(TickMapElement(0.0, 0.020, 24*4));  // 125bpm 4/4
            //_tickMap.push_back(TickMapElement(7.68, 0.010, 24*3)); // after 4 bars, 250bpm 3/4
            
            
#ifdef DEBUG
            stream.str("");
            for (auto i : _tickMap)
                stream  << "@" << i.startPosition << ":     tickLen=" << i.tickLength << "     barLen="
                        << i.barLength << "     tickOffset=" << i.tickOffset << "\n";
            
            stream << "\n";
            
            for (auto i : _tickMap)
                stream << "@" << i.startPosition << ":   " << (1.0/(i.tickLength * 24.0)) * 60.0 << " BPM (" << i.barLength/24 << "/4)\n";

            std::cout << "------ TickMap: -----\n\n" << stream.str() << "\n";
#endif
            
        }
    }
}


 
