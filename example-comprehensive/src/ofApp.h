#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <cratedigger/database.hpp>
#include <optional>
#include <array>
#include <deque>

/**
 * Comprehensive Monitor Example
 *
 * Full-featured DJ Link monitor displaying detailed information for 4 CDJs:
 * - Waveforms (color/3-band if available)
 * - Beat grid and beat position
 * - BPM, pitch, key
 * - Play status, master, sync, on-air
 * - Cue points and loops
 * - Song structure/phrases
 * - Track info (title, artist, album)
 * - Next beat/bar timing
 * - Beat history graph
 *
 * Requires VirtualCdj mode for full status information.
 * Optionally load rekordbox database for waveforms and cue points.
 */
class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;
    void keyPressed(int key) override;
    void dragEvent(ofDragInfo dragInfo) override;

private:
    // ========================================================================
    // DJ Link
    // ========================================================================
    ofxBeatLink beatLink;
    bool virtualCdjRunning = false;

    // ========================================================================
    // rekordbox Database (optional)
    // ========================================================================
    std::unique_ptr<cratedigger::Database> database;
    bool databaseLoaded = false;
    std::string databasePath;

    // ========================================================================
    // Per-Device State (4 CDJs)
    // ========================================================================
    static constexpr int MAX_DEVICES = 4;

    struct WaveformCache {
        std::vector<uint8_t> preview;      // Monochrome preview
        std::vector<uint8_t> colorData;    // RGB color data
        std::vector<uint8_t> threeBandData; // 3-band data
        cratedigger::WaveformStyle style = cratedigger::WaveformStyle::Blue;
        size_t entryCount = 0;
        bool loaded = false;
    };

    struct CuePointDisplay {
        float positionPercent;  // 0.0 - 1.0
        uint8_t colorId;
        bool isHotCue;
        uint8_t hotCueNumber;
        bool isLoop;
        float loopEndPercent;
    };

    struct PhraseDisplay {
        float startPercent;
        float endPercent;
        std::string name;
        ofColor color;
    };

    struct BeatHistoryEntry {
        uint64_t timestamp;
        double bpm;
    };

    struct DeviceState {
        // Connection
        bool connected = false;
        ofxBeatLinkDevice info;

        // Status (from VirtualCdj)
        std::optional<ofxBeatLinkCdjStatus> status;

        // Beat
        std::optional<ofxBeatLinkBeat> lastBeat;
        uint64_t lastBeatTime = 0;
        float beatAlpha = 0.0f;
        float beatProgress = 0.0f;

        // Track info
        std::string trackTitle;
        std::string trackArtist;
        std::string trackAlbum;
        std::string trackKey;
        uint32_t trackDurationMs = 0;

        // Waveform
        WaveformCache waveform;
        float playheadPosition = 0.0f;  // 0.0 - 1.0

        // Cue points
        std::vector<CuePointDisplay> cuePoints;

        // Song structure
        std::vector<PhraseDisplay> phrases;
        int currentPhraseIndex = -1;

        // Beat history (for mini graph)
        std::deque<BeatHistoryEntry> beatHistory;
        static constexpr size_t MAX_BEAT_HISTORY = 60;

        // Animation
        float flashAlpha = 0.0f;
    };

    std::array<DeviceState, MAX_DEVICES> devices;

    // ========================================================================
    // Global State
    // ========================================================================
    int masterDeviceNumber = -1;
    double masterTempo = 0.0;
    uint64_t lastUpdateTime = 0;

    // Display options
    bool showWaveforms = true;
    bool showCuePoints = true;
    bool showPhrases = true;
    bool showBeatHistory = true;
    bool compactMode = false;

    // ========================================================================
    // Event Handlers
    // ========================================================================
    void onBeat(ofxBeatLinkBeat& beat);
    void onDeviceFound(ofxBeatLinkDevice& device);
    void onDeviceLost(ofxBeatLinkDevice& device);
    void onDeviceUpdate(ofxBeatLinkCdjStatus& status);
    void onMasterChanged(ofxBeatLinkCdjStatus& status);

    // ========================================================================
    // Database Loading
    // ========================================================================
    void loadDatabase(const std::string& path);
    void loadTrackDataForDevice(int deviceNumber, const std::string& trackPath);
    void loadWaveformForDevice(int deviceNumber, const std::string& trackPath);
    void loadCuePointsForDevice(int deviceNumber, const std::string& trackPath);
    void loadSongStructureForDevice(int deviceNumber, const std::string& trackPath);

    // ========================================================================
    // Drawing
    // ========================================================================
    void drawHeader();
    void drawDevicePanel(int deviceNumber, float x, float y, float width, float height);
    void drawDeviceHeader(const DeviceState& state, float x, float y, float width);
    void drawStatusIndicators(const DeviceState& state, float x, float y);
    void drawBpmDisplay(const DeviceState& state, float x, float y);
    void drawWaveform(const DeviceState& state, float x, float y, float width, float height);
    void drawCuePoints(const DeviceState& state, float x, float y, float width, float height);
    void drawPhrases(const DeviceState& state, float x, float y, float width, float height);
    void drawBeatIndicators(const DeviceState& state, float x, float y);
    void drawBeatProgress(const DeviceState& state, float x, float y, float width);
    void drawTimingInfo(const DeviceState& state, float x, float y);
    void drawBeatHistory(const DeviceState& state, float x, float y, float width, float height);
    void drawEmptySlot(int deviceNumber, float x, float y, float width, float height);
    void drawMasterInfo();
    void drawInstructions();
    void drawDatabaseStatus();

    // ========================================================================
    // Helpers
    // ========================================================================
    ofColor getCuePointColor(uint8_t colorId);
    ofColor getPhraseColor(const std::string& phraseName);
    std::string formatTime(uint32_t ms);
    std::string formatBpm(double bpm);
};
