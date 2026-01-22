#include "ofApp.h"
#include <algorithm>
#include <filesystem>

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("ofxBeatLink - Loop Viewer");
}

void ofApp::update() {
}

void ofApp::draw() {
    drawHeader();

    if (!databaseLoaded) {
        drawInstructions();
        return;
    }

    if (!errorMessage.empty()) {
        ofSetColor(255, 80, 80);
        ofDrawBitmapString(errorMessage, 30, 100);
        return;
    }

    if (tracksWithLoops.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No tracks with loops found", 30, 100);
        ofDrawBitmapString("Make sure ANLZ files are present in PIONEER/USBANLZ folder", 30, 120);
        return;
    }

    drawTrackList();
    drawLoopDetails();
    drawLoopVisualization();
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - Loop Viewer", 30, 30);

    if (databaseLoaded) {
        ofSetColor(120);
        ofDrawBitmapString("Tracks with loops: " + ofToString(tracksWithLoops.size()), 30, 50);
        ofDrawBitmapString("[Up/Down] Track  [Left/Right] Loop  [q] Quit",
                          ofGetWidth() - 380, 30);
    }
}

void ofApp::drawInstructions() {
    ofSetColor(100);
    ofDrawBitmapString("Drag and drop a PIONEER folder to load rekordbox database", 30, 100);
    ofDrawBitmapString("The folder should contain export.pdb file and ANLZ data", 30, 120);

    ofSetColor(60);
    ofDrawBitmapString("This example displays saved loops from cue points.", 30, 160);
    ofDrawBitmapString("Loops are created when you set loop points in rekordbox.", 30, 180);
}

void ofApp::drawTrackList() {
    const float listX = 30;
    const float listY = 70;
    const float listWidth = 500;
    const float rowHeight = 20;

    // Background
    ofSetColor(20);
    ofDrawRectangle(listX - 5, listY - 5, listWidth, VISIBLE_TRACKS * rowHeight + 10);

    // Track list
    for (int i = 0; i < VISIBLE_TRACKS && (trackScrollOffset + i) < static_cast<int>(tracksWithLoops.size()); ++i) {
        int idx = trackScrollOffset + i;
        const auto& track = tracksWithLoops[idx];
        float y = listY + i * rowHeight;

        // Selection highlight
        if (idx == selectedTrackIndex) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(listX - 3, y - 2, listWidth - 4, rowHeight - 2);
        }

        // Loop count indicator
        ofSetColor(100, 200, 255);
        ofDrawBitmapString("[" + ofToString(track.loops.size()) + "]", listX, y + 13);

        // Title
        ofSetColor(idx == selectedTrackIndex ? 255 : 180);
        std::string title = track.title;
        if (title.length() > 30) title = title.substr(0, 27) + "...";
        ofDrawBitmapString(title, listX + 35, y + 13);

        // Artist
        ofSetColor(idx == selectedTrackIndex ? 150 : 100);
        std::string artist = track.artist;
        if (artist.length() > 18) artist = artist.substr(0, 15) + "...";
        ofDrawBitmapString(artist, listX + 290, y + 13);

        // BPM
        ofSetColor(80, 200, 120);
        ofDrawBitmapString(ofToString(track.bpm, 1), listX + 430, y + 13);
    }

    // Scrollbar
    if (tracksWithLoops.size() > static_cast<size_t>(VISIBLE_TRACKS)) {
        float scrollbarHeight = VISIBLE_TRACKS * rowHeight;
        float thumbHeight = scrollbarHeight * VISIBLE_TRACKS / tracksWithLoops.size();
        float thumbY = listY + (scrollbarHeight - thumbHeight) * trackScrollOffset / (tracksWithLoops.size() - VISIBLE_TRACKS);

        ofSetColor(40);
        ofDrawRectangle(listX + listWidth - 8, listY, 6, scrollbarHeight);
        ofSetColor(80);
        ofDrawRectangle(listX + listWidth - 8, thumbY, 6, thumbHeight);
    }
}

