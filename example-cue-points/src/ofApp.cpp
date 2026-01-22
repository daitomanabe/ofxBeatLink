#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("Cue Points Viewer");

    // rekordbox hot cue colors
    hotCueColors = {
        ofColor(40),              // 0: None/Default (gray)
        ofColor(255, 50, 50),     // 1: Red
        ofColor(255, 150, 50),    // 2: Orange
        ofColor(255, 255, 50),    // 3: Yellow
        ofColor(50, 255, 50),     // 4: Green
        ofColor(50, 200, 255),    // 5: Aqua
        ofColor(100, 100, 255),   // 6: Blue
        ofColor(200, 100, 255),   // 7: Purple
        ofColor(255, 100, 200)    // 8: Pink
    };

    // Try loading from data folder
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
    // Animate playhead for demonstration
    if (selectedTrack && selectedTrack->duration_seconds > 0) {
        playheadPosition += 0.0002f;
        if (playheadPosition > 1.0f) playheadPosition = 0.0f;
    }
}

void ofApp::draw() {
    drawHeader();

    if (!databaseLoaded) {
        ofSetColor(80);
        ofNoFill();
        ofSetLineWidth(2);
        ofDrawRectangle(100, 150, ofGetWidth() - 200, 200);
        ofFill();
        ofSetLineWidth(1);

        ofSetColor(120);
        ofDrawBitmapString("Drag and drop export.pdb here", ofGetWidth() / 2 - 120, 250);
        ofDrawBitmapString("Then drag PIONEER/USBANLZ folder for cue data", ofGetWidth() / 2 - 170, 280);
    } else {
        drawTrackList();
        drawCuePointList();
        drawTimeline();
        drawHotCueGrid();
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
        if (selectedTrackIndex < trackScrollOffset) {
            trackScrollOffset = selectedTrackIndex;
        }
        selectTrack(selectedTrackIndex);
    } else if (key == OF_KEY_DOWN) {
        selectedTrackIndex = std::min(static_cast<int>(trackIds.size()) - 1, selectedTrackIndex + 1);
        if (selectedTrackIndex >= trackScrollOffset + visibleTrackRows) {
            trackScrollOffset = selectedTrackIndex - visibleTrackRows + 1;
        }
        selectTrack(selectedTrackIndex);
    } else if (key == OF_KEY_PAGE_UP) {
        selectedTrackIndex = std::max(0, selectedTrackIndex - visibleTrackRows);
        trackScrollOffset = std::max(0, trackScrollOffset - visibleTrackRows);
        selectTrack(selectedTrackIndex);
    } else if (key == OF_KEY_PAGE_DOWN) {
        selectedTrackIndex = std::min(static_cast<int>(trackIds.size()) - 1, selectedTrackIndex + visibleTrackRows);
        trackScrollOffset = std::min(static_cast<int>(trackIds.size()) - visibleTrackRows, trackScrollOffset + visibleTrackRows);
        if (trackScrollOffset < 0) trackScrollOffset = 0;
        selectTrack(selectedTrackIndex);
    } else if (key == ' ') {
        // Reset playhead
        playheadPosition = 0;
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.empty()) return;

    std::string path = dragInfo.files[0];

    // Check if it's a .pdb file
    if (path.find(".pdb") != std::string::npos) {
        loadDatabase(path);
    }
    // Check if it's a directory (ANLZ folder)
    else if (ofDirectory::doesDirectoryExist(path)) {
        loadAnlzFolder(path);
    }
}

void ofApp::loadDatabase(const std::string& path) {
    errorMessage.clear();
    databaseLoaded = false;

    auto result = cratedigger::Database::open(path);
    if (!result.has_value()) {
        errorMessage = result.error().message;
        ofLogError("ofApp") << "Failed to open database: " << errorMessage;
        return;
    }

    database = std::make_unique<cratedigger::Database>(std::move(result.value()));
    databasePath = path;
    databaseLoaded = true;

    trackIds = database->all_track_ids();
    selectedTrackIndex = 0;
    trackScrollOffset = 0;

    if (!trackIds.empty()) {
        selectTrack(0);
    }

    ofLogNotice("ofApp") << "Loaded database with " << trackIds.size() << " tracks";
}

void ofApp::loadAnlzFolder(const std::string& path) {
    if (!database) {
        errorMessage = "Load database first";
        return;
    }

    database->load_cue_points(path);
    anlzPath = path;
    anlzLoaded = true;

    // Refresh current track's cue points
    if (selectedTrackIndex < static_cast<int>(trackIds.size())) {
        selectTrack(selectedTrackIndex);
    }

    ofLogNotice("ofApp") << "Loaded ANLZ data from " << path;
}

