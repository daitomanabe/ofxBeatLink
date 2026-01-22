#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("BPM Search");

    std::string defaultPdb = ofToDataPath("export.pdb");
    if (ofFile::doesFileExist(defaultPdb)) {
        loadDatabase(defaultPdb);
    }
}

void ofApp::update() {
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
    } else {
        drawBpmControls();
        drawHistogram();
        drawResults();
        drawTrackDetails();
    }

    if (!errorMessage.empty()) {
        ofSetColor(255, 100, 100);
        ofDrawBitmapString("Error: " + errorMessage, 20, ofGetHeight() - 60);
    }

    drawInstructions();
}

void ofApp::keyPressed(int key) {
    if (!databaseLoaded) return;

    if (key == OF_KEY_TAB) {
        editingMin = !editingMin;
    } else if (key == OF_KEY_UP) {
        if (editingMin) {
            bpmMin = std::min(bpmMax - 1, bpmMin + bpmStep);
        } else {
            bpmMax = std::min(300.0f, bpmMax + bpmStep);
        }
        performSearch();
    } else if (key == OF_KEY_DOWN) {
        if (editingMin) {
            bpmMin = std::max(20.0f, bpmMin - bpmStep);
        } else {
            bpmMax = std::max(bpmMin + 1, bpmMax - bpmStep);
        }
        performSearch();
    } else if (key == OF_KEY_LEFT) {
        selectedIndex = std::max(0, selectedIndex - 1);
        if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
    } else if (key == OF_KEY_RIGHT) {
        selectedIndex = std::min(static_cast<int>(searchResults.size()) - 1, selectedIndex + 1);
        if (selectedIndex >= scrollOffset + 20) scrollOffset = selectedIndex - 19;
    } else if (key == OF_KEY_PAGE_UP) {
        selectedIndex = std::max(0, selectedIndex - 20);
        scrollOffset = std::max(0, scrollOffset - 20);
    } else if (key == OF_KEY_PAGE_DOWN) {
        selectedIndex = std::min(static_cast<int>(searchResults.size()) - 1, selectedIndex + 20);
        scrollOffset = std::min(static_cast<int>(searchResults.size()) - 20, scrollOffset + 20);
        if (scrollOffset < 0) scrollOffset = 0;
    } else if (key == '+' || key == '=') {
        bpmStep = std::min(20.0f, bpmStep + 1.0f);
    } else if (key == '-') {
        bpmStep = std::max(1.0f, bpmStep - 1.0f);
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.empty()) return;
    loadDatabase(dragInfo.files[0]);
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

    buildHistogram();
    performSearch();

    ofLogNotice("ofApp") << "Loaded database with " << database->track_count() << " tracks";
}

void ofApp::performSearch() {
    searchResults = database->find_tracks_by_bpm_range(bpmMin, bpmMax);
    selectedIndex = 0;
    scrollOffset = 0;
}

void ofApp::buildHistogram() {
    bpmHistogram.clear();

    for (const auto& id : database->all_track_ids()) {
        auto track = database->get_track(id);
        if (track && track->bpm() > 0) {
            int bucket = static_cast<int>(track->bpm() / 5) * 5;  // 5 BPM buckets
            bpmHistogram[bucket]++;
        }
    }
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("BPM Search", 20, 25);

    if (databaseLoaded) {
        ofSetColor(150);
        ofDrawBitmapString("Total Tracks: " + ofToString(database->track_count()), 180, 25);
    }
}

void ofApp::drawBpmControls() {
    float x = 20, y = 60;

    ofSetColor(200);
    ofDrawBitmapString("BPM Range:", x, y);
    y += 25;

    // Min BPM
    ofSetColor(editingMin ? ofColor(100, 200, 255) : ofColor(150));
    ofDrawBitmapString("Min: ", x, y);
    ofSetColor(editingMin ? 255 : 180);
    if (editingMin) {
        ofNoFill();
        ofDrawRectangle(x + 45, y - 15, 80, 22);
        ofFill();
    }
    ofDrawBitmapString(ofToString(bpmMin, 1) + " BPM", x + 50, y);
    y += 30;

    // Max BPM
    ofSetColor(!editingMin ? ofColor(100, 200, 255) : ofColor(150));
    ofDrawBitmapString("Max: ", x, y);
    ofSetColor(!editingMin ? 255 : 180);
    if (!editingMin) {
        ofNoFill();
        ofDrawRectangle(x + 45, y - 15, 80, 22);
        ofFill();
    }
    ofDrawBitmapString(ofToString(bpmMax, 1) + " BPM", x + 50, y);
    y += 30;

    // Step
    ofSetColor(100);
    ofDrawBitmapString("Step: " + ofToString(bpmStep, 0) + " BPM (+/-)", x, y);
    y += 30;

    // Results count
    ofSetColor(255, 200, 80);
    ofDrawBitmapString("Found: " + ofToString(searchResults.size()) + " tracks", x, y);
}

