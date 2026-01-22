#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * BPM Search Example
 *
 * Search rekordbox database by BPM range.
 * Useful for finding tracks that match a target tempo.
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

    // BPM range
    float bpmMin = 120.0f;
    float bpmMax = 130.0f;
    float bpmStep = 5.0f;
    bool editingMin = true;

    // Search results
    std::vector<cratedigger::TrackId> searchResults;
    int selectedIndex = 0;
    int scrollOffset = 0;

    // BPM distribution histogram
    std::map<int, int> bpmHistogram;

    void loadDatabase(const std::string& path);
    void performSearch();
    void buildHistogram();

    void drawHeader();
    void drawBpmControls();
    void drawHistogram();
    void drawResults();
    void drawTrackDetails();
    void drawInstructions();

    std::string formatDuration(uint32_t seconds);
    std::string getArtistName(cratedigger::ArtistId id);
};
