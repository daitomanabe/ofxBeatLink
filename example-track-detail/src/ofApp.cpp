#include "ofApp.h"
#include <algorithm>
#include <filesystem>

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("ofxBeatLink - Track Detail Viewer");
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

    drawTrackList();
    drawTrackDetails();
    drawCuePoints();
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - Track Detail Viewer", 30, 30);

    if (databaseLoaded) {
        ofSetColor(120);
        ofDrawBitmapString("Tracks: " + ofToString(trackIds.size()), 30, 50);
        ofDrawBitmapString("[Up/Down] Navigate  [PageUp/Down] Scroll  [q] Quit",
                          ofGetWidth() - 420, 30);
    }
}

void ofApp::drawInstructions() {
    ofSetColor(100);
    ofDrawBitmapString("Drag and drop a PIONEER folder to load rekordbox database", 30, 100);
    ofDrawBitmapString("The folder should contain export.pdb file", 30, 120);
}

void ofApp::drawTrackList() {
    const float listX = 30;
    const float listY = 70;
    const float listWidth = 350;
    const float rowHeight = 18;

    // Background
    ofSetColor(20);
    ofDrawRectangle(listX - 5, listY - 5, listWidth, VISIBLE_TRACKS * rowHeight + 10);

    // Track list
    for (int i = 0; i < VISIBLE_TRACKS && (listScrollOffset + i) < static_cast<int>(trackIds.size()); ++i) {
        int idx = listScrollOffset + i;
        float y = listY + i * rowHeight;

        // Selection highlight
        if (idx == selectedIndex) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(listX - 3, y - 2, listWidth - 4, rowHeight - 2);
        }

        // Get track for display
        auto trackOpt = database->get_track(trackIds[idx]);
        if (!trackOpt) continue;

        // Track number
        ofSetColor(60);
        ofDrawBitmapString(ofToString(idx + 1, 0, 3, '0'), listX, y + 11);

        // Title
        ofSetColor(idx == selectedIndex ? 255 : 180);
        std::string title = trackOpt->title;
        if (title.length() > 35) title = title.substr(0, 32) + "...";
        ofDrawBitmapString(title, listX + 35, y + 11);
    }

    // Scrollbar
    if (trackIds.size() > static_cast<size_t>(VISIBLE_TRACKS)) {
        float scrollbarHeight = VISIBLE_TRACKS * rowHeight;
        float thumbHeight = scrollbarHeight * VISIBLE_TRACKS / trackIds.size();
        float thumbY = listY + (scrollbarHeight - thumbHeight) * listScrollOffset / (trackIds.size() - VISIBLE_TRACKS);

        ofSetColor(40);
        ofDrawRectangle(listX + listWidth - 8, listY, 6, scrollbarHeight);
        ofSetColor(80);
        ofDrawRectangle(listX + listWidth - 8, thumbY, 6, thumbHeight);
    }
}