void ofApp::drawLoopDetails() {
    if (tracksWithLoops.empty()) return;

    const auto& track = tracksWithLoops[selectedTrackIndex];
    const float detailX = 560;
    const float detailY = 70;
    const float detailWidth = 300;
    const float lineHeight = 24;

    // Background
    ofSetColor(30);
    ofDrawRectangle(detailX - 10, detailY - 10, detailWidth, 200);

    // Track info
    ofSetColor(255);
    ofDrawBitmapString("LOOPS FOR:", detailX, detailY);

    ofSetColor(200);
    std::string title = track.title;
    if (title.length() > 35) title = title.substr(0, 32) + "...";
    ofDrawBitmapString(title, detailX, detailY + 20);

    ofSetColor(120);
    ofDrawBitmapString(track.artist, detailX, detailY + 40);

    float y = detailY + 75;

    // Loop list
    for (size_t i = 0; i < track.loops.size() && i < 5; ++i) {
        const auto& loop = track.loops[i];
        bool isSelected = (static_cast<int>(i) == selectedLoopIndex);

        // Selection indicator
        if (isSelected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(detailX - 5, y - 4, detailWidth - 10, lineHeight - 2);
        }

        // Loop number/type
        ofColor loopColor = getLoopColor(loop.color_id);
        ofSetColor(loopColor);

        std::string loopLabel;
        if (loop.is_hot_cue()) {
            loopLabel = "HOT " + ofToString(loop.hot_cue_number);
        } else {
            loopLabel = "LOOP " + ofToString(i + 1);
        }
        ofDrawBitmapString(loopLabel, detailX, y + 10);

        // Time range
        ofSetColor(isSelected ? 255 : 180);
        std::string timeStr = formatTime(loop.time_ms) + " - " + formatTime(loop.loop_time_ms);
        ofDrawBitmapString(timeStr, detailX + 70, y + 10);

        // Duration
        ofSetColor(100, 180, 255);
        ofDrawBitmapString(ofToString(loop.loop_duration_ms()) + "ms", detailX + 200, y + 10);

        y += lineHeight;
    }

    if (track.loops.size() > 5) {
        ofSetColor(80);
        ofDrawBitmapString("... and " + ofToString(track.loops.size() - 5) + " more loops", detailX, y + 10);
    }
}

