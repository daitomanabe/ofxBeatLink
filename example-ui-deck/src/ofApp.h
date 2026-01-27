#pragma once

// Comment out this line to disable TCP features (VirtualCdj, track names, waveforms)
//#define ENABLE_TCP_FEATURES

#include "ofMain.h"
#include "ofxBeatLink.h"
#include <optional>

#ifdef ENABLE_TCP_FEATURES
#include <beatlink/VirtualCdj.hpp>
#include <beatlink/CdjStatus.hpp>
#include <beatlink/data/MetadataFinder.hpp>
#include <beatlink/data/TrackMetadata.hpp>
#include <beatlink/data/WaveformFinder.hpp>
#endif

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
        
#ifdef ENABLE_TCP_FEATURES
        // TCP-based data (requires VirtualCdj)
        bool isPlaying = false;
        bool isMaster = false;
        bool isSynced = false;
        bool isOnAir = false;
        std::shared_ptr<beatlink::data::TrackMetadata> metadata;
        std::shared_ptr<beatlink::data::WaveformPreview> waveformPreview;
        ofImage waveformImage;
        bool waveformImageReady = false;
#endif
    };
    
    std::map<int, DeckInfo> decks;  // deviceNumber -> deck info

#ifdef ENABLE_TCP_FEATURES
    bool virtualCdjRunning = false;
    bool metadataFinderRunning = false;
    bool waveformFinderRunning = false;
    
    void updateDeviceStatus(int deviceNumber);
    void updateTrackMetadata(int deviceNumber);
    void updateWaveform(int deviceNumber);
    void convertWaveformToImage(DeckInfo& deck);
#endif

    // Draw helpers
    void drawDeckColumn(int deviceNumber, float x, float y, float width, float height);
    void drawProgressBar(float x, float y, float width, float height, float progress, ofColor color);
    void drawPitchMeter(float x, float y, float width, float height, float pitchPercent);
    void drawBeatIndicator(float x, float y, float size, int currentBeat, float alpha);
    
#ifdef ENABLE_TCP_FEATURES
    void drawStatusBadge(float x, float y, const std::string& text, ofColor color);
    void drawWaveform(float x, float y, float width, float height, const ofImage& waveform);
#endif
};
