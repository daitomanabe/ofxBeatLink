#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(15);
    ofSetWindowTitle("3-Band Waveform Viewer (CDJ-3000 Style)");

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
    playheadPosition += 0.0003f;
    if (playheadPosition > 1.0f) playheadPosition = 0;
}

void ofApp::draw() {
    drawHeader();

    if (!databaseLoaded) {
        ofSetColor(80);
        ofNoFill();
        ofDrawRectangle(100, 150, ofGetWidth() - 200, 150);
        ofFill();
        ofSetColor(120);
        ofDrawBitmapString("Drag and drop export.pdb here", ofGetWidth() / 2 - 120, 220);
        ofDrawBitmapString("Then drag PIONEER/USBANLZ folder", ofGetWidth() / 2 - 130, 250);
    } else {
        drawTrackList();
        if (showStacked) {
            drawStackedWaveform();
        } else {
            drawOverlayWaveform();
        }
        drawLegend();
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
        if (selectedTrackIndex >= trackScrollOffset + 15) trackScrollOffset = selectedTrackIndex - 14;
        selectTrack(selectedTrackIndex);
    } else if (key == 'm' || key == 'M') {
        showStacked = !showStacked;
    } else if (key == ' ') {
        playheadPosition = 0;
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
    auto result = cratedigger::Database::open(path);
    if (!result.has_value()) return;
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
    waveforms.reset();
    playheadPosition = 0;

    if (anlzLoaded) {
        auto* wf = database->get_waveforms_for_track(trackIds[index]);
        if (wf) waveforms = *wf;
    }
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("3-Band Waveform Viewer", 20, 25);

    if (selectedTrack) {
        ofSetColor(255, 200, 80);
        ofDrawBitmapString(selectedTrack->title, 280, 25);
        ofSetColor(150);
        ofDrawBitmapString("BPM: " + ofToString(selectedTrack->bpm(), 1), 280, 45);
    }

    ofSetColor(100, 200, 255);
    ofDrawBitmapString(showStacked ? "Mode: Stacked" : "Mode: Overlay", ofGetWidth() - 150, 25);
}

void ofApp::drawTrackList() {
    float x = 20, y = 70;
    ofSetColor(200);
    ofDrawBitmapString("== Tracks ==", x, y);
    y += 20;

    int endIndex = std::min(trackScrollOffset + 15, static_cast<int>(trackIds.size()));
    for (int i = trackScrollOffset; i < endIndex; ++i) {
        auto track = database->get_track(trackIds[i]);
        if (!track) continue;

        bool selected = (i == selectedTrackIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 5, y - 12, 200, 18);
        }
        ofSetColor(selected ? 255 : 150);
        std::string title = track->title;
        if (title.length() > 24) title = title.substr(0, 21) + "...";
        ofDrawBitmapString(title, x, y);
        y += 18;
    }
}

void ofApp::drawStackedWaveform() {
    float waveX = 250, waveY = 70;
    float waveW = ofGetWidth() - 280;
    float bandH = 120;
    float gap = 20;

    // Check for 3-band waveform
    cratedigger::WaveformData* waveData = nullptr;
    if (waveforms && waveforms->detail &&
        waveforms->detail->style == cratedigger::WaveformStyle::ThreeBand) {
        waveData = &(*waveforms->detail);
    }

    std::vector<std::pair<std::string, ofColor>> bands = {
        {"HIGH", ofColor(100, 200, 255)},  // Blue
        {"MID", ofColor(100, 255, 100)},   // Green
        {"LOW", ofColor(255, 100, 100)}    // Red
    };

    for (int b = 0; b < 3; ++b) {
        float y = waveY + b * (bandH + gap);

        ofSetColor(200);
        ofDrawBitmapString(bands[b].first, waveX - 50, y + bandH / 2 + 5);

        ofSetColor(25);
        ofDrawRectangle(waveX, y, waveW, bandH);

        if (!waveData) {
            ofSetColor(100);
            ofDrawBitmapString(anlzLoaded ? "No 3-band data" : "Load ANLZ", waveX + 10, y + bandH / 2);
            continue;
        }

        float barWidth = waveW / waveData->size();
        float centerY = y + bandH / 2;

        ofSetColor(bands[b].second);
        for (size_t i = 0; i < waveData->size(); ++i) {
            auto [low, mid, high] = waveData->bands_at(i);
            uint8_t val = (b == 0) ? high : (b == 1) ? mid : low;
            float h = (val / 31.0f) * (bandH / 2 - 5);
            float x = waveX + i * barWidth;
            ofDrawRectangle(x, centerY - h, std::max(1.0f, barWidth), h * 2);
        }

        // Playhead
        float playX = waveX + playheadPosition * waveW;
        ofSetColor(255, 255, 100);
        ofDrawLine(playX, y, playX, y + bandH);
    }
}