void ofApp::drawHistogram() {
    float histX = 20, histY = 220;
    float histW = 350, histH = 200;

    ofSetColor(200);
    ofDrawBitmapString("BPM Distribution", histX, histY - 10);

    ofSetColor(35);
    ofDrawRectangle(histX, histY, histW, histH);

    if (bpmHistogram.empty()) return;

    // Find max count
    int maxCount = 0;
    for (auto& kv : bpmHistogram) {
        maxCount = std::max(maxCount, kv.second);
    }

    // Draw bars
    float barWidth = histW / ((300 - 60) / 5);  // 60-300 BPM range, 5 BPM buckets

    for (auto& kv : bpmHistogram) {
        int bucket = kv.first;
        int count = kv.second;

        if (bucket < 60 || bucket > 295) continue;

        float x = histX + ((bucket - 60) / 5.0f) * barWidth;
        float h = (static_cast<float>(count) / maxCount) * (histH - 20);

        // Highlight selected range
        bool inRange = (bucket >= bpmMin && bucket <= bpmMax);
        if (inRange) {
            ofSetColor(100, 200, 255, 200);
        } else {
            ofSetColor(80, 80, 80);
        }

        ofDrawRectangle(x, histY + histH - h - 10, barWidth - 2, h);
    }

    // Draw range indicator
    float rangeStartX = histX + ((bpmMin - 60) / 5.0f) * barWidth;
    float rangeEndX = histX + ((bpmMax - 60) / 5.0f) * barWidth;
    ofSetColor(255, 100, 100, 150);
    ofDrawRectangle(rangeStartX, histY, rangeEndX - rangeStartX, histH);

    // X-axis labels
    ofSetColor(100);
    for (int bpm = 60; bpm <= 300; bpm += 30) {
        float x = histX + ((bpm - 60) / 5.0f) * barWidth;
        ofDrawBitmapString(ofToString(bpm), x - 10, histY + histH + 15);
    }
}

void ofApp::drawResults() {
    float x = 400, y = 60;
    float panelW = ofGetWidth() - x - 20;

    ofSetColor(200);
    ofDrawBitmapString("== Search Results (" + ofToString(bpmMin, 0) + "-" +
                      ofToString(bpmMax, 0) + " BPM) ==", x, y);
    y += 25;

    // Column headers
    ofSetColor(150);
    ofDrawBitmapString("#", x, y);
    ofDrawBitmapString("Title", x + 40, y);
    ofDrawBitmapString("Artist", x + 340, y);
    ofDrawBitmapString("BPM", x + 540, y);
    ofDrawBitmapString("Duration", x + 600, y);
    y += 5;
    ofSetColor(60);
    ofDrawLine(x, y, x + panelW, y);
    y += 15;

    if (searchResults.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No tracks found in this BPM range", x, y + 20);
        return;
    }

    int endIndex = std::min(scrollOffset + 20, static_cast<int>(searchResults.size()));
    for (int i = scrollOffset; i < endIndex; ++i) {
        auto track = database->get_track(searchResults[i]);
        if (!track) continue;

        bool selected = (i == selectedIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 3, y - 12, panelW, 18);
        }

        ofSetColor(selected ? 255 : 150);
        ofDrawBitmapString(ofToString(i + 1), x, y);

        std::string title = track->title;
        if (title.length() > 35) title = title.substr(0, 32) + "...";
        ofDrawBitmapString(title, x + 40, y);

        std::string artist = getArtistName(track->artist_id);
        if (artist.length() > 22) artist = artist.substr(0, 19) + "...";
        ofDrawBitmapString(artist, x + 340, y);

        ofSetColor(selected ? ofColor(255, 200, 80) : ofColor(200, 150, 50));
        ofDrawBitmapString(ofToString(track->bpm(), 1), x + 540, y);

        ofSetColor(selected ? 255 : 150);
        ofDrawBitmapString(formatDuration(track->duration_seconds), x + 600, y);

        y += 18;
    }

    // Scroll indicator
    if (searchResults.size() > 20) {
        ofSetColor(80);
        ofDrawBitmapString(ofToString(scrollOffset + 1) + "-" + ofToString(endIndex) +
                          "/" + ofToString(searchResults.size()), x, ofGetHeight() - 85);
    }
}

void ofApp::drawTrackDetails() {
    if (searchResults.empty() || selectedIndex >= static_cast<int>(searchResults.size())) return;

    auto track = database->get_track(searchResults[selectedIndex]);
    if (!track) return;

    float x = 400, y = ofGetHeight() - 60;

    ofSetColor(100);
    ofDrawBitmapString("Selected: " + track->title + " - " + getArtistName(track->artist_id) +
                      " (" + ofToString(track->bpm(), 2) + " BPM)", x, y);
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    ofDrawBitmapString("Tab: Switch Min/Max | Up/Down: Adjust BPM | Left/Right: Navigate results | +/-: Step size | 'q': Quit",
                       20, ofGetHeight() - 20);
}

std::string ofApp::formatDuration(uint32_t seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return ofToString(mins) + ":" + (secs < 10 ? "0" : "") + ofToString(secs);
}

std::string ofApp::getArtistName(cratedigger::ArtistId id) {
    if (id.value == 0) return "";
    if (auto artist = database->get_artist(id)) {
        return artist->name;
    }
    return "";
}
