#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include "ofxOsc.h"
#include <deque>
#include <string>
#include <string_view>

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
    ofxOscSender oscSender;

    // OSC settings (C++17 inline static constexpr)
    inline static constexpr std::string_view DEFAULT_HOST = "127.0.0.1";
    inline static constexpr int DEFAULT_PORT = 9000;

    std::string oscHost{DEFAULT_HOST};
    int oscPort = DEFAULT_PORT;

    // Statistics
    int messagesSent = 0;
    float messagesPerSecond = 0.0f;
    int messageCountLastSecond = 0;
    float lastSecondTime = 0.0f;

    // OSC message log
    struct OscLogEntry {
        std::string address;
        std::string args;
        float alpha = 1.0f;
    };

    std::deque<OscLogEntry> oscLog;
    inline static constexpr size_t MAX_OSC_LOG = 15;

    void addOscLog(std::string_view address, std::string_view args);

    // Device count
    int deviceCount = 0;
};
