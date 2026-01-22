#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Rekordbox Database Browser Example
 *
 * Browse rekordbox export.pdb database files.
 * Displays tracks, artists, albums, genres, and playlists.
 *
 * Usage:
 * - Drag and drop an export.pdb file onto the window
 * - Or place export.pdb in bin/data folder
 * - Use arrow keys to navigate, Enter to select
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
    std::string databasePath;
    std::string errorMessage;

    // View modes
    enum class ViewMode {
        Tracks,
        Artists,
        Albums,
        Genres,
        Playlists
    };
    ViewMode currentView = ViewMode::Tracks;

    // Track list
    std::vector<cratedigger::TrackId> trackIds;
    std::vector<cratedigger::ArtistId> artistIds;
    std::vector<cratedigger::AlbumId> albumIds;
    std::vector<cratedigger::GenreId> genreIds;

    // Selection
    int selectedIndex = 0;
    int scrollOffset = 0;
    int visibleRows = 25;

    // Selected track details
    std::optional<cratedigger::TrackRow> selectedTrack;

    // Methods
    void loadDatabase(const std::string& path);
    void refreshLists();
    void drawHeader();
    void drawTabs();
    void drawTrackList();
    void drawArtistList();
    void drawAlbumList();
    void drawGenreList();
    void drawTrackDetails();
    void drawInstructions();

    std::string formatDuration(int seconds);
    std::string getArtistName(cratedigger::ArtistId id);
    std::string getAlbumName(cratedigger::AlbumId id);
    std::string getGenreName(cratedigger::GenreId id);
};
