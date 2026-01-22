#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * 3-Band Waveform Example
 *
 * Displays 3-band waveforms (Low/Mid/High) from rekordbox ANLZ files.
 * CDJ-3000 style visualization with separate frequency bands.
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

    std::vector<cratedigger::TrackId> trackIds;
    int selectedTrackIndex = 0;
    int trackScrollOffset = 0;

    std::optional<cratedigger::TrackRow> selectedTrack;
    std::optional<cratedigger::TrackWaveforms> waveforms;

    float playheadPosition = 0;
    bool showStacked = true;  // Stacked vs overlay mode

    void loadDatabase(const std::string& path);
    void loadAnlzFolder(const std::string& path);
    void selectTrack(int index);

    void drawHeader();
    void drawTrackList();
    void drawStackedWaveform();
    void drawOverlayWaveform();
    void drawLegend();
    void drawInstructions();
};