void ofApp::selectTrack(int index) {
    if (index < 0 || index >= static_cast<int>(trackIds.size())) return;

    selectedTrack = database->get_track(trackIds[index]);
    cuePoints.clear();
    playheadPosition = 0;

    if (database && anlzLoaded) {
        cuePoints = database->get_cue_points_for_track(trackIds[index]);
    }
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("Cue Points Viewer", 20, 25);

    float y = 45;
    if (databaseLoaded) {
        ofSetColor(100, 200, 100);
        ofDrawBitmapString("DB: " + databasePath, 20, y);
        y += 18;
    }
    if (anlzLoaded) {
        ofSetColor(100, 200, 255);
        ofDrawBitmapString("ANLZ: " + anlzPath, 20, y);
    } else if (databaseLoaded) {
        ofSetColor(255, 200, 100);
        ofDrawBitmapString("ANLZ: Not loaded (drag USBANLZ folder)", 20, y);
    }
}

void ofApp::drawTrackList() {
    float x = 20;
    float y = 90;

    ofSetColor(200);
    ofDrawBitmapString("== Tracks ==", x, y);
    y += 20;

    int endIndex = std::min(trackScrollOffset + visibleTrackRows, static_cast<int>(trackIds.size()));
    for (int i = trackScrollOffset; i < endIndex; ++i) {
        auto track = database->get_track(trackIds[i]);
        if (!track) continue;

        bool selected = (i == selectedTrackIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 5, y - 12, 270, 18);
        }

        ofSetColor(selected ? 255 : 150);

        std::string title = track->title;
        if (title.length() > 30) title = title.substr(0, 27) + "...";
        ofDrawBitmapString(title, x, y);

        y += 18;
    }

    // Scroll indicator
    if (trackIds.size() > static_cast<size_t>(visibleTrackRows)) {
        ofSetColor(80);
        ofDrawBitmapString(ofToString(trackScrollOffset + 1) + "-" + ofToString(endIndex) +
                          "/" + ofToString(trackIds.size()), x, y + 10);
    }
}

void ofApp::drawCuePointList() {
    if (!selectedTrack) return;

    float x = timelineStartX;
    float y = 90;

    // Track info
    ofSetColor(255, 200, 80);
    ofDrawBitmapString("== " + selectedTrack->title + " ==", x, y);
    y += 20;

    ofSetColor(150);
    ofDrawBitmapString("Duration: " + formatTime(static_cast<float>(selectedTrack->duration_seconds)) +
                      "  |  BPM: " + ofToString(selectedTrack->bpm(), 1), x, y);
    y += 30;

    if (cuePoints.empty()) {
        ofSetColor(100);
        if (!anlzLoaded) {
            ofDrawBitmapString("Load ANLZ folder to see cue points", x, y);
        } else {
            ofDrawBitmapString("No cue points found for this track", x, y);
        }
        return;
    }

    // Cue point list
    ofSetColor(200);
    ofDrawBitmapString("Type", x, y);
    ofDrawBitmapString("Time", x + 100, y);
    ofDrawBitmapString("Hot#", x + 180, y);
    ofDrawBitmapString("Comment", x + 230, y);
    y += 5;

    ofSetColor(60);
    ofDrawLine(x, y, x + 400, y);
    y += 15;

    for (const auto& cue : cuePoints) {
        ofColor color = getCueColor(cue);
        ofSetColor(color);
        ofDrawRectangle(x - 15, y - 10, 10, 12);

        ofSetColor(180);
        ofDrawBitmapString(getCueTypeName(cue.type), x, y);
        ofDrawBitmapString(formatTime(cue.time_seconds()), x + 100, y);

        if (cue.is_hot_cue()) {
            ofSetColor(255, 220, 100);
            ofDrawBitmapString(ofToString(static_cast<int>(cue.hot_cue_number)), x + 180, y);
        }

        ofSetColor(120);
        std::string comment = cue.comment;
        if (comment.length() > 25) comment = comment.substr(0, 22) + "...";
        ofDrawBitmapString(comment, x + 230, y);

        // Show loop info
        if (cue.is_loop()) {
            ofSetColor(100, 200, 255);
            ofDrawBitmapString("Loop: " + formatTime(cue.loop_duration_ms() / 1000.0f), x + 400, y);
        }

        y += 18;
    }
}

