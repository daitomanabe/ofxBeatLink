#pragma once

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <set>
#include <system_error>

// Direct access to beat-link-cpp for TCP features
#include <beatlink/VirtualCdj.hpp>
#include <beatlink/CdjStatus.hpp>
#include <beatlink/DeviceUpdateListener.hpp>
#include <beatlink/data/MetadataFinder.hpp>
#include <beatlink/data/TrackMetadata.hpp>
#include <beatlink/data/ArtFinder.hpp>
#include <beatlink/data/AlbumArt.hpp>
#include <beatlink/data/WaveformFinder.hpp>
#include <beatlink/data/WaveformPreview.hpp>

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void exit();
    void keyPressed(int key);

    // Event handlers
    void onDeviceFound(ofxBeatLinkDevice& device);
    void onDeviceLost(ofxBeatLinkDevice& device);

private:
    ofxBeatLink beatLink;
    
    // Status flags
    bool virtualCdjStarted = false;
    bool metadataFinderStarted = false;
    bool artFinderStarted = false;
    bool waveformFinderStarted = false;
    
    // Listener for VirtualCdj updates
    beatlink::DeviceUpdateListenerPtr updateListener;
    
    // Current data for each player
    struct PlayerData {
        std::string title;
        std::string artist;
        ofImage albumArt;
        ofImage waveform;
        bool hasArt = false;
        bool hasWaveform = false;
    };
    std::map<int, PlayerData> playerData;
    
    // Detected devices
    std::vector<ofxBeatLinkDevice> devices;
    
    // Log
    std::deque<std::string> logs;
    void log(const std::string& msg);
};
