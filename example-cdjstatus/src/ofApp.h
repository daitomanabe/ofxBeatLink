#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <map>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

/**
 * CDJ Status Monitor Example
 *
 * Uses VirtualCdj mode to receive detailed CDJ status updates
 * including play state, master, sync, and on-air flags.
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

private:
    ofxBeatLink beatLink;

    // Device info with C++20 features
    struct PlayerInfo {
        ofxBeatLinkDevice device{};
        std::optional<ofxBeatLinkCdjStatus> status;
        std::optional<ofxBeatLinkBeat> lastBeat;
        float beatAlpha = 0.0f;
    };

    std::map<int, PlayerInfo> players;

    // Log messages
    struct LogEntry {
        std::string timestamp;
        std::string message;
        ofColor color;
    };

    std::deque<LogEntry> logMessages;
    static constexpr std::size_t MAX_LOG_MESSAGES = 12;

    void addLog(std::string_view message, const ofColor& color = ofColor(150));
    void drawPlayerPanel(int deviceNumber, float x, float y, float width, float height);
    void drawBeatIndicators(int beatWithinBar, float alpha, float x, float y);
    void drawStatusBadge(float x, float y, std::string_view text, const ofColor& color);
};