void ofApp::drawLoopVisualization() {
    if (tracksWithLoops.empty()) return;

    const auto& track = tracksWithLoops[selectedTrackIndex];
    if (track.loops.empty()) return;

    const float vizX = 560;
    const float vizY = 290;
    const float vizWidth = 600;
    const float vizHeight = 100;
    const float trackDuration = static_cast<float>(track.duration * 1000);  // in ms

    if (trackDuration <= 0) return;

    // Background
    ofSetColor(20);
    ofDrawRectangle(vizX - 10, vizY - 10, vizWidth + 20, vizHeight + 60);

    // Title
    ofSetColor(100);
    ofDrawBitmapString("LOOP POSITIONS IN TRACK", vizX, vizY);

    // Track timeline
    ofSetColor(40);
    ofDrawRectangle(vizX, vizY + 20, vizWidth, vizHeight);

    // Draw loops on timeline
    for (size_t i = 0; i < track.loops.size(); ++i) {
        const auto& loop = track.loops[i];
        bool isSelected = (static_cast<int>(i) == selectedLoopIndex);

        float startX = vizX + (loop.time_ms / trackDuration) * vizWidth;
        float endX = vizX + (loop.loop_time_ms / trackDuration) * vizWidth;
        float loopWidth = std::max(endX - startX, 3.0f);

        // Loop bar
        ofColor loopColor = getLoopColor(loop.color_id);
        int alpha = isSelected ? 255 : 150;
        ofSetColor(loopColor.r, loopColor.g, loopColor.b, alpha);
        ofDrawRectangle(startX, vizY + 25 + i * 12, loopWidth, 10);

        // Border for selected
        if (isSelected) {
            ofNoFill();
            ofSetColor(255);
            ofSetLineWidth(2);
            ofDrawRectangle(startX - 1, vizY + 24 + i * 12, loopWidth + 2, 12);
            ofFill();
            ofSetLineWidth(1);
        }
    }

    // Time markers
    ofSetColor(80);
    for (int i = 0; i <= 4; ++i) {
        float x = vizX + (vizWidth * i / 4.0f);
        ofDrawLine(x, vizY + vizHeight + 20, x, vizY + vizHeight + 30);

        uint32_t timeMs = static_cast<uint32_t>(trackDuration * i / 4.0f);
        ofDrawBitmapString(formatTime(timeMs), x - 20, vizY + vizHeight + 45);
    }

    // Selected loop details
    if (selectedLoopIndex < static_cast<int>(track.loops.size())) {
        const auto& loop = track.loops[selectedLoopIndex];

        float infoY = vizY + vizHeight + 70;
        ofSetColor(100);
        ofDrawBitmapString("Selected Loop:", vizX, infoY);

        ofSetColor(200);
        ofDrawBitmapString("Start: " + formatTime(loop.time_ms), vizX + 120, infoY);
        ofDrawBitmapString("End: " + formatTime(loop.loop_time_ms), vizX + 250, infoY);

        ofSetColor(100, 200, 255);
        ofDrawBitmapString("Duration: " + ofToString(loop.loop_duration_ms()) + " ms", vizX + 370, infoY);

        // Calculate beats (approximate)
        if (track.bpm > 0) {
            float beatMs = 60000.0f / track.bpm;
            float loopBeats = loop.loop_duration_ms() / beatMs;
            ofSetColor(255, 200, 100);
            ofDrawBitmapString("(" + ofToString(loopBeats, 1) + " beats)", vizX + 510, infoY);
        }
    }
}

