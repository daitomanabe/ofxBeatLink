#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <map>
#include <optional>
#include <vector>

/**
 * Waveform Example
 *
 * Demonstrates beat-synchronized waveform visualization.
 * Shows simulated waveform with playhead position based on beat timing.
 *
 * Note: For real waveform data from CDJs, enable VirtualRekordbox support
 * in beat-link-cpp and use WaveformFinder API.
 */
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

    // Player state
    struct PlayerWaveform {
        ofxBeatLinkDevice device;
        std::optional<ofxBeatLinkBeat> lastBeat;

        // Simulated waveform data
        std::vector<float> waveformData;
        float playheadPosition = 0.0f;  // 0-1
        float beatAlpha = 0.0f;
        uint64_t lastBeatTime = 0;
        double bpm = 0.0;
        int beatWithinBar = 1;

        // Waveform colors (simulated CDJ colors)
        ofColor lowColor{0, 0, 255};      // Blue for bass
        ofColor midColor{0, 255, 0};      // Green for mids
        ofColor highColor{255, 255, 255}; // White for highs
    };

    std::map<int, PlayerWaveform> players;

    // Generate simulated waveform
    void generateWaveform(PlayerWaveform& player);
    void updatePlayhead(PlayerWaveform& player);
    void drawWaveform(const PlayerWaveform& player, float x, float y, float width, float height);
    void drawPlayhead(float x, float y, float height, float alpha);
    void drawBeatGrid(const PlayerWaveform& player, float x, float y, float width, float height);
};
