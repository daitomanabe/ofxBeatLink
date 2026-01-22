#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Color Waveform Example
 *
 * Displays RGB color waveforms from rekordbox ANLZ files.
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

    void loadDatabase(const std::string& path);
    void loadAnlzFolder(const std::string& path);
    void selectTrack(int index);

    void drawHeader();
    void drawTrackList();
    void drawWaveform();
    void drawPreviewWaveform();
    void drawDetailWaveform();
    void drawInstructions();
};