void ofApp::drawOverlayWaveform() {
    float waveX = 250, waveY = 100;
    float waveW = ofGetWidth() - 280;
    float waveH = 350;

    ofSetColor(200);
    ofDrawBitmapString("3-Band Overlay", waveX, waveY - 10);

    ofSetColor(25);
    ofDrawRectangle(waveX, waveY, waveW, waveH);

    cratedigger::WaveformData* waveData = nullptr;
    if (waveforms && waveforms->detail &&
        waveforms->detail->style == cratedigger::WaveformStyle::ThreeBand) {
        waveData = &(*waveforms->detail);
    }

    if (!waveData) {
        ofSetColor(100);
        ofDrawBitmapString(anlzLoaded ? "No 3-band waveform data" : "Load ANLZ folder", waveX + 10, waveY + waveH / 2);
        return;
    }

    float barWidth = waveW / waveData->size();
    float centerY = waveY + waveH / 2;

    // Draw in order: Low (back), Mid, High (front) with transparency
    std::vector<std::tuple<int, ofColor, int>> bandOrder = {
        {2, ofColor(255, 80, 80, 150), 0},   // Low - red
        {1, ofColor(80, 255, 80, 180), 1},   // Mid - green
        {0, ofColor(80, 180, 255, 200), 2}   // High - blue
    };

    for (auto& [bandIdx, color, _] : bandOrder) {
        ofSetColor(color);
        for (size_t i = 0; i < waveData->size(); ++i) {
            auto [low, mid, high] = waveData->bands_at(i);
            uint8_t val = (bandIdx == 0) ? high : (bandIdx == 1) ? mid : low;
            float h = (val / 31.0f) * (waveH / 2 - 10);
            float x = waveX + i * barWidth;
            ofDrawRectangle(x, centerY - h, std::max(1.0f, barWidth), h * 2);
        }
    }

    // Center line
    ofSetColor(80);
    ofDrawLine(waveX, centerY, waveX + waveW, centerY);

    // Playhead
    float playX = waveX + playheadPosition * waveW;
    ofSetColor(255, 255, 100);
    ofDrawLine(playX, waveY, playX, waveY + waveH);
}

void ofApp::drawLegend() {
    float x = 250, y = ofGetHeight() - 80;

    ofSetColor(200);
    ofDrawBitmapString("Legend:", x, y);

    ofSetColor(255, 100, 100);
    ofDrawRectangle(x + 70, y - 10, 15, 12);
    ofSetColor(150);
    ofDrawBitmapString("Low", x + 90, y);

    ofSetColor(100, 255, 100);
    ofDrawRectangle(x + 130, y - 10, 15, 12);
    ofSetColor(150);
    ofDrawBitmapString("Mid", x + 150, y);

    ofSetColor(100, 200, 255);
    ofDrawRectangle(x + 190, y - 10, 15, 12);
    ofSetColor(150);
    ofDrawBitmapString("High", x + 210, y);

    if (waveforms && waveforms->detail) {
        ofSetColor(100);
        ofDrawBitmapString("Style: " + std::string(cratedigger::waveform_style_to_string(waveforms->detail->style)),
                          x + 280, y);
    }
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    ofDrawBitmapString("Up/Down: Select | 'm': Toggle mode | Space: Reset | 'q': Quit", 20, ofGetHeight() - 20);
}
