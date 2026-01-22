#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("Beat Grid Viewer");

    std::string defaultPdb = ofToDataPath("export.pdb");
    if (ofFile::doesFileExist(defaultPdb)) {
        loadDatabase(defaultPdb);
    }

    std::string defaultAnlz = ofToDataPath("PIONEER/USBANLZ");
    if (ofDirectory::doesDirectoryExist(defaultAnlz)) {
        loadAnlzFolder(defaultAnlz);
    }
}

void ofApp::update() {
    if (beatGrid && !beatGrid->empty()) {
        float duration = beatGrid->beats.back().time_ms;
        playheadMs += 16.67f;  // ~60fps
        if (playheadMs > duration) playheadMs = 0;
    }
}

void ofApp::draw() {
    drawHeader();

    if (!databaseLoaded) {
        ofSetColor(80);
        ofNoFill();
        ofDrawRectangle(100, 150, ofGetWidth() - 200, 200);
        ofFill();
        ofSetColor(120);
        ofDrawBitmapString("Drag and drop export.pdb here", ofGetWidth() / 2 - 120, 250);
        ofDrawBitmapString("Then drag PIONEER/USBANLZ folder", ofGetWidth() / 2 - 130, 280);
    } else {
        drawTrackList();
        drawBeatGrid();
        drawTempoGraph();
    }

    if (!errorMessage.empty()) {
        ofSetColor(255, 100, 100);
        ofDrawBitmapString("Error: " + errorMessage, 20, ofGetHeight() - 60);
    }

    drawInstructions();
}

void ofApp::keyPressed(int key) {
    if (!databaseLoaded) return;

    if (key == OF_KEY_UP) {
        selectedTrackIndex = std::max(0, selectedTrackIndex - 1);
        if (selectedTrackIndex < trackScrollOffset) trackScrollOffset = selectedTrackIndex;
        selectTrack(selectedTrackIndex);
    } else if (key == OF_KEY_DOWN) {
        selectedTrackIndex = std::min(static_cast<int>(trackIds.size()) - 1, selectedTrackIndex + 1);
        if (selectedTrackIndex >= trackScrollOffset + 12) trackScrollOffset = selectedTrackIndex - 11;
        selectTrack(selectedTrackIndex);
    } else if (key == '+' || key == '=') {
        zoomLevel = std::min(10.0f, zoomLevel * 1.5f);
    } else if (key == '-') {
        zoomLevel = std::max(0.1f, zoomLevel / 1.5f);
    } else if (key == OF_KEY_LEFT) {
        scrollOffsetMs = std::max(0.0f, scrollOffsetMs - 5000.0f);
    } else if (key == OF_KEY_RIGHT) {
        scrollOffsetMs += 5000.0f;
    } else if (key == ' ') {
        playheadMs = 0;
        scrollOffsetMs = 0;
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.empty()) return;
    std::string path = dragInfo.files[0];
    if (path.find(".pdb") != std::string::npos) {
        loadDatabase(path);
    } else if (ofDirectory::doesDirectoryExist(path)) {
        loadAnlzFolder(path);
    }
}

void ofApp::loadDatabase(const std::string& path) {
    errorMessage.clear();
    auto result = cratedigger::Database::open(path);
    if (!result.has_value()) {
        errorMessage = result.error().message;
        return;
    }
    database = std::make_unique<cratedigger::Database>(std::move(result.value()));
    databaseLoaded = true;
    trackIds = database->all_track_ids();
    if (!trackIds.empty()) selectTrack(0);
}

void ofApp::loadAnlzFolder(const std::string& path) {
    if (!database) return;
    database->load_cue_points(path);
    anlzLoaded = true;
    if (selectedTrackIndex < static_cast<int>(trackIds.size())) {
        selectTrack(selectedTrackIndex);
    }
}

void ofApp::selectTrack(int index) {
    if (index < 0 || index >= static_cast<int>(trackIds.size())) return;
    selectedTrack = database->get_track(trackIds[index]);
    beatGrid.reset();
    playheadMs = 0;
    scrollOffsetMs = 0;

    if (anlzLoaded) {
        auto* grid = database->get_beat_grid_for_track(trackIds[index]);
        if (grid) beatGrid = *grid;
    }
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("Beat Grid Viewer", 20, 25);

    if (selectedTrack) {
        ofSetColor(255, 200, 80);
        ofDrawBitmapString(selectedTrack->title, 200, 25);
        ofSetColor(150);
        ofDrawBitmapString("BPM: " + ofToString(selectedTrack->bpm(), 1), 200, 45);
    }

    ofSetColor(100);
    ofDrawBitmapString("Zoom: " + ofToString(zoomLevel, 1) + "x", ofGetWidth() - 150, 25);
}

void ofApp::drawTrackList() {
    float x = 20, y = 70;
    ofSetColor(200);
    ofDrawBitmapString("== Tracks ==", x, y);
    y += 20;

    int endIndex = std::min(trackScrollOffset + 12, static_cast<int>(trackIds.size()));
    for (int i = trackScrollOffset; i < endIndex; ++i) {
        auto track = database->get_track(trackIds[i]);
        if (!track) continue;

        bool selected = (i == selectedTrackIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 5, y - 12, 170, 18);
        }
        ofSetColor(selected ? 255 : 150);
        std::string title = track->title;
        if (title.length() > 20) title = title.substr(0, 17) + "...";
        ofDrawBitmapString(title, x, y);
        y += 18;
    }
}