void ofApp::drawTrackDetails() {
    if (!currentTrack.has_value()) return;

    const float detailX = 400;
    const float detailY = 70;
    const float detailWidth = 400;
    const float lineHeight = 20;
    float y = detailY;

    // Background
    ofSetColor(30);
    ofDrawRectangle(detailX - 10, detailY - 10, detailWidth, 350);

    // Title
    ofSetColor(255);
    ofDrawBitmapString("TRACK INFO", detailX, y);
    y += 25;

    // Title
    ofSetColor(100);
    ofDrawBitmapString("Title:", detailX, y);
    ofSetColor(255);
    ofDrawBitmapString(currentTrack->title, detailX + 80, y);
    y += lineHeight;

    // Artist
    ofSetColor(100);
    ofDrawBitmapString("Artist:", detailX, y);
    ofSetColor(200);
    ofDrawBitmapString(artistName.empty() ? "-" : artistName, detailX + 80, y);
    y += lineHeight;

    // Album
    ofSetColor(100);
    ofDrawBitmapString("Album:", detailX, y);
    ofSetColor(200);
    ofDrawBitmapString(albumName.empty() ? "-" : albumName, detailX + 80, y);
    y += lineHeight;

    // Genre
    ofSetColor(100);
    ofDrawBitmapString("Genre:", detailX, y);
    ofSetColor(200);
    ofDrawBitmapString(genreName.empty() ? "-" : genreName, detailX + 80, y);
    y += lineHeight;

    // Label
    ofSetColor(100);
    ofDrawBitmapString("Label:", detailX, y);
    ofSetColor(200);
    ofDrawBitmapString(labelName.empty() ? "-" : labelName, detailX + 80, y);
    y += lineHeight;

    // Key
    ofSetColor(100);
    ofDrawBitmapString("Key:", detailX, y);
    ofSetColor(100, 200, 255);
    ofDrawBitmapString(keyName.empty() ? "-" : keyName, detailX + 80, y);
    y += lineHeight;

    // Separator
    y += 10;
    ofSetColor(50);
    ofDrawLine(detailX, y, detailX + detailWidth - 20, y);
    y += 15;

    // BPM
    ofSetColor(100);
    ofDrawBitmapString("BPM:", detailX, y);
    ofSetColor(80, 255, 120);
    ofDrawBitmapString(ofToString(currentTrack->bpm(), 2), detailX + 80, y);
    y += lineHeight;

    // Duration
    ofSetColor(100);
    ofDrawBitmapString("Duration:", detailX, y);
    ofSetColor(200);
    ofDrawBitmapString(formatDuration(currentTrack->duration_seconds), detailX + 80, y);
    y += lineHeight;

    // Year
    ofSetColor(100);
    ofDrawBitmapString("Year:", detailX, y);
    ofSetColor(200);
    ofDrawBitmapString(currentTrack->year > 0 ? ofToString(currentTrack->year) : "-", detailX + 80, y);
    y += lineHeight;

    // Rating
    ofSetColor(100);
    ofDrawBitmapString("Rating:", detailX, y);
    ofSetColor(255, 200, 80);
    std::string stars = "";
    for (int i = 0; i < currentTrack->rating; ++i) stars += "*";
    ofDrawBitmapString(stars.empty() ? "-" : stars, detailX + 80, y);
    y += lineHeight;

    // Color
    if (!colorName.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("Color:", detailX, y);
        ofSetColor(200);
        ofDrawBitmapString(colorName, detailX + 80, y);
        y += lineHeight;
    }

    // Separator
    y += 10;
    ofSetColor(50);
    ofDrawLine(detailX, y, detailX + detailWidth - 20, y);
    y += 15;

    // Technical info
    ofSetColor(100);
    ofDrawBitmapString("Bitrate:", detailX, y);
    ofSetColor(150);
    ofDrawBitmapString(ofToString(currentTrack->bitrate) + " kbps", detailX + 80, y);
    y += lineHeight;

    ofSetColor(100);
    ofDrawBitmapString("Sample:", detailX, y);
    ofSetColor(150);
    ofDrawBitmapString(ofToString(currentTrack->sample_rate) + " Hz", detailX + 80, y);
    y += lineHeight;

    ofSetColor(100);
    ofDrawBitmapString("Size:", detailX, y);
    ofSetColor(150);
    ofDrawBitmapString(formatFileSize(currentTrack->file_size), detailX + 80, y);
    y += lineHeight;

    // Comment
    if (!currentTrack->comment.empty()) {
        y += 10;
        ofSetColor(100);
        ofDrawBitmapString("Comment:", detailX, y);
        y += lineHeight;
        ofSetColor(150);
        std::string comment = currentTrack->comment;
        if (comment.length() > 45) comment = comment.substr(0, 42) + "...";
        ofDrawBitmapString(comment, detailX + 10, y);
    }
}

void ofApp::drawCuePoints() {
    const float cueX = 830;
    const float cueY = 70;
    const float cueWidth = 350;
    const float lineHeight = 22;

    // Background
    ofSetColor(30);
    ofDrawRectangle(cueX - 10, cueY - 10, cueWidth, 350);

    ofSetColor(255);
    ofDrawBitmapString("CUE POINTS (" + ofToString(cuePoints.size()) + ")", cueX, cueY);

    if (cuePoints.empty()) {
        ofSetColor(60);
        ofDrawBitmapString("No cue points loaded", cueX, cueY + 30);
        ofDrawBitmapString("Load ANLZ files to see cue points", cueX, cueY + 50);
        return;
    }

    float y = cueY + 30;

    for (size_t i = 0; i < cuePoints.size() && i < 12; ++i) {
        const auto& cue = cuePoints[i];

        // Type indicator
        ofColor typeColor = getCuePointColor(cue.color_id);
        ofSetColor(typeColor);

        std::string typeStr;
        if (cue.is_hot_cue()) {
            typeStr = "HOT " + ofToString(cue.hot_cue_number);
        } else if (cue.is_loop()) {
            typeStr = "LOOP";
        } else {
            typeStr = "MEM";
        }
        ofDrawBitmapString(typeStr, cueX, y);

        // Time
        ofSetColor(200);
        int mins = static_cast<int>(cue.time_seconds()) / 60;
        int secs = static_cast<int>(cue.time_seconds()) % 60;
        int ms = (cue.time_ms % 1000) / 10;
        std::string timeStr = ofToString(mins) + ":" +
                              ofToString(secs, 0, 2, '0') + "." +
                              ofToString(ms, 0, 2, '0');
        ofDrawBitmapString(timeStr, cueX + 60, y);

        // Loop duration
        if (cue.is_loop()) {
            ofSetColor(100, 180, 255);
            int loopMs = cue.loop_duration_ms();
            ofDrawBitmapString("(" + ofToString(loopMs) + "ms)", cueX + 130, y);
        }

        // Comment
        if (!cue.comment.empty()) {
            ofSetColor(150);
            std::string comment = cue.comment;
            if (comment.length() > 20) comment = comment.substr(0, 17) + "...";
            ofDrawBitmapString(comment, cueX + 200, y);
        }

        y += lineHeight;
    }

    if (cuePoints.size() > 12) {
        ofSetColor(80);
        ofDrawBitmapString("... and " + ofToString(cuePoints.size() - 12) + " more", cueX, y);
    }
}

