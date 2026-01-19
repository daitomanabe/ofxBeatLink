#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <map>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;
    void keyPressed(int key) override;

    void onBeat(ofxBeatLinkBeat& beat);
    void onDeviceFound(ofxBeatLinkDevice& device);
    void onDeviceLost(ofxBeatLinkDevice& device);

private:
    ofxBeatLink beatLink;

    // Device state tracking (C++17 aggregate with optional)
    struct DeviceStatus {
        ofxBeatLinkDevice info{};
        std::optional<ofxBeatLinkBeat> lastBeat;
        float beatAlpha = 0.0f;

        // Status flags (inferred from beat activity)
        bool isPlaying = false;
        bool isMaster = false;
        bool isSynced = false;
        bool isOnAir = false;

        uint64_t lastUpdateTime = 0;
    };

    std::map<int, DeviceStatus> devices;

    // Layout constants (C++17 inline static constexpr)
    inline static constexpr int MAX_DEVICES = 4;
    inline static constexpr float PANEL_WIDTH = 500.0f;
    inline static constexpr float PANEL_HEIGHT = 180.0f;

    // Event log
    struct LogEntry {
        std::string timestamp;
        std::string message;
        ofColor color;
    };

    std::deque<LogEntry> eventLog;
    inline static constexpr size_t MAX_LOG = 12;

    void addLog(std::string_view msg, const ofColor& color = ofColor(150));
    void drawDevicePanel(const DeviceStatus& status, float x, float y, float width, float height);
    void drawEmptySlot(int deviceNum, float x, float y, float width, float height);
    void drawBeatIndicators(float x, float y, int currentBeat, float alpha);
};
