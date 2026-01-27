#pragma once

// Comment out this line to disable TCP features (VirtualCdj, track names, waveforms)
//#define ENABLE_TCP_FEATURES

// Uncomment this line to show dummy data when TCP features are disabled (for UI testing)
#define USE_DUMMY_DATA

#include "ofMain.h"
#include "ofxBeatLink.h"
#include "ofxSpout.h"
#include "ofxOsc.h"
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
    ofTrueTypeFont fontTitle;  // For track titles
    
    // FBO and Spout
    ofFbo fbo;
    ofxSpout::Sender spoutSender;
    
    // OSC
    ofxOscSender oscSender;
    
    // Layout settings - centralized configuration
    struct LayoutConfig {
        // Screen settings
        static constexpr float SCREEN_WIDTH = 1920.0f;
        static constexpr float SCREEN_HEIGHT = 1080.0f;
        static constexpr int MAX_DECKS = 2;
        
        // Spacing
        static constexpr float PADDING = 40.0f;
        static constexpr float SECTION_SPACING = 30.0f;
        static constexpr float ELEMENT_SPACING = 15.0f;
        
        // Album art and track info
        static constexpr float ALBUM_ART_SIZE = 120.0f;
        static constexpr float TRACK_INFO_SPACING = 20.0f;  // Space between art and text
        
        // Status badges
        static constexpr float BADGE_HEIGHT = 26.0f;
        static constexpr float BADGE_PADDING = 8.0f;
        static constexpr float BADGE_SPACING = 10.0f;
        
        // BPM display
        static constexpr float BPM_DISPLAY_HEIGHT = 150.0f;
        
        // Beat indicator
        static constexpr float BEAT_CIRCLE_SIZE = 40.0f;
        static constexpr float BEAT_CIRCLE_SPACING = 60.0f;
        
        // Pitch meter
        static constexpr float PITCH_METER_HEIGHT = 80.0f;
        
        // Waveform
        static constexpr float WAVEFORM_HEIGHT = 80.0f;
    };
    
    int currentLayout = 2;  // 2 or 4 deck layout

    // Display data structure (used for both TCP and dummy data)
    struct DisplayData {
        std::string trackTitle;
        std::string trackArtist;
        bool isPlaying = false;
        bool isMaster = false;
        bool isSynced = false;
        bool isOnAir = false;
        ofImage waveformImage;
        ofImage albumArt;
        bool hasWaveform = false;
        bool hasAlbumArt = false;
    };
    
    // Device data (max 2 decks)
    struct DeckInfo {
        ofxBeatLinkDevice device;
        std::optional<ofxBeatLinkBeat> beat;
        float beatAlpha = 0.0f;
        uint64_t lastBeatTime = 0;  // For tracking if playing
        
        DisplayData displayData;  // Display data (TCP or dummy)
        
#ifdef ENABLE_TCP_FEATURES
        // TCP-based data (requires VirtualCdj)
        std::shared_ptr<beatlink::data::TrackMetadata> metadata;
        std::shared_ptr<beatlink::data::WaveformPreview> waveformPreview;
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
    void drawToFbo();  // Draw all content to FBO
    void drawDeckColumn(int deviceNumber, float x, float y, float width, float height);
    void drawPitchMeter(float x, float y, float width, float height, float pitchPercent);
    void drawBeatIndicator(float x, float y, float size, int currentBeat, float alpha);
    void drawStatusBadge(float x, float y, const std::string& text, ofColor color);
    void drawWaveform(float x, float y, float width, float height, const ofImage& waveform);
    void drawAlbumArt(float x, float y, float size, const ofImage& art);
    
    // OSC
    void sendBpmOverOsc(int deviceNumber, float bpm);
    
#ifdef USE_DUMMY_DATA
    void initializeDummyData(DeckInfo& deck);
    void createDummyWaveform(ofImage& image);
    void createDummyAlbumArt(ofImage& image);
#endif

#ifdef ENABLE_TCP_FEATURES
    void updateDisplayDataFromTCP(DeckInfo& deck);
#endif
};
