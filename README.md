# AtFirstJuice

A professional, polyphonic analog-modeled software synthesizer built in C++ using the [JUCE Framework](https://juce.com/) and CMake.

## Features

- **Analog Waveform Selection**: Snap between Sine, Saw, and Triangle oscillators natively.
- **Classic Ladder Filter**: Integrated a resonant Low-Pass filter modeled after legendary analog circuits.
- **Polyphonic ADSR**: Full envelope control (Attack, Decay, Sustain, Release) across 8 simultaneous voices.
- **LFO Modulation**: High-speed Low Frequency Oscillator wired directly into the filter cutoff for dynamic sweeps.
- **Hyperbolic Distortion**: Warm, analog-style saturation utilizing a hyperbolic tangent waveshaper.
- **Hardware Aesthetic**: Unified mixing-fader interface with an integrated on-screen MIDI keyboard.
- **Thread-safe Processing**: Lock-free parameter fetches utilizing atomic operations directly mapped via `juce::AudioProcessorValueTreeState`. Strict adherence to real-time audio thread safety principles.

## Architecture & Code Map

* `CMakeLists.txt` - Project structure configuration using `FetchContent`.
* `PluginProcessor.h/cpp` - The audio core logic. Defines the DSP pipeline, voices, and state management.
* `PluginEditor.h/cpp` - The visual interface and parameter binding.

## How To Build

Make sure you have CMake and Ninja installed. Then navigate to the project directory in your terminal and run:

```bash
# Provide the Release build instructions to CMake:
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Execute the compiler target globally:
cmake --build build
```

The resulting plugins will be placed in `build/AtFirstJuice_artefacts/Release/`.

## How To Run Without A DAW

Run the interactive standalone preview directly:
```bash
open build/AtFirstJuice_artefacts/Release/Standalone/AtFirstJuice.app
```

## Author

**Gary A. Newsome**
**(c) 2026**
