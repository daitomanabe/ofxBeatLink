#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Tag Browser Example
 *
 * Browse rekordbox tags and categories from exportExt.pdb.
 * Shows tag hierarchy with categories and associated tracks.
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
    std::unique_ptr<cratedigger::Database> extDatabase;
    bool databaseLoaded = false;
    bool extDatabaseLoaded = false;
    std::string errorMessage;

    // Categories and tags
    std::vector<cratedigger::TagId> categoryIds;
    int selectedCategoryIndex = 0;

    std::vector<cratedigger::TagId> tagsInCategory;
    int selectedTagIndex = 0;

    // Tracks with selected tag
    std::vector<cratedigger::TrackId> taggedTracks;
    int selectedTrackIndex = 0;
    int trackScrollOffset = 0;

    // Focus: 0=category, 1=tag, 2=tracks
    int focusPanel = 0;

    void loadDatabase(const std::string& path);
    void loadExtDatabase(const std::string& path);
    void selectCategory(int index);
    void selectTag(int index);

    void drawHeader();
    void drawCategoryPanel();
    void drawTagPanel();
    void drawTrackPanel();
    void drawInstructions();

    std::string formatDuration(uint32_t seconds);
};
