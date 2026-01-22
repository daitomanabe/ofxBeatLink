#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"

/**
 * BPM Display Example
 *
 * Large BPM display suitable for DJ booth monitors.
 * Shows master tempo with beat flash indicators.
 */
class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;
    void keyPressed(int key) override;
    void windowResized(int w, int h) override;

    void onBeat(ofxBeatLinkBeat& beat);
    void onDeviceFound(ofxBeatLinkDevice& device);
    void onDeviceLost(ofxBeatLinkDevice& device);

private:
    ofxBeatLink beatLink;

    // Display state
    double currentBpm = 0.0;
    int currentBeat = 1;
    float beatAlpha = 0.0f;
    std::string masterDeviceName;
    int connectedDevices = 0;

    // Large font for BPM
    ofTrueTypeFont fontBpm;
    ofTrueTypeFont fontInfo;

    void drawBeatIndicators();
    void drawBpmDisplay();
};
