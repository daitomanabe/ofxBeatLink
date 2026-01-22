#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Track Detail Example
 *
 * Displays comprehensive track information from rekordbox database.
 * Shows all metadata, cue points, and analysis data.
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

    // Track list
    std::vector<cratedigger::TrackId> trackIds;
    int selectedIndex = 0;
    int listScrollOffset = 0;
    static constexpr int VISIBLE_TRACKS = 20;

    // Current track data
    std::optional<cratedigger::TrackRow> currentTrack;
    std::string artistName;
    std::string albumName;
    std::string genreName;
    std::string labelName;
    std::string keyName;
    std::string colorName;
    std::vector<cratedigger::CuePoint> cuePoints;

    // Scroll for detail panel
    int detailScrollOffset = 0;

    void loadDatabase(const std::string& path);
    void selectTrack(int index);
    void loadTrackDetails(cratedigger::TrackId id);

    void drawHeader();
    void drawTrackList();
    void drawTrackDetails();
    void drawCuePoints();
    void drawInstructions();

    std::string formatDuration(uint32_t seconds);
    std::string formatFileSize(uint32_t bytes);
    ofColor getCuePointColor(uint8_t colorId);
};
