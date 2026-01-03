#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"

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

    // Visualization
    std::vector<ofxBeatLinkDevice> devices;
    std::map<int, ofxBeatLinkBeat> deviceBeats;
    std::map<int, float> beatAlpha;  // For beat flash animation

    // Log messages
    std::deque<std::string> logMessages;
    static const size_t MAX_LOG_MESSAGES = 20;

    void addLog(const std::string& message);
};