void ofApp::drawTimeline() {
    if (!selectedTrack || selectedTrack->duration_seconds == 0) return;

    float duration = static_cast<float>(selectedTrack->duration_seconds);

    // Timeline background
    ofSetColor(40);
    ofDrawRectangle(timelineStartX, timelineY, timelineWidth, 60);

    // Draw cue markers
    for (const auto& cue : cuePoints) {
        drawCueOnTimeline(cue, duration);
    }

    // Playhead
    float playheadX = timelineStartX + playheadPosition * timelineWidth;
    ofSetColor(255, 100, 100);
    ofDrawLine(playheadX, timelineY - 5, playheadX, timelineY + 65);

    // Time markers
    ofSetColor(80);
    int numMarkers = 10;
    for (int i = 0; i <= numMarkers; ++i) {
        float markerX = timelineStartX + (static_cast<float>(i) / numMarkers) * timelineWidth;
        ofDrawLine(markerX, timelineY + 60, markerX, timelineY + 65);

        float time = (static_cast<float>(i) / numMarkers) * duration;
        ofDrawBitmapString(formatTime(time), markerX - 15, timelineY + 80);
    }

    // Current time
    ofSetColor(255);
    float currentTime = playheadPosition * duration;
    ofDrawBitmapString("Time: " + formatTime(currentTime), timelineStartX, timelineY - 15);
}

void ofApp::drawCueOnTimeline(const cratedigger::CuePoint& cue, float trackDuration) {
    if (trackDuration <= 0) return;

    float x = timelineStartX + (cue.time_seconds() / trackDuration) * timelineWidth;
    ofColor color = getCueColor(cue);

    if (cue.is_loop()) {
        // Draw loop region
        float endX = timelineStartX + ((cue.time_ms + cue.loop_duration_ms()) / 1000.0f / trackDuration) * timelineWidth;
        ofSetColor(color, 60);
        ofDrawRectangle(x, timelineY, endX - x, 60);
        ofSetColor(color);
        ofDrawLine(x, timelineY, x, timelineY + 60);
        ofDrawLine(endX, timelineY, endX, timelineY + 60);
    } else {
        // Draw cue marker
        ofSetColor(color);
        ofDrawLine(x, timelineY, x, timelineY + 60);

        // Triangle marker at top
        ofDrawTriangle(x - 5, timelineY, x + 5, timelineY, x, timelineY + 10);
    }

    // Hot cue number
    if (cue.is_hot_cue()) {
        ofSetColor(255);
        ofDrawBitmapString(ofToString(static_cast<int>(cue.hot_cue_number)), x - 3, timelineY + 25);
    }
}

void ofApp::drawHotCueGrid() {
    float x = timelineStartX;
    float y = timelineY + 100;

    ofSetColor(200);
    ofDrawBitmapString("Hot Cues:", x, y);
    y += 20;

    // Draw 8 hot cue slots
    for (int i = 1; i <= 8; ++i) {
        float slotX = x + (i - 1) * 100;

        // Find hot cue for this slot
        const cratedigger::CuePoint* hotCue = nullptr;
        for (const auto& cue : cuePoints) {
            if (cue.hot_cue_number == i) {
                hotCue = &cue;
                break;
            }
        }

        if (hotCue) {
            ofSetColor(getCueColor(*hotCue));
            ofDrawRectangle(slotX, y, 80, 40);
            ofSetColor(255);
            ofDrawBitmapString(ofToString(i), slotX + 5, y + 15);
            ofDrawBitmapString(formatTime(hotCue->time_seconds()), slotX + 5, y + 32);
        } else {
            ofSetColor(50);
            ofNoFill();
            ofDrawRectangle(slotX, y, 80, 40);
            ofFill();
            ofSetColor(80);
            ofDrawBitmapString(ofToString(i), slotX + 35, y + 25);
        }
    }
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    std::string instr = "Up/Down: Select track | Space: Reset playhead | 'q': Quit";
    ofDrawBitmapString(instr, 20, ofGetHeight() - 20);
}

std::string ofApp::formatTime(float seconds) {
    if (seconds < 0) return "--:--";
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - static_cast<int>(seconds)) * 100);
    return ofToString(mins) + ":" + (secs < 10 ? "0" : "") + ofToString(secs) +
           "." + (ms < 10 ? "0" : "") + ofToString(ms);
}

ofColor ofApp::getCueColor(const cratedigger::CuePoint& cue) {
    if (cue.is_hot_cue() && cue.color_id < hotCueColors.size()) {
        return hotCueColors[cue.color_id];
    }
    // Memory cue default color
    return ofColor(255, 100, 100);
}

std::string ofApp::getCueTypeName(cratedigger::CuePointType type) {
    switch (type) {
        case cratedigger::CuePointType::Cue: return "Cue";
        case cratedigger::CuePointType::FadeIn: return "Fade In";
        case cratedigger::CuePointType::FadeOut: return "Fade Out";
        case cratedigger::CuePointType::Load: return "Load";
        case cratedigger::CuePointType::Loop: return "Loop";
    }
    return "Unknown";
}
