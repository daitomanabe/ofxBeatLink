#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Loop Viewer Example
 *
 * Displays saved loops from rekordbox ANLZ files.
 * Shows loop start, end, and duration for each track.
 */
class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(int key) override;
    void dragEvent(ofDragInfo dragInfo) override;

private:
    std::unique_ptr<cratedigger::Database> database;
    bool databaseLoaded = false;
    std::string errorMessage;

    // Track data with loops
    struct TrackLoopData {
        cratedigger::TrackId id;
        std::string title;
        std::string artist;
        float bpm = 0;
        uint32_t duration = 0;
        std::vector<cratedigger::CuePoint> loops;
    };
    std::vector<TrackLoopData> tracksWithLoops;

    // Selection
    int selectedTrackIndex = 0;
    int selectedLoopIndex = 0;
    int trackScrollOffset = 0;
    static constexpr int VISIBLE_TRACKS = 15;

    void loadDatabase(const std::string& path);
    void buildLoopList();
    void selectTrack(int index);

    void drawHeader();
    void drawTrackList();
    void drawLoopDetails();
    void drawLoopVisualization();
    void drawInstructions();

    std::string getArtistName(cratedigger::ArtistId id);
    std::string formatTime(uint32_t ms);
    std::string formatDuration(uint32_t seconds);
    ofColor getLoopColor(uint8_t colorId);
};
