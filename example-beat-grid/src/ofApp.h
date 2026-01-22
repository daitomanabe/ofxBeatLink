#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Beat Grid Example
 *
 * Visualizes beat grid data from rekordbox ANLZ files.
 * Shows tempo changes and beat positions throughout the track.
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
    bool anlzLoaded = false;
    std::string errorMessage;

    std::vector<cratedigger::TrackId> trackIds;
    int selectedTrackIndex = 0;
    int trackScrollOffset = 0;

    std::optional<cratedigger::TrackRow> selectedTrack;
    std::optional<cratedigger::BeatGrid> beatGrid;

    float playheadMs = 0;
    float zoomLevel = 1.0f;
    float scrollOffsetMs = 0;

    void loadDatabase(const std::string& path);
    void loadAnlzFolder(const std::string& path);
    void selectTrack(int index);

    void drawHeader();
    void drawTrackList();
    void drawBeatGrid();
    void drawTempoGraph();
    void drawInstructions();

    std::string formatTime(float ms);
};
