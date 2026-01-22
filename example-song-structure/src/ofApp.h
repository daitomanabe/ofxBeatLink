#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Song Structure Example
 *
 * Displays phrase analysis (song structure) from rekordbox ANLZ files.
 * Shows Intro, Verse, Chorus, Bridge, Outro sections with colors.
 *
 * Usage:
 * - Load a rekordbox export.pdb and ANLZ folder
 * - Select tracks to view their phrase structure
 * - Colors represent phrase types based on mood setting
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
    int visibleTrackRows = 12;

    // Selected track data
    std::optional<cratedigger::TrackRow> selectedTrack;
    std::optional<cratedigger::SongStructure> songStructure;
    std::optional<cratedigger::BeatGrid> beatGrid;

    // Phrase colors by type
    std::map<std::string, ofColor> phraseColors;

    // Timeline
    float timelineStartX = 300;
    float timelineWidth = 1000;
    float timelineY = 150;
    float timelineHeight = 80;
    float playheadBeat = 0;

    // Methods
    void loadDatabase(const std::string& path);
    void loadAnlzFolder(const std::string& path);
    void selectTrack(int index);
    void initPhraseColors();

    void drawHeader();
    void drawTrackList();
    void drawSongStructure();
    void drawPhraseTimeline();
    void drawPhraseList();
    void drawBeatGrid();
    void drawLegend();
    void drawInstructions();

    ofColor getPhraseColor(const cratedigger::PhraseEntry& phrase);
    std::string formatBeat(uint16_t beat);
    float beatToX(uint16_t beat);
};