void ofApp::keyPressed(int key) {
    if (key == 'q' || key == 'Q') {
        ofExit();
        return;
    }

    if (!databaseLoaded || tracksWithLoops.empty()) return;

    if (key == OF_KEY_UP) {
        selectTrack(selectedTrackIndex - 1);
    } else if (key == OF_KEY_DOWN) {
        selectTrack(selectedTrackIndex + 1);
    } else if (key == OF_KEY_PAGE_UP) {
        selectTrack(selectedTrackIndex - VISIBLE_TRACKS);
    } else if (key == OF_KEY_PAGE_DOWN) {
        selectTrack(selectedTrackIndex + VISIBLE_TRACKS);
    } else if (key == OF_KEY_LEFT) {
        // Previous loop
        if (!tracksWithLoops[selectedTrackIndex].loops.empty()) {
            selectedLoopIndex = std::max(0, selectedLoopIndex - 1);
        }
    } else if (key == OF_KEY_RIGHT) {
        // Next loop
        const auto& loops = tracksWithLoops[selectedTrackIndex].loops;
        if (!loops.empty()) {
            selectedLoopIndex = std::min(static_cast<int>(loops.size()) - 1, selectedLoopIndex + 1);
        }
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (!dragInfo.files.empty()) {
        loadDatabase(dragInfo.files[0]);
    }
}

void ofApp::loadDatabase(const std::string& path) {
    namespace fs = std::filesystem;

    errorMessage.clear();
    tracksWithLoops.clear();
    databaseLoaded = false;

    // Find export.pdb
    fs::path dbPath;
    fs::path anlzPath;
    fs::path inputPath(path);

    if (fs::is_directory(inputPath)) {
        dbPath = inputPath / "rekordbox" / "export.pdb";
        anlzPath = inputPath / "PIONEER" / "USBANLZ";
        if (!fs::exists(dbPath)) {
            dbPath = inputPath / "export.pdb";
            anlzPath = inputPath.parent_path() / "PIONEER" / "USBANLZ";
        }
    } else if (inputPath.filename() == "export.pdb") {
        dbPath = inputPath;
        anlzPath = inputPath.parent_path().parent_path() / "PIONEER" / "USBANLZ";
    }

    if (!fs::exists(dbPath)) {
        errorMessage = "Could not find export.pdb in: " + path;
        return;
    }

    auto result = cratedigger::Database::open(dbPath);
    if (!result) {
        errorMessage = "Failed to open database: " + result.error().message;
        return;
    }

    database = std::make_unique<cratedigger::Database>(std::move(*result));

    // Load ANLZ files
    if (fs::exists(anlzPath)) {
        database->load_cue_points(anlzPath);
    } else {
        errorMessage = "ANLZ folder not found. Loops require ANLZ files.";
        return;
    }

    databaseLoaded = true;
    buildLoopList();

    if (!tracksWithLoops.empty()) {
        selectTrack(0);
    }
}

void ofApp::buildLoopList() {
    tracksWithLoops.clear();

    auto trackIds = database->all_track_ids();

    for (const auto& id : trackIds) {
        auto trackOpt = database->get_track(id);
        if (!trackOpt) continue;

        auto cuePoints = database->get_cue_points_for_track(id);

        // Filter to loops only
        std::vector<cratedigger::CuePoint> loops;
        for (const auto& cue : cuePoints) {
            if (cue.is_loop()) {
                loops.push_back(cue);
            }
        }

        if (loops.empty()) continue;

        TrackLoopData data;
        data.id = id;
        data.title = trackOpt->title;
        data.artist = getArtistName(trackOpt->artist_id);
        data.bpm = trackOpt->bpm();
        data.duration = trackOpt->duration_seconds;
        data.loops = std::move(loops);

        tracksWithLoops.push_back(std::move(data));
    }

    // Sort by number of loops (descending)
    std::sort(tracksWithLoops.begin(), tracksWithLoops.end(),
              [](const TrackLoopData& a, const TrackLoopData& b) {
                  return a.loops.size() > b.loops.size();
              });
}

void ofApp::selectTrack(int index) {
    if (tracksWithLoops.empty()) return;

    selectedTrackIndex = std::max(0, std::min(index, static_cast<int>(tracksWithLoops.size()) - 1));
    selectedLoopIndex = 0;

    // Adjust scroll
    if (selectedTrackIndex < trackScrollOffset) {
        trackScrollOffset = selectedTrackIndex;
    } else if (selectedTrackIndex >= trackScrollOffset + VISIBLE_TRACKS) {
        trackScrollOffset = selectedTrackIndex - VISIBLE_TRACKS + 1;
    }
}

std::string ofApp::getArtistName(cratedigger::ArtistId id) {
    if (id.value == 0) return "";
    auto artist = database->get_artist(id);
    return artist ? artist->name : "";
}

std::string ofApp::formatTime(uint32_t ms) {
    int mins = ms / 60000;
    int secs = (ms / 1000) % 60;
    int millis = (ms % 1000) / 10;
    return ofToString(mins) + ":" + ofToString(secs, 0, 2, '0') + "." + ofToString(millis, 0, 2, '0');
}

std::string ofApp::formatDuration(uint32_t seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return ofToString(mins) + ":" + ofToString(secs, 0, 2, '0');
}

ofColor ofApp::getLoopColor(uint8_t colorId) {
    switch (colorId) {
        case 0: return ofColor(100, 180, 255);  // Default blue
        case 1: return ofColor(255, 80, 80);    // Red
        case 2: return ofColor(255, 180, 80);   // Orange
        case 3: return ofColor(255, 255, 80);   // Yellow
        case 4: return ofColor(80, 255, 80);    // Green
        case 5: return ofColor(80, 255, 255);   // Aqua
        case 6: return ofColor(80, 80, 255);    // Blue
        case 7: return ofColor(180, 80, 255);   // Purple
        case 8: return ofColor(255, 80, 180);   // Pink
        default: return ofColor(100, 180, 255);
    }
}
