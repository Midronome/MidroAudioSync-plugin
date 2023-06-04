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

#include <juce_audio_processors/juce_audio_processors.h>



class MidroAudioSyncDocumentController  : public juce::ARADocumentControllerSpecialisation
{
public:
    using ARADocumentControllerSpecialisation::ARADocumentControllerSpecialisation;

    
protected:
    juce::ARAPlaybackRenderer* doCreatePlaybackRenderer() noexcept override;
    bool doRestoreObjectsFromStream (juce::ARAInputStream& input, const juce::ARARestoreObjectsFilter* filter) noexcept override;
    bool doStoreObjectsToStream (juce::ARAOutputStream& output, const juce::ARAStoreObjectsFilter* filter) noexcept override;


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidroAudioSyncDocumentController)
};
