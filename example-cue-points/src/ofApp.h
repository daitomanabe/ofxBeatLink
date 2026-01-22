#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Cue Points Example
 *
 * Displays cue points and hot cues from rekordbox ANLZ files.
 * Visualizes memory cues, hot cues, and loops on a timeline.
 *
 * Usage:
 * - Load a rekordbox export.pdb and ANLZ folder
 * - Select tracks to view their cue points
 * - Colors correspond to hot cue colors
 */
class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(int key) override;
    void dragEvent(ofDragInfo dragInfo) override;

private:
    // Database
    std::unique_ptr<cratedigger::Database> database;
    bool databaseLoaded = false;
    bool anlzLoaded = false;
    std::string databasePath;
    std::string anlzPath;
    std::string errorMessage;

    // Track list
    std::vector<cratedigger::TrackId> trackIds;
    int selectedTrackIndex = 0;
    int trackScrollOffset = 0;
    int visibleTrackRows = 15;

    // Selected track data
    std::optional<cratedigger::TrackRow> selectedTrack;
    std::vector<cratedigger::CuePoint> cuePoints;

    // Hot cue colors (rekordbox standard)
    std::vector<ofColor> hotCueColors;

    // Timeline
    float timelineStartX = 300;
    float timelineWidth = 900;
    float timelineY = 500;
    float playheadPosition = 0;  // 0.0 - 1.0

    // Methods
    void loadDatabase(const std::string& path);
    void loadAnlzFolder(const std::string& path);
    void selectTrack(int index);
    void drawHeader();
    void drawTrackList();
    void drawCuePointList();
    void drawTimeline();
    void drawCueOnTimeline(const cratedigger::CuePoint& cue, float trackDuration);
    void drawHotCueGrid();
    void drawInstructions();

    std::string formatTime(float seconds);
    ofColor getCueColor(const cratedigger::CuePoint& cue);
    std::string getCueTypeName(cratedigger::CuePointType type);
};
