#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <map>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

/**
 * VirtualCdj Example - Full-featured CDJ Status Monitor
 *
 * Demonstrates VirtualCdj mode which allows receiving detailed
 * device status including play state, master, sync, and on-air flags.
 */
class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;
    void keyPressed(int key) override;

    // Event handlers
    void onBeat(ofxBeatLinkBeat& beat);
    void onDeviceFound(ofxBeatLinkDevice& device);
    void onDeviceLost(ofxBeatLinkDevice& device);
    void onDeviceUpdate(ofxBeatLinkCdjStatus& status);
    void onMasterChanged(ofxBeatLinkCdjStatus& status);

private:
    ofxBeatLink beatLink;

    // Device state tracking
    struct PlayerState {
        ofxBeatLinkDevice info{};
        std::optional<ofxBeatLinkCdjStatus> status;
        std::optional<ofxBeatLinkBeat> lastBeat;
        float beatAlpha = 0.0f;
        uint64_t lastUpdateTime = 0;
    };

    std::map<int, PlayerState> players;
    std::optional<int> currentMaster;

    // Layout constants
    static constexpr int MAX_PLAYERS = 4;
    static constexpr float PANEL_WIDTH = 520.0f;
    static constexpr float PANEL_HEIGHT = 160.0f;

    // Event log
    struct LogEntry {
        std::string timestamp;
        std::string message;
        ofColor color;
    };

    std::deque<LogEntry> eventLog;
    static constexpr std::size_t MAX_LOG = 10;

    void addLog(std::string_view msg, const ofColor& color = ofColor(150));
    void drawPlayerPanel(const PlayerState& player, float x, float y, float width, float height);
    void drawEmptySlot(int slotNum, float x, float y, float width, float height);
    void drawBeatIndicators(float x, float y, int currentBeat, float alpha);
    void drawStatusBadge(float x, float y, std::string_view text, const ofColor& color);
};
