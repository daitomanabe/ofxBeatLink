#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("Tag Browser (exportExt.pdb)");

    std::string defaultPdb = ofToDataPath("export.pdb");
    if (ofFile::doesFileExist(defaultPdb)) {
        loadDatabase(defaultPdb);
    }

    std::string defaultExtPdb = ofToDataPath("exportExt.pdb");
    if (ofFile::doesFileExist(defaultExtPdb)) {
        loadExtDatabase(defaultExtPdb);
    }
}

void ofApp::update() {
}

void ofApp::draw() {
    drawHeader();

    if (!extDatabaseLoaded) {
        ofSetColor(80);
        ofNoFill();
        ofDrawRectangle(100, 150, ofGetWidth() - 200, 200);
        ofFill();
        ofSetColor(120);
        ofDrawBitmapString("Drag and drop exportExt.pdb here", ofGetWidth() / 2 - 130, 240);
        ofDrawBitmapString("(Also need export.pdb for track info)", ofGetWidth() / 2 - 145, 270);
    } else {
        drawCategoryPanel();
        drawTagPanel();
        drawTrackPanel();
    }

    if (!errorMessage.empty()) {
        ofSetColor(255, 100, 100);
        ofDrawBitmapString("Error: " + errorMessage, 20, ofGetHeight() - 60);
    }

    drawInstructions();
}

void ofApp::keyPressed(int key) {
    if (!extDatabaseLoaded) return;

    if (key == OF_KEY_TAB) {
        focusPanel = (focusPanel + 1) % 3;
    } else if (key == OF_KEY_LEFT) {
        focusPanel = std::max(0, focusPanel - 1);
    } else if (key == OF_KEY_RIGHT) {
        focusPanel = std::min(2, focusPanel + 1);
    } else if (key == OF_KEY_UP) {
        if (focusPanel == 0) {
            selectedCategoryIndex = std::max(0, selectedCategoryIndex - 1);
            selectCategory(selectedCategoryIndex);
        } else if (focusPanel == 1) {
            selectedTagIndex = std::max(0, selectedTagIndex - 1);
            selectTag(selectedTagIndex);
        } else {
            selectedTrackIndex = std::max(0, selectedTrackIndex - 1);
            if (selectedTrackIndex < trackScrollOffset) trackScrollOffset = selectedTrackIndex;
        }
    } else if (key == OF_KEY_DOWN) {
        if (focusPanel == 0) {
            selectedCategoryIndex = std::min(static_cast<int>(categoryIds.size()) - 1, selectedCategoryIndex + 1);
            selectCategory(selectedCategoryIndex);
        } else if (focusPanel == 1) {
            selectedTagIndex = std::min(static_cast<int>(tagsInCategory.size()) - 1, selectedTagIndex + 1);
            selectTag(selectedTagIndex);
        } else {
            selectedTrackIndex = std::min(static_cast<int>(taggedTracks.size()) - 1, selectedTrackIndex + 1);
            if (selectedTrackIndex >= trackScrollOffset + 20) trackScrollOffset = selectedTrackIndex - 19;
        }
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.empty()) return;
    std::string path = dragInfo.files[0];
    if (path.find("exportExt") != std::string::npos || path.find("Ext") != std::string::npos) {
        loadExtDatabase(path);
    } else if (path.find(".pdb") != std::string::npos) {
        loadDatabase(path);
    }
}

void ofApp::loadDatabase(const std::string& path) {
    auto result = cratedigger::Database::open(path);
    if (!result.has_value()) {
        errorMessage = "export.pdb: " + result.error().message;
        return;
    }
    database = std::make_unique<cratedigger::Database>(std::move(result.value()));
    databaseLoaded = true;
    ofLogNotice("ofApp") << "Loaded export.pdb";
}

