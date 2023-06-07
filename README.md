# MidroAudioSync DAW Plugin

This plugin is meant as an alternative to the Sync File Generator tool. it will generate the correct audio to send to your [Midronome](https://www.midronome.com/) in order to synchronize using "_AudioSync_" (see [the Midronome manual](https://www.midronome.com/support)).



## Beta-test the plugin

The plugin is still under development, but you can beta-test compiled versions, more information about this on [the Midronome Forum topic](https://forum.midronome.com/viewtopic.php?t=221).
Note that the plugin can only run (for now at least) as an ARA plugin and will not run on every DAW (see the [current list of DAW supporting ARA](https://en.wikipedia.org/wiki/Audio_Random_Access#ARA_implementation))



## Compile the Code

The plugin is based around the [JUCE framework](https://juce.com/), the Projucer files are included.

To compile it, you will need:
* The JUCE framework and the Projucer - [more info](https://juce.com/download/)
* An IDE: Xcode on Mac, Visual Studio 2022 on Windows
* The ARA_SDK - [download v2.2.0](https://github.com/Celemony/ARA_SDK/releases/tag/releases%2F2.2.0), unpack it, and edit accordingly the "_ARA SDK Folder_" configuration in the "_Exporters_" in the Projucer project


Please write any questions/comments/problems on [the Midronome Forum topic](https://forum.midronome.com/viewtopic.php?t=221).




## Copyright notice

This code is free software, it is released under the terms of [GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html). You can redistribute it and/or modify it under the same terms. See the COPYING.txt file for more information.
