#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <array>
#include <optional>

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

    // Current beat info (C++17 std::optional)
    std::optional<ofxBeatLinkBeat> latestBeat;

    // Animation state
    float pulseRadius = 100.0f;
    float targetRadius = 100.0f;
    float pulseAlpha = 0.0f;

    // Beat timing
    uint64_t lastBeatTime = 0;
    float beatProgress = 0.0f;

    // Beat indicators (C++17 inline static)
    inline static constexpr size_t NUM_BEATS = 4;
    std::array<float, NUM_BEATS> beatIndicators{};

    // Device count
    int deviceCount = 0;
};
