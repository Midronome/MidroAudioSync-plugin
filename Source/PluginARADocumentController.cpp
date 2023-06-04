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

#include "PluginARADocumentController.h"
#include "PluginARAPlaybackRenderer.h"

using namespace juce;


ARAPlaybackRenderer* MidroAudioSyncDocumentController::doCreatePlaybackRenderer() noexcept
{
    return new MidroAudioSyncPlaybackRenderer (getDocumentController());
}

bool MidroAudioSyncDocumentController::doRestoreObjectsFromStream (ARAInputStream& input, const ARARestoreObjectsFilter* filter) noexcept
{
    return true;
}

bool MidroAudioSyncDocumentController::doStoreObjectsToStream (ARAOutputStream& output, const ARAStoreObjectsFilter* filter) noexcept
{
    return true;
}

// This creates the static ARAFactory instances for the plugin.
const ARA::ARAFactory* JUCE_CALLTYPE createARAFactory()
{
    return ARADocumentControllerSpecialisation::createARAFactory<MidroAudioSyncDocumentController>();
}
