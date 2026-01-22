#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Artwork Example
 *
 * Displays album artwork from rekordbox database.
 * Drag and drop a PIONEER folder to load.
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
    std::string basePath;

    // Track data
    struct TrackWithArtwork {
        cratedigger::TrackId id;
        std::string title;
        std::string artist;
        std::string album;
        std::string artworkPath;
        ofImage artwork;
        bool artworkLoaded = false;
    };
    std::vector<TrackWithArtwork> tracks;

    // Selection
    int selectedIndex = 0;
    int scrollOffset = 0;
    static constexpr int VISIBLE_TRACKS = 12;

    // Artwork display
    ofImage currentArtwork;
    bool hasCurrentArtwork = false;

    void loadDatabase(const std::string& path);
    void buildTrackList();
    void loadArtworkForTrack(TrackWithArtwork& track);
    void selectTrack(int index);

    void drawHeader();
    void drawTrackList();
    void drawArtworkDisplay();
    void drawInstructions();

    std::string getArtistName(cratedigger::ArtistId id);
    std::string getAlbumName(cratedigger::AlbumId id);
};
