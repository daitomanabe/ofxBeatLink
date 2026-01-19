#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <array>
#include <optional>
#include <numbers>
#include <ranges>

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

    // Beat indicators (C++20 constexpr with std::array)
    static constexpr std::size_t NUM_BEATS = 4;
    std::array<float, NUM_BEATS> beatIndicators{};

    // Device count
    int deviceCount = 0;

    // C++20 helper: lerp for smooth animation
    static constexpr auto smoothLerp(float a, float b, float t) noexcept -> float {
        return std::lerp(a, b, t);
    }
};
