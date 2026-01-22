# ofxBeatLink

openFrameworks addon for Pioneer DJ Link protocol. Wraps [beat-link-cpp](https://github.com/daitomanabe/beat-link-cpp) library.

## Features

- Detect CDJ, XDJ, DJM devices on the network
- Receive beat information (BPM, beat position, pitch)
- VirtualCdj mode for detailed device status (playing, master, sync, on-air)
- Thread-safe event delivery to main thread
- Simple openFrameworks-style API with ofEvent
- rekordbox database parsing via [crate-digger-cpp](https://github.com/daitomanabe/crate-digger-cpp)
  - Parse export.pdb database files
  - Read cue points, beat grids, waveforms, song structure from ANLZ files
## Requirements

- openFrameworks 0.12.0+
- C++17 compiler
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

### Basic Mode (Passive Listener)

```cpp
#include "ofxBeatLink.h"

class ofApp : public ofBaseApp {
    ofxBeatLink beatLink;

    void setup() {
        ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
        ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
        beatLink.start();
    }

    void update() {
        beatLink.update();  // Required: dispatch events on main thread
    }

    void exit() {
        ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
        ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
        beatLink.stop();
    }

    void onBeat(ofxBeatLinkBeat& beat) {
        cout << "BPM: " << beat.bpm << " Beat: " << beat.beatWithinBar << "/4" << endl;
    }

    void onDeviceFound(ofxBeatLinkDevice& device) {
        cout << "Found: " << device.deviceName << endl;
    }
};
```

### VirtualCdj Mode (Full Status)

VirtualCdj mode makes your application appear as a CDJ on the network, enabling detailed status updates:

```cpp
#include "ofxBeatLink.h"

class ofApp : public ofBaseApp {
    ofxBeatLink beatLink;

    void setup() {
        // Register for detailed status updates
        ofAddListener(beatLink.deviceUpdateEvent, this, &ofApp::onDeviceUpdate);
        ofAddListener(beatLink.masterChangedEvent, this, &ofApp::onMasterChanged);

        beatLink.start();
        beatLink.startVirtualCdj();  // Enables detailed status
    }

    void update() {
        beatLink.update();
    }

    void exit() {
        beatLink.stopVirtualCdj();
        beatLink.stop();
    }

    void onDeviceUpdate(ofxBeatLinkCdjStatus& status) {
        cout << status.deviceName
             << " Playing:" << status.isPlaying
             << " Master:" << status.isMaster
             << " Sync:" << status.isSynced
             << " OnAir:" << status.isOnAir << endl;
    }

    void onMasterChanged(ofxBeatLinkCdjStatus& status) {
        cout << status.deviceName << " is now tempo master" << endl;
    }
};
```

## API Reference

### ofxBeatLink

Main class for DJ Link communication.

#### Basic Methods

| Method | Description |
|--------|-------------|
| `start()` | Start listening on ports 50000/50001 |
| `stop()` | Stop listening |
| `update()` | Call in ofApp::update() to dispatch events |
| `isRunning()` | Check if listening |
| `getCurrentDevices()` | Get list of detected devices |
| `getLatestBeat(int deviceNumber)` | Get latest beat from specific device |
| `getLatestBeat()` | Get latest beat from any device |

#### VirtualCdj Methods

| Method | Description |
|--------|-------------|
| `startVirtualCdj(int deviceNumber = 0)` | Start as virtual CDJ (0 = auto-assign) |
| `stopVirtualCdj()` | Stop VirtualCdj |
| `isVirtualCdjRunning()` | Check if VirtualCdj is running |
| `getVirtualCdjDeviceNumber()` | Get assigned device number |
| `setVirtualCdjDeviceName(string)` | Set device name (before start) |
| `getTempoMaster()` | Get current tempo master status |
| `getMasterTempo()` | Get master tempo BPM |
| `getCurrentStatuses()` | Get all device statuses |
| `getStatusFor(int deviceNumber)` | Get status for specific device |

### Events

#### Basic Events

| Event | Type | Description |
|-------|------|-------------|
| `beatEvent` | `ofxBeatLinkBeat` | Fired on each beat |
| `deviceFoundEvent` | `ofxBeatLinkDevice` | Fired when device appears |
| `deviceLostEvent` | `ofxBeatLinkDevice` | Fired when device disappears |

#### VirtualCdj Events

| Event | Type | Description |
|-------|------|-------------|
| `deviceUpdateEvent` | `ofxBeatLinkCdjStatus` | Detailed status updates |
| `masterChangedEvent` | `ofxBeatLinkCdjStatus` | Tempo master changed |

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

### ofxBeatLinkCdjStatus

Detailed device status (requires VirtualCdj mode).

| Field | Type | Description |
|-------|------|-------------|
| `deviceNumber` | int | Device number |
| `deviceName` | string | Device name |
| `ipAddress` | string | IP address |
| `isPlaying` | bool | Currently playing |
| `isTrackLoaded` | bool | Track is loaded |
| `isAtCue` | bool | Paused at cue point |
| `isPlayingForwards` | bool | Playing forward |
| `isPlayingBackwards` | bool | Playing backward |
| `isMaster` | bool | Is tempo master |
| `isSynced` | bool | Sync enabled |
| `isOnAir` | bool | Fader is up (on air) |
| `isBpmOnlySynced` | bool | BPM-only sync |
| `bpm` | double | Track BPM |
| `effectiveBpm` | double | BPM with pitch |
| `pitchPercent` | double | Pitch adjustment % |
| `beatWithinBar` | int | Position in bar (1-4) |
| `beatNumber` | int | Beat position in track |
| `trackSourcePlayer` | int | Source player number |
| `rekordboxId` | int | rekordbox track ID |
| `trackNumber` | int | Track number |
| `timestamp` | uint64_t | Update timestamp |

## Examples

### DJ Link Examples
- **example-basic** - Basic passive listener
- **example-beatsync** - Beat-synchronized visual effects
- **example-multidevice** - Multi-device monitoring
- **example-osc** - OSC output for external applications
- **example-statusmonitor** - Status monitor with event log
- **example-cdjstatus** - CDJ status with VirtualCdj mode
- **example-virtualcdj** - Full VirtualCdj status monitor
- **example-bpm-display** - Large BPM display for DJ booth
- **example-timeline** - Beat/BPM history timeline visualization
- **example-waveform** - Waveform display with playhead
- **example-metronome** - Visual metronome synced to DJ Link
- **example-ableton-link** - Ableton Link bridge (requires ofxAbletonLink)

### rekordbox Database Examples (crate-digger-cpp)
- **example-rekordbox-browser** - Browse rekordbox export.pdb database (tracks, artists, albums)
- **example-cue-points** - Display cue points and hot cues from ANLZ files
- **example-song-structure** - Visualize phrase analysis (Intro, Verse, Chorus, etc.)
- **example-beat-grid** - Beat grid visualization with tempo changes
- **example-color-waveform** - RGB color waveform display from ANLZ files
- **example-3band-waveform** - 3-band waveform (Low/Mid/High, CDJ-3000 style)
- **example-playlist-browser** - Browse playlists and track contents
- **example-tag-browser** - Browse tags and categories (exportExt.pdb)
- **example-bpm-search** - Search tracks by BPM range with histogram
- **example-key-wheel** - Circle of Fifths key visualization for harmonic mixing
- **example-artwork** - Album artwork display from rekordbox
- **example-track-detail** - Detailed track information viewer
- **example-loop-viewer** - Saved loops visualization

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

### VirtualCdj fails to start

- Check that no other VirtualCdj instance is running
- Try specifying a device number: `startVirtualCdj(5)` or `startVirtualCdj(6)`
- Ensure ports 50000-50002 are available

## License

EPL-2.0 (same as beat-link-cpp)

## Credits

- [beat-link-cpp](https://github.com/daitomanabe/beat-link-cpp) - C++ DJ Link implementation
- [crate-digger-cpp](https://github.com/daitomanabe/crate-digger-cpp) - C++ rekordbox database parser
- [Deep Symmetry Beat Link](https://github.com/Deep-Symmetry/beat-link) - Original Java library
- [Deep Symmetry Crate Digger](https://github.com/Deep-Symmetry/crate-digger) - Original Java library
