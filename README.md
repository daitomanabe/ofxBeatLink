# ofxBeatLink

openFrameworks addon for Pioneer DJ Link protocol. Wraps [beat-link-cpp](https://github.com/daitomanabe/beat-link-cpp) library.

## Features

- Detect CDJ, XDJ, DJM devices on the network
- Receive beat information (BPM, beat position, pitch)
- Thread-safe event delivery to main thread
- Simple openFrameworks-style API with ofEvent

## Requirements

- openFrameworks 0.12.0+
- C++20 compiler
- Network connection to DJ Link devices (same subnet)

## Installation

1. Clone to your openFrameworks addons folder:
```bash
cd of_v0.12.x/addons
git clone --recursive https://github.com/daitomanabe/ofxBeatLink.git
```

2. If you cloned without `--recursive`, initialize submodules:
```bash
cd ofxBeatLink
git submodule update --init --recursive
```

## Usage

```cpp
#include "ofxBeatLink.h"

class ofApp : public ofBaseApp {
    ofxBeatLink beatLink;

    void setup() {
        // Register event listeners
        ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
        ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);

        // Start listening
        beatLink.start();
    }

    void update() {
        // Required: process events on main thread
        beatLink.update();
    }

    void exit() {
        ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
        ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
        beatLink.stop();
    }

    void onBeat(ofxBeatLinkBeat& beat) {
        cout << "BPM: " << beat.bpm
             << " Beat: " << beat.beatWithinBar << "/4" << endl;
    }

    void onDeviceFound(ofxBeatLinkDevice& device) {
        cout << "Found: " << device.deviceName << endl;
    }
};
```

## API Reference

### ofxBeatLink

Main class for DJ Link communication.

| Method | Description |
|--------|-------------|
| `start()` | Start listening on ports 50000/50001 |
| `stop()` | Stop listening |
| `update()` | Call in ofApp::update() to dispatch events |
| `isRunning()` | Check if listening |
| `getCurrentDevices()` | Get list of detected devices |
| `getLatestBeat(int deviceNumber)` | Get latest beat from specific device |
| `getLatestBeat()` | Get latest beat from any device |

### Events

| Event | Type | Description |
|-------|------|-------------|
| `beatEvent` | `ofxBeatLinkBeat` | Fired on each beat |
| `deviceFoundEvent` | `ofxBeatLinkDevice` | Fired when device appears |
| `deviceLostEvent` | `ofxBeatLinkDevice` | Fired when device disappears |

### ofxBeatLinkBeat

| Field | Type | Description |
|-------|------|-------------|
| `deviceNumber` | int | Device number (1-4) |
| `deviceName` | string | Device name (e.g., "CDJ-3000") |
| `bpm` | double | Effective BPM (with pitch) |
| `trackBpm` | double | Original track BPM |
| `beatWithinBar` | int | Position in bar (1-4, 1=downbeat) |
| `pitchPercent` | double | Pitch adjustment % |
| `nextBeatMs` | int64_t | ms until next beat |
| `nextBarMs` | int64_t | ms until next bar |

### ofxBeatLinkDevice

| Field | Type | Description |
|-------|------|-------------|
| `deviceNumber` | int | Device number |
| `deviceName` | string | Device name |
| `ipAddress` | string | IP address |
| `macAddress` | string | MAC address |
| `isOpusQuad` | bool | Is Opus Quad |
| `isXdjAz` | bool | Is XDJ-AZ |

## Troubleshooting

### Port already in use

Close rekordbox or other DJ Link applications using ports 50000-50002.

Check with:
```bash
lsof -i :50000
```

### No devices found

- Ensure PC and DJ equipment are on the same network/subnet
- Check firewall settings for ports 50000-50002
- Verify DJ Link is enabled on the equipment

## License

EPL-2.0 (same as beat-link-cpp)

## Credits

- [beat-link-cpp](https://github.com/daitomanabe/beat-link-cpp) - C++ implementation
- [Deep Symmetry Beat Link](https://github.com/Deep-Symmetry/beat-link) - Original Java library
