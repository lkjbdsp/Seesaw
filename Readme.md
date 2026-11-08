# lkjb SeeSaw

## Build steps:

- get JUCE: https://github.com/juce-framework/JUCE.git
- build Projucer in JUCE/extras/Projucer/Builds
- Open lkjb_seesaw.jucer in Projucer
- select exporters, save and open IDE
- build in IDE

For building VST2 you need to specify the VST2 SDK in Projucer

The project is based on JUCE and was compiled with JUCE 9.

The SimdBandfiler class used for the BandAnalyser uses Intel intrinsics. This might be a problem when compiling for ARM like Apple Silicon. Libraries like https://github.com/DLTcollab/sse2neon could help but this hasn't been tested.

## 3rd Party Libraries used with QRange

As mentione JUCE is used as plugin framework.

Some filters are inspired from https://github.com/vinniefalco/DSPFilters with modifications
