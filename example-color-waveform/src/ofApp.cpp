#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(15);
    ofSetWindowTitle("Color Waveform Viewer");

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
    playheadPosition += 0.0005f;
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
        drawWaveform();
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
    ofDrawBitmapString("Color Waveform Viewer", 20, 25);

    if (selectedTrack) {
        ofSetColor(255, 200, 80);
        ofDrawBitmapString(selectedTrack->title, 250, 25);
        ofSetColor(150);
        ofDrawBitmapString("BPM: " + ofToString(selectedTrack->bpm(), 1), 250, 45);
    }
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

void ofApp::drawWaveform() {
    drawPreviewWaveform();
    drawDetailWaveform();
}

void ofApp::drawPreviewWaveform() {
    float waveX = 250, waveY = 70;
    float waveW = ofGetWidth() - 280;
    float waveH = 100;

    ofSetColor(200);
    ofDrawBitmapString("Preview Waveform", waveX, waveY - 5);

    ofSetColor(30);
    ofDrawRectangle(waveX, waveY, waveW, waveH);

    if (!waveforms || !waveforms->preview) {
        ofSetColor(100);
        ofDrawBitmapString(anlzLoaded ? "No preview waveform" : "Load ANLZ folder", waveX + 10, waveY + 50);
        return;
    }

    auto& preview = *waveforms->preview;
    float barWidth = waveW / preview.size();
    float centerY = waveY + waveH / 2;

    for (size_t i = 0; i < preview.size(); ++i) {
        float x = waveX + i * barWidth;
        uint8_t height = preview.height_at(i);
        float h = (height / 31.0f) * (waveH / 2 - 5);

        // Get color if RGB waveform
        if (preview.style == cratedigger::WaveformStyle::RGB) {
            uint32_t color = preview.color_at(i);
            ofSetColor((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        } else {
            ofSetColor(100, 180, 255);
        }

        ofDrawRectangle(x, centerY - h, std::max(1.0f, barWidth - 1), h * 2);
    }

    // Playhead
    float playX = waveX + playheadPosition * waveW;
    ofSetColor(255, 100, 100);
    ofDrawLine(playX, waveY, playX, waveY + waveH);
}

void ofApp::drawDetailWaveform() {
    float waveX = 250, waveY = 200;
    float waveW = ofGetWidth() - 280;
    float waveH = 150;

    ofSetColor(200);
    ofDrawBitmapString("Detail Waveform (Color)", waveX, waveY - 5);

    ofSetColor(30);
    ofDrawRectangle(waveX, waveY, waveW, waveH);

    if (!waveforms || !waveforms->detail) {
        ofSetColor(100);
        ofDrawBitmapString(anlzLoaded ? "No detail waveform" : "Load ANLZ folder", waveX + 10, waveY + 70);
        return;
    }

    auto& detail = *waveforms->detail;

    // Calculate visible range based on playhead
    float viewWidth = 0.1f;  // Show 10% of track at a time
    float viewStart = std::max(0.0f, playheadPosition - viewWidth / 2);
    float viewEnd = std::min(1.0f, viewStart + viewWidth);

    size_t startIdx = static_cast<size_t>(viewStart * detail.size());
    size_t endIdx = static_cast<size_t>(viewEnd * detail.size());
    size_t visibleCount = endIdx - startIdx;

    if (visibleCount == 0) return;

    float barWidth = waveW / visibleCount;
    float centerY = waveY + waveH / 2;

    for (size_t i = startIdx; i < endIdx; ++i) {
        float x = waveX + (i - startIdx) * barWidth;
        uint8_t height = detail.height_at(i);
        float h = (height / 31.0f) * (waveH / 2 - 10);

        if (detail.style == cratedigger::WaveformStyle::RGB) {
            uint32_t color = detail.color_at(i);
            ofSetColor((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        } else {
            ofSetColor(100, 200, 255);
        }

        ofDrawRectangle(x, centerY - h, std::max(1.0f, barWidth), h * 2);
    }

    // Center playhead
    ofSetColor(255, 255, 100);
    ofDrawLine(waveX + waveW / 2, waveY, waveX + waveW / 2, waveY + waveH);

    // Info
    ofSetColor(150);
    ofDrawBitmapString("Style: " + std::string(cratedigger::waveform_style_to_string(detail.style)),
                       waveX, waveY + waveH + 20);
    ofDrawBitmapString("Entries: " + ofToString(detail.size()), waveX + 150, waveY + waveH + 20);
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    ofDrawBitmapString("Up/Down: Select track | Space: Reset | 'q': Quit", 20, ofGetHeight() - 20);
}
