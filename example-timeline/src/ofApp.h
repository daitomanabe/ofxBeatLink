#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <deque>

/**
 * Timeline Example
 *
 * Visualizes beat and BPM history as a scrolling timeline.
 * Useful for analyzing DJ sets and transitions.
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

    // Beat history entry
    struct BeatEvent {
        uint64_t timestamp;
        int deviceNumber;
        std::string deviceName;
        double bpm;
        int beatWithinBar;
        ofColor color;
    };

    std::deque<BeatEvent> beatHistory;
    static constexpr size_t MAX_HISTORY = 500;

    // BPM history for graph
    struct BpmSample {
        uint64_t timestamp;
        double bpm;
    };
    std::deque<BpmSample> bpmHistory;
    static constexpr size_t MAX_BPM_SAMPLES = 300;

    // Display settings
    float timelineSeconds = 30.0f;  // How many seconds to show
    bool showBpmGraph = true;
    bool showBeatMarkers = true;

    // Device colors
    std::map<int, ofColor> deviceColors;
    ofColor getDeviceColor(int deviceNumber);

    void drawTimeline();
    void drawBpmGraph();
    void drawBeatMarkers();
    void drawLegend();
};
