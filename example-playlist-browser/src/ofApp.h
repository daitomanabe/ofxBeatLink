#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Playlist Browser Example
 *
 * Browse rekordbox playlists and their tracks.
 * Displays playlist tree structure and track contents.
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

    // Playlist tree
    std::vector<cratedigger::PlaylistId> playlistIds;
    int selectedPlaylistIndex = 0;
    int playlistScrollOffset = 0;

    // Tracks in selected playlist
    std::vector<cratedigger::TrackId> playlistTracks;
    int selectedTrackIndex = 0;
    int trackScrollOffset = 0;

    // Focus: 0 = playlist panel, 1 = track panel
    int focusPanel = 0;

    void loadDatabase(const std::string& path);
    void selectPlaylist(int index);

    void drawHeader();
    void drawPlaylistPanel();
    void drawTrackPanel();
    void drawInstructions();

    std::string getPlaylistName(cratedigger::PlaylistId id);
    std::string formatDuration(uint32_t seconds);
};
