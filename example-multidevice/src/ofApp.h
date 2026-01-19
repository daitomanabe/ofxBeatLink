#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <map>
#include <deque>
#include <optional>
#include <string_view>
#include <ranges>
#include <span>

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

    // Per-device data
    struct DeviceState {
        ofxBeatLinkDevice info{};
        std::optional<ofxBeatLinkBeat> lastBeat;
        float beatAlpha = 0.0f;
    };

    std::map<int, DeviceState> devices;

    // Layout constants (C++20 constexpr)
    static constexpr int MAX_DEVICES = 4;
    static constexpr float PANEL_WIDTH = 450.0f;
    static constexpr float PANEL_HEIGHT = 200.0f;

    void drawDevicePanel(const DeviceState& state, float x, float y, float width, float height);
    void drawEmptySlot(int deviceNum, float x, float y, float width, float height);

    // Log
    std::deque<std::string> logMessages;
    static constexpr std::size_t MAX_LOG = 10;
    void addLog(std::string_view msg);
};