void ofApp::loadExtDatabase(const std::string& path) {
    auto result = cratedigger::Database::open(path);
    if (!result.has_value()) {
        errorMessage = "exportExt.pdb: " + result.error().message;
        return;
    }
    extDatabase = std::make_unique<cratedigger::Database>(std::move(result.value()));
    extDatabaseLoaded = true;

    categoryIds = extDatabase->all_category_ids();
    selectedCategoryIndex = 0;

    if (!categoryIds.empty()) {
        selectCategory(0);
    }

    ofLogNotice("ofApp") << "Loaded exportExt.pdb with " << categoryIds.size() << " categories";
}

void ofApp::selectCategory(int index) {
    if (index < 0 || index >= static_cast<int>(categoryIds.size())) return;

    tagsInCategory = extDatabase->get_tags_in_category(categoryIds[index]);
    selectedTagIndex = 0;
    taggedTracks.clear();
    selectedTrackIndex = 0;
    trackScrollOffset = 0;

    if (!tagsInCategory.empty()) {
        selectTag(0);
    }
}

void ofApp::selectTag(int index) {
    if (index < 0 || index >= static_cast<int>(tagsInCategory.size())) return;

    taggedTracks = extDatabase->find_tracks_by_tag(tagsInCategory[index]);
    selectedTrackIndex = 0;
    trackScrollOffset = 0;
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("Tag Browser", 20, 25);

    if (extDatabaseLoaded) {
        ofSetColor(150);
        ofDrawBitmapString("Categories: " + ofToString(categoryIds.size()), 180, 25);
        ofDrawBitmapString("Total Tags: " + ofToString(extDatabase->tag_count()), 320, 25);
    }

    if (databaseLoaded) {
        ofSetColor(100, 200, 100);
        ofDrawBitmapString("[export.pdb loaded]", ofGetWidth() - 180, 25);
    } else {
        ofSetColor(255, 200, 100);
        ofDrawBitmapString("[export.pdb needed]", ofGetWidth() - 180, 25);
    }
}

void ofApp::drawCategoryPanel() {
    float x = 20, y = 60;
    float panelW = 200;

    ofSetColor(focusPanel == 0 ? ofColor(100, 180, 255) : ofColor(150));
    ofDrawBitmapString("== Categories ==", x, y);
    y += 25;

    ofSetColor(focusPanel == 0 ? 40 : 30);
    ofDrawRectangle(x - 5, y - 15, panelW, 400);

    if (categoryIds.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No categories", x, y + 20);
        return;
    }

    for (size_t i = 0; i < categoryIds.size() && i < 20; ++i) {
        auto cat = extDatabase->get_category(categoryIds[i]);
        if (!cat) continue;

        bool selected = (static_cast<int>(i) == selectedCategoryIndex);
        if (selected && focusPanel == 0) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 3, y - 12, panelW - 5, 18);
        }

        ofSetColor(selected ? 255 : 150);
        std::string name = cat->name;
        if (name.length() > 22) name = name.substr(0, 19) + "...";
        ofDrawBitmapString(name, x, y);

        y += 18;
    }
}

void ofApp::drawTagPanel() {
    float x = 240, y = 60;
    float panelW = 250;

    ofSetColor(focusPanel == 1 ? ofColor(100, 180, 255) : ofColor(150));
    std::string title = "== Tags";
    if (selectedCategoryIndex < static_cast<int>(categoryIds.size())) {
        if (auto cat = extDatabase->get_category(categoryIds[selectedCategoryIndex])) {
            title += " (" + cat->name + ")";
        }
    }
    title += " ==";
    ofDrawBitmapString(title, x, y);
    y += 25;

    ofSetColor(focusPanel == 1 ? 40 : 30);
    ofDrawRectangle(x - 5, y - 15, panelW, 400);

    if (tagsInCategory.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No tags in category", x, y + 20);
        return;
    }

    for (size_t i = 0; i < tagsInCategory.size() && i < 20; ++i) {
        auto tag = extDatabase->get_tag(tagsInCategory[i]);
        if (!tag) continue;

        bool selected = (static_cast<int>(i) == selectedTagIndex);
        if (selected && focusPanel == 1) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 3, y - 12, panelW - 5, 18);
        }

        ofSetColor(selected ? 255 : 150);
        std::string name = tag->name;
        if (name.length() > 28) name = name.substr(0, 25) + "...";
        ofDrawBitmapString(name, x, y);

        // Track count
        auto tracks = extDatabase->find_tracks_by_tag(tagsInCategory[i]);
        ofSetColor(100);
        ofDrawBitmapString("(" + ofToString(tracks.size()) + ")", x + 190, y);

        y += 18;
    }
}

