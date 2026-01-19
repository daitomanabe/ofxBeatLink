#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <map>
#include <deque>
#include <optional>
#include <string_view>

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;

    void onBeat(ofxBeatLinkBeat& beat);
    void onDeviceFound(ofxBeatLinkDevice& device);
    void onDeviceLost(ofxBeatLinkDevice& device);

private:
    ofxBeatLink beatLink;

    // Per-device data (C++17 aggregate initialization)
    struct DeviceState {
        ofxBeatLinkDevice info{};
        std::optional<ofxBeatLinkBeat> lastBeat;
        float beatAlpha = 0.0f;
    };

    std::map<int, DeviceState> devices;

    // Layout constants (C++17 inline static constexpr)
    inline static constexpr int MAX_DEVICES = 4;
    inline static constexpr float PANEL_WIDTH = 450.0f;
    inline static constexpr float PANEL_HEIGHT = 200.0f;

    void drawDevicePanel(const DeviceState& state, float x, float y, float width, float height);
    void drawEmptySlot(int deviceNum, float x, float y, float width, float height);

    // Log
    std::deque<std::string> logMessages;
    inline static constexpr size_t MAX_LOG = 10;
    void addLog(std::string_view msg);
};
