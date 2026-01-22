#pragma once

#include "ofMain.h"
#include <cratedigger/database.hpp>

/**
 * Key Wheel Example
 *
 * Displays tracks organized by musical key in a Circle of Fifths layout.
 * Useful for harmonic mixing and finding compatible tracks.
 */
class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(int key) override;
    void mouseMoved(int x, int y) override;
    void mousePressed(int x, int y, int button) override;
    void dragEvent(ofDragInfo dragInfo) override;

private:
    std::unique_ptr<cratedigger::Database> database;
    bool databaseLoaded = false;
    std::string errorMessage;

    // Key data
    struct KeyData {
        cratedigger::KeyId id;
        std::string name;
        int camelotNumber;      // 1-12
        bool isMinor;           // A or B in Camelot
        std::vector<cratedigger::TrackId> tracks;
        ofColor color;
    };
    std::vector<KeyData> keys;

    // Selection
    int hoveredKeyIndex = -1;
    int selectedKeyIndex = -1;
    int selectedTrackIndex = 0;
    int trackScrollOffset = 0;

    // Wheel parameters
    float wheelCenterX, wheelCenterY;
    float wheelRadius = 250;

    void loadDatabase(const std::string& path);
    void buildKeyData();
    int getKeyIndexAtPosition(float x, float y);

    void drawHeader();
    void drawKeyWheel();
    void drawKeySegment(int index, float innerR, float outerR);
    void drawCompatibleKeys();
    void drawTrackList();
    void drawInstructions();

    std::string getArtistName(cratedigger::ArtistId id);
    std::string formatDuration(uint32_t seconds);
    ofColor getKeyColor(int camelotNum, bool isMinor);
};
