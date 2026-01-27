#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <optional>

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void exit();

    void keyPressed(int key);

    // Event handlers
    void onBeat(ofxBeatLinkBeat& beat);
    void onDeviceFound(ofxBeatLinkDevice& device);
    void onDeviceLost(ofxBeatLinkDevice& device);

private:
    ofxBeatLink beatLink;

    // Font
    ofTrueTypeFont fontLarge;
    ofTrueTypeFont fontMedium;
    ofTrueTypeFont fontSmall;

    // Device data (max 2 decks)
    struct DeckInfo {
        ofxBeatLinkDevice device;
        std::optional<ofxBeatLinkBeat> beat;
        float beatAlpha = 0.0f;
        uint64_t lastBeatTime = 0;  // For tracking if playing
    };
    
    std::map<int, DeckInfo> decks;  // deviceNumber -> deck info

    // Draw helpers
    void drawDeckColumn(int deviceNumber, float x, float y, float width, float height);
    void drawProgressBar(float x, float y, float width, float height, float progress, ofColor color);
    void drawPitchMeter(float x, float y, float width, float height, float pitchPercent);
    void drawBeatIndicator(float x, float y, float size, int currentBeat, float alpha);
};
