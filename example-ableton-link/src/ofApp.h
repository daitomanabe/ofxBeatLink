#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"

// Uncomment and add ofxAbletonLink to addons.make to enable Link support
// #define USE_ABLETON_LINK
#ifdef USE_ABLETON_LINK
#include "ofxAbletonLink.h"
#endif

/**
 * Ableton Link Bridge Example
 *
 * Bridges DJ Link (Pioneer) beats to Ableton Link protocol.
 * Allows Link-enabled applications (Ableton Live, etc.) to sync
 * with DJ equipment.
 *
 * Requirements for full functionality:
 * - ofxAbletonLink addon (https://github.com/2bbb/ofxAbletonLink)
 * - Add ofxAbletonLink to addons.make
 * - Uncomment #define USE_ABLETON_LINK above
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

#ifdef USE_ABLETON_LINK
    ofxAbletonLink link;
#endif

    // State
    double currentBpm = 120.0;
    int currentBeat = 1;
    float beatPhase = 0.0f;
    float beatAlpha = 0.0f;
    std::string masterDevice;
    int connectedDjLinkDevices = 0;
    int connectedLinkPeers = 0;

    // Settings
    bool linkEnabled = true;
    bool followDjLink = true;  // If true, Link follows DJ; if false, DJ could follow Link

    void updateLink();
    void drawStatus();
    void drawBeatVisualizer();
};