void ofApp::drawTrackPanel() {
    float x = 510, y = 60;
    float panelW = ofGetWidth() - x - 20;

    ofSetColor(focusPanel == 2 ? ofColor(100, 180, 255) : ofColor(150));
    std::string title = "== Tracks";
    if (selectedTagIndex < static_cast<int>(tagsInCategory.size())) {
        if (auto tag = extDatabase->get_tag(tagsInCategory[selectedTagIndex])) {
            title += " tagged \"" + tag->name + "\"";
        }
    }
    title += " ==";
    ofDrawBitmapString(title, x, y);
    y += 25;

    ofSetColor(focusPanel == 2 ? 40 : 30);
    ofDrawRectangle(x - 5, y - 15, panelW, 400);

    if (taggedTracks.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No tracks with this tag", x, y + 20);
        return;
    }

    if (!databaseLoaded) {
        ofSetColor(255, 200, 100);
        ofDrawBitmapString("Load export.pdb to see track details", x, y + 20);

        // Show just IDs
        ofSetColor(100);
        y += 40;
        int endIdx = std::min(trackScrollOffset + 15, static_cast<int>(taggedTracks.size()));
        for (int i = trackScrollOffset; i < endIdx; ++i) {
            ofDrawBitmapString("Track ID: " + ofToString(taggedTracks[i].value), x, y);
            y += 18;
        }
        return;
    }

    // Column headers
    ofSetColor(200);
    ofDrawBitmapString("Title", x, y);
    ofDrawBitmapString("Artist", x + 300, y);
    ofDrawBitmapString("BPM", x + 500, y);
    y += 5;
    ofSetColor(60);
    ofDrawLine(x, y, x + panelW - 10, y);
    y += 15;

    int endIdx = std::min(trackScrollOffset + 20, static_cast<int>(taggedTracks.size()));
    for (int i = trackScrollOffset; i < endIdx; ++i) {
        auto track = database->get_track(taggedTracks[i]);
        if (!track) continue;

        bool selected = (i == selectedTrackIndex);
        if (selected && focusPanel == 2) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 3, y - 12, panelW - 5, 18);
        }

        ofSetColor(selected ? 255 : 150);

        std::string title = track->title;
        if (title.length() > 35) title = title.substr(0, 32) + "...";
        ofDrawBitmapString(title, x, y);

        if (auto artist = database->get_artist(track->artist_id)) {
            std::string artistName = artist->name;
            if (artistName.length() > 22) artistName = artistName.substr(0, 19) + "...";
            ofDrawBitmapString(artistName, x + 300, y);
        }

        if (track->bpm() > 0) {
            ofDrawBitmapString(ofToString(track->bpm(), 1), x + 500, y);
        }

        y += 18;
    }

    if (taggedTracks.size() > 20) {
        ofSetColor(80);
        ofDrawBitmapString(ofToString(trackScrollOffset + 1) + "-" + ofToString(endIdx) +
                          "/" + ofToString(taggedTracks.size()), x, y + 10);
    }
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    ofDrawBitmapString("Tab/Left/Right: Switch panel | Up/Down: Navigate | 'q': Quit", 20, ofGetHeight() - 20);
}

std::string ofApp::formatDuration(uint32_t seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return ofToString(mins) + ":" + (secs < 10 ? "0" : "") + ofToString(secs);
}
