#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <optional>

/**
 * Metronome Example
 *
 * Visual metronome synced to DJ Link devices.
 * Shows beat countdown and next bar timing.
 */
class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;
    void keyPressed(int key) override;

private:
    ofxBeatLink beatLink;

    // Beat state
    std::optional<ofxBeatLinkBeat> latestBeat;
    uint64_t lastBeatTime = 0;
    int deviceCount = 0;

    // Animation
    float beatProgress = 0.0f;
    float beatFlash = 0.0f;
    float pendulumAngle = 0.0f;
    float targetAngle = 0.0f;

    // Visual settings
    bool showPendulum = true;
    bool showCircle = true;
    bool showBars = true;

    // Event handlers
    void onBeat(ofxBeatLinkBeat& beat);
    void onDeviceFound(ofxBeatLinkDevice& device);
    void onDeviceLost(ofxBeatLinkDevice& device);

    // Drawing helpers
    void drawBpmDisplay();
    void drawPendulum();
    void drawBeatCircle();
    void drawBeatBars();
    void drawCountdown();
    void drawDeviceInfo();
    void drawInstructions();
};