void ofApp::keyPressed(int key) {
    if (key == 'q' || key == 'Q') {
        ofExit();
        return;
    }

    if (!databaseLoaded || trackIds.empty()) return;

    if (key == OF_KEY_UP) {
        selectTrack(selectedIndex - 1);
    } else if (key == OF_KEY_DOWN) {
        selectTrack(selectedIndex + 1);
    } else if (key == OF_KEY_PAGE_UP) {
        selectTrack(selectedIndex - VISIBLE_TRACKS);
    } else if (key == OF_KEY_PAGE_DOWN) {
        selectTrack(selectedIndex + VISIBLE_TRACKS);
    } else if (key == OF_KEY_HOME) {
        selectTrack(0);
    } else if (key == OF_KEY_END) {
        selectTrack(static_cast<int>(trackIds.size()) - 1);
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
    trackIds.clear();
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

    // Load ANLZ files if available
    if (fs::exists(anlzPath)) {
        database->load_cue_points(anlzPath);
    }

    databaseLoaded = true;
    trackIds = database->all_track_ids();

    if (!trackIds.empty()) {
        selectTrack(0);
    }
}

void ofApp::selectTrack(int index) {
    if (trackIds.empty()) return;

    selectedIndex = std::max(0, std::min(index, static_cast<int>(trackIds.size()) - 1));

    // Adjust scroll
    if (selectedIndex < listScrollOffset) {
        listScrollOffset = selectedIndex;
    } else if (selectedIndex >= listScrollOffset + VISIBLE_TRACKS) {
        listScrollOffset = selectedIndex - VISIBLE_TRACKS + 1;
    }

    loadTrackDetails(trackIds[selectedIndex]);
}

void ofApp::loadTrackDetails(cratedigger::TrackId id) {
    currentTrack = database->get_track(id);
    if (!currentTrack.has_value()) return;

    // Load related data
    if (auto artist = database->get_artist(currentTrack->artist_id)) {
        artistName = artist->name;
    } else {
        artistName.clear();
    }

    if (auto album = database->get_album(currentTrack->album_id)) {
        albumName = album->name;
    } else {
        albumName.clear();
    }

    if (auto genre = database->get_genre(currentTrack->genre_id)) {
        genreName = genre->name;
    } else {
        genreName.clear();
    }

    if (auto label = database->get_label(currentTrack->label_id)) {
        labelName = label->name;
    } else {
        labelName.clear();
    }

    if (auto key = database->get_key(currentTrack->key_id)) {
        keyName = key->name;
    } else {
        keyName.clear();
    }

    if (auto color = database->get_color(currentTrack->color_id)) {
        colorName = color->name;
    } else {
        colorName.clear();
    }

    // Load cue points
    cuePoints = database->get_cue_points_for_track(id);
}

std::string ofApp::formatDuration(uint32_t seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return ofToString(mins) + ":" + ofToString(secs, 0, 2, '0');
}

std::string ofApp::formatFileSize(uint32_t bytes) {
    if (bytes < 1024) return ofToString(bytes) + " B";
    if (bytes < 1024 * 1024) return ofToString(bytes / 1024.0f, 1) + " KB";
    return ofToString(bytes / (1024.0f * 1024.0f), 1) + " MB";
}

ofColor ofApp::getCuePointColor(uint8_t colorId) {
    switch (colorId) {
        case 0: return ofColor(255, 255, 255);  // None/White
        case 1: return ofColor(255, 80, 80);    // Red
        case 2: return ofColor(255, 180, 80);   // Orange
        case 3: return ofColor(255, 255, 80);   // Yellow
        case 4: return ofColor(80, 255, 80);    // Green
        case 5: return ofColor(80, 255, 255);   // Aqua
        case 6: return ofColor(80, 80, 255);    // Blue
        case 7: return ofColor(180, 80, 255);   // Purple
        case 8: return ofColor(255, 80, 180);   // Pink
        default: return ofColor(150);
    }
}
