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


MidroAudioSyncAudioProcessorEditor::MidroAudioSyncAudioProcessorEditor (MidroAudioSyncAudioProcessor& p)
    : AudioProcessorEditor (&p),
      AudioProcessorEditorARAExtension (&p),
      audioProcessor (p)
{
    
    // ARA requires that plugin editors are resizable
    setResizable (true, false);
    setSize (400, 200);
    
    //_logoImage = ImageCache::getFromMemory(...); // TODO improve GUI and add Midronome Logo
        
    addAndMakeVisible (_delaySlider);
    _delaySlider.setRange (-200.0, 200.0);
    _delaySlider.setTextValueSuffix (" ms");
    _delaySlider.setNumDecimalPlacesToDisplay(2);
    _delaySlider.onValueChange = [this] { setDelayOnTempoMap(); };

    addAndMakeVisible (_delayLabel);
    _delayLabel.setText ("Delay", dontSendNotification);
    _delayLabel.attachToComponent (&_delaySlider, true);
    
    addAndMakeVisible(_button);
    _button.setButtonText("Only send signal when playing");
    _button.onClick = [this] { setSendSignalAlwaysFromButton(); };
    
    
    if (isARAEditorView()) {
        MidroAudioSyncPlaybackRenderer *renderer = dynamic_cast<MidroAudioSyncPlaybackRenderer*>(audioProcessor.getPlaybackRenderer());
        
        if (renderer) {
            _delaySlider.setValue(renderer->getTempoMapDelay()*1000.0, dontSendNotification);
            _button.setToggleState(!renderer->getSendSignalAlways(), dontSendNotification);
        }
    }
}

MidroAudioSyncAudioProcessorEditor::~MidroAudioSyncAudioProcessorEditor()
{
}


void MidroAudioSyncAudioProcessorEditor::setSendSignalAlwaysFromButton() {
    if (isARAEditorView()) {
        MidroAudioSyncPlaybackRenderer *renderer = dynamic_cast<MidroAudioSyncPlaybackRenderer*>(audioProcessor.getPlaybackRenderer());
        
        if (renderer)
            renderer->setSendSignalAlways(!_button.getToggleState());
    }
}


void MidroAudioSyncAudioProcessorEditor::setDelayOnTempoMap() {
    if (isARAEditorView()) {
        MidroAudioSyncPlaybackRenderer *renderer = dynamic_cast<MidroAudioSyncPlaybackRenderer*>(audioProcessor.getPlaybackRenderer());
        
        if (renderer)
            renderer->setTempoMapDelay(_delaySlider.getValue()/1000.0);
    }
}



void MidroAudioSyncAudioProcessorEditor::paint (Graphics& g)
{
	g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

	if (! isARAEditorView())
	{
		g.setColour (Colours::white);
		g.setFont (15.0f);
		g.drawFittedText ("Your DAW does not seem to support ARA,\nor this plugin has not been loaded as an ARA plugin.",
							getLocalBounds(),
							Justification::centred,
							1);
	}
}


void MidroAudioSyncAudioProcessorEditor::resized()
{
    if (isARAEditorView()) {
        auto sliderLeft = 100;
        auto width = getWidth();
        if (width > 600)
            width = 600;
        _delaySlider.setBounds (sliderLeft, 20, getWidth() - sliderLeft - 10, 20);
        _button.setBounds(sliderLeft, 60, getWidth() - sliderLeft - 10, 20);
    }
}