void ofApp::drawBeatGrid() {
    if (!beatGrid || beatGrid->empty()) {
        ofSetColor(100);
        ofDrawBitmapString(anlzLoaded ? "No beat grid data" : "Load ANLZ folder", 250, 100);
        return;
    }

    float gridX = 220, gridY = 70;
    float gridW = ofGetWidth() - 250;
    float gridH = 200;

    // Background
    ofSetColor(35);
    ofDrawRectangle(gridX, gridY, gridW, gridH);

    float duration = beatGrid->beats.back().time_ms;
    float visibleDuration = duration / zoomLevel;
    float startMs = scrollOffsetMs;
    float endMs = startMs + visibleDuration;

    // Draw beats
    for (const auto& beat : beatGrid->beats) {
        if (beat.time_ms < startMs || beat.time_ms > endMs) continue;

        float x = gridX + ((beat.time_ms - startMs) / visibleDuration) * gridW;

        // Color by beat position in bar
        if (beat.beat_number == 1) {
            ofSetColor(255, 100, 100);  // Downbeat - red
        } else {
            ofSetColor(100, 200, 100);  // Other beats - green
        }

        float height = (beat.beat_number == 1) ? gridH : gridH * 0.6f;
        float yOffset = (beat.beat_number == 1) ? 0 : gridH * 0.2f;
        ofDrawLine(x, gridY + yOffset, x, gridY + yOffset + height);

        // Beat number
        if (zoomLevel > 2.0f) {
            ofSetColor(80);
            ofDrawBitmapString(ofToString(beat.beat_number), x - 3, gridY + gridH + 15);
        }
    }

    // Playhead
    if (playheadMs >= startMs && playheadMs <= endMs) {
        float playX = gridX + ((playheadMs - startMs) / visibleDuration) * gridW;
        ofSetColor(255, 255, 100);
        ofDrawLine(playX, gridY - 5, playX, gridY + gridH + 5);
    }

    // Time markers
    ofSetColor(80);
    int markerInterval = static_cast<int>(visibleDuration / 10000) * 1000;
    if (markerInterval < 1000) markerInterval = 1000;
    for (float ms = startMs; ms <= endMs; ms += markerInterval) {
        float x = gridX + ((ms - startMs) / visibleDuration) * gridW;
        ofDrawLine(x, gridY + gridH, x, gridY + gridH + 5);
        ofDrawBitmapString(formatTime(ms), x - 15, gridY + gridH + 20);
    }

    // Current time
    ofSetColor(255);
    ofDrawBitmapString("Time: " + formatTime(playheadMs), gridX, gridY - 10);

    // Legend
    ofSetColor(255, 100, 100);
    ofDrawRectangle(gridX + gridW - 150, gridY - 15, 10, 10);
    ofSetColor(150);
    ofDrawBitmapString("Downbeat", gridX + gridW - 135, gridY - 6);

    ofSetColor(100, 200, 100);
    ofDrawRectangle(gridX + gridW - 60, gridY - 15, 10, 10);
    ofSetColor(150);
    ofDrawBitmapString("Beat", gridX + gridW - 45, gridY - 6);
}

void ofApp::drawTempoGraph() {
    if (!beatGrid || beatGrid->empty()) return;

    float graphX = 220, graphY = 320;
    float graphW = ofGetWidth() - 250;
    float graphH = 150;

    // Background
    ofSetColor(35);
    ofDrawRectangle(graphX, graphY, graphW, graphH);

    // Find BPM range
    float minBpm = 999, maxBpm = 0;
    for (const auto& beat : beatGrid->beats) {
        float bpm = beat.bpm();
        if (bpm > 0) {
            minBpm = std::min(minBpm, bpm);
            maxBpm = std::max(maxBpm, bpm);
        }
    }

    if (maxBpm == minBpm) {
        minBpm -= 5;
        maxBpm += 5;
    }

    float duration = beatGrid->beats.back().time_ms;
    float visibleDuration = duration / zoomLevel;
    float startMs = scrollOffsetMs;
    float endMs = startMs + visibleDuration;

    // Draw tempo line
    ofSetColor(100, 200, 255);
    ofNoFill();
    ofBeginShape();
    for (const auto& beat : beatGrid->beats) {
        if (beat.time_ms < startMs || beat.time_ms > endMs) continue;
        float x = graphX + ((beat.time_ms - startMs) / visibleDuration) * graphW;
        float y = graphY + graphH - ((beat.bpm() - minBpm) / (maxBpm - minBpm)) * graphH;
        ofVertex(x, y);
    }
    ofEndShape(false);
    ofFill();

    // BPM labels
    ofSetColor(150);
    ofDrawBitmapString(ofToString(maxBpm, 1) + " BPM", graphX - 60, graphY + 12);
    ofDrawBitmapString(ofToString(minBpm, 1) + " BPM", graphX - 60, graphY + graphH);
    ofDrawBitmapString("Tempo Graph", graphX, graphY - 10);

    // Average BPM
    float avgBpm = beatGrid->average_bpm();
    ofSetColor(255, 200, 80);
    ofDrawBitmapString("Avg: " + ofToString(avgBpm, 2) + " BPM", graphX + 120, graphY - 10);

    // Playhead
    if (playheadMs >= startMs && playheadMs <= endMs) {
        float playX = graphX + ((playheadMs - startMs) / visibleDuration) * graphW;
        ofSetColor(255, 255, 100);
        ofDrawLine(playX, graphY, playX, graphY + graphH);
    }
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    ofDrawBitmapString("Up/Down: Select | +/-: Zoom | Left/Right: Scroll | Space: Reset | 'q': Quit",
                       20, ofGetHeight() - 20);
}

std::string ofApp::formatTime(float ms) {
    int totalSecs = static_cast<int>(ms / 1000);
    int mins = totalSecs / 60;
    int secs = totalSecs % 60;
    return ofToString(mins) + ":" + (secs < 10 ? "0" : "") + ofToString(secs);
}
