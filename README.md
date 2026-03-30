# MultiBandEQ

A multi-band equalizer audio plugin built with [JUCE](https://juce.com/). Available as a Standalone application, AU, and VST3.

## Requirements

- CMake 3.23 or higher
- Xcode (macOS) or a compatible C++ compiler
- macOS 10.13+ (x86_64 and arm64 supported)

## Build Instructions

Clone the repository with submodules:

```bash
git clone --recurse-submodules git@github.com:GiorgosChr/MultiBandEQ.git
cd MultiBandEQ
```

Configure and build:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cd build && make -j
```

For a release build:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cd build && make -j
```

## Running

**Standalone:**

```bash
./build/MultiBandEQ_artefacts/Debug/Standalone/MultiBandEQ.app/Contents/MacOS/MultiBandEQ
```

**AU / VST3:**

After a successful build, the plugins are automatically copied to:
- AU: `~/Library/Audio/Plug-Ins/Components/`
- VST3: `~/Library/Audio/Plug-Ins/VST3/`
