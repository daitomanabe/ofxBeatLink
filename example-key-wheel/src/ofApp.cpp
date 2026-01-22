#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("Key Wheel (Circle of Fifths)");

    wheelCenterX = 350;
    wheelCenterY = 400;

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
        drawKeyWheel();
        drawCompatibleKeys();
        drawTrackList();
    }

    if (!errorMessage.empty()) {
        ofSetColor(255, 100, 100);
        ofDrawBitmapString("Error: " + errorMessage, 20, ofGetHeight() - 60);
    }

    drawInstructions();
}

void ofApp::keyPressed(int key) {
    if (!databaseLoaded) return;

    if (key == OF_KEY_UP && selectedKeyIndex >= 0) {
        selectedTrackIndex = std::max(0, selectedTrackIndex - 1);
        if (selectedTrackIndex < trackScrollOffset) trackScrollOffset = selectedTrackIndex;
    } else if (key == OF_KEY_DOWN && selectedKeyIndex >= 0) {
        int maxTracks = static_cast<int>(keys[selectedKeyIndex].tracks.size());
        selectedTrackIndex = std::min(maxTracks - 1, selectedTrackIndex + 1);
        if (selectedTrackIndex >= trackScrollOffset + 20) trackScrollOffset = selectedTrackIndex - 19;
    } else if (key == OF_KEY_LEFT) {
        if (selectedKeyIndex > 0) {
            selectedKeyIndex--;
            selectedTrackIndex = 0;
            trackScrollOffset = 0;
        }
    } else if (key == OF_KEY_RIGHT) {
        if (selectedKeyIndex < static_cast<int>(keys.size()) - 1) {
            selectedKeyIndex++;
            selectedTrackIndex = 0;
            trackScrollOffset = 0;
        }
    } else if (key == OF_KEY_ESCAPE) {
        selectedKeyIndex = -1;
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::mouseMoved(int x, int y) {
    hoveredKeyIndex = getKeyIndexAtPosition(x, y);
}

void ofApp::mousePressed(int x, int y, int button) {
    int clickedKey = getKeyIndexAtPosition(x, y);
    if (clickedKey >= 0) {
        selectedKeyIndex = clickedKey;
        selectedTrackIndex = 0;
        trackScrollOffset = 0;
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

    buildKeyData();

    ofLogNotice("ofApp") << "Loaded database with " << database->track_count() << " tracks";
}

void ofApp::buildKeyData() {
    keys.clear();

    // Camelot wheel mapping (standard DJ key notation)
    // Inner ring: Minor keys (A), Outer ring: Major keys (B)
    std::vector<std::tuple<std::string, int, bool>> camelotMapping = {
        // Minor keys (inner ring, counterclockwise from top)
        {"Abm", 1, true}, {"Ebm", 2, true}, {"Bbm", 3, true}, {"Fm", 4, true},
        {"Cm", 5, true}, {"Gm", 6, true}, {"Dm", 7, true}, {"Am", 8, true},
        {"Em", 9, true}, {"Bm", 10, true}, {"F#m", 11, true}, {"Dbm", 12, true},
        // Major keys (outer ring)
        {"B", 1, false}, {"Gb", 2, false}, {"Db", 3, false}, {"Ab", 4, false},
        {"Eb", 5, false}, {"Bb", 6, false}, {"F", 7, false}, {"C", 8, false},
        {"G", 9, false}, {"D", 10, false}, {"A", 11, false}, {"E", 12, false}
    };

    // Get all keys from database
    auto keyIds = database->all_key_ids();

    for (auto& id : keyIds) {
        auto keyRow = database->get_key(id);
        if (!keyRow) continue;

        KeyData kd;
        kd.id = id;
        kd.name = keyRow->name;
        kd.camelotNumber = 0;
        kd.isMinor = false;

        // Match to Camelot
        std::string keyName = keyRow->name;
        // Normalize key name
        for (auto& mapping : camelotMapping) {
            if (keyName.find(std::get<0>(mapping)) != std::string::npos ||
                std::get<0>(mapping).find(keyName) != std::string::npos) {
                kd.camelotNumber = std::get<1>(mapping);
                kd.isMinor = std::get<2>(mapping);
                break;
            }
        }

        // Find tracks with this key
        auto trackIds = database->all_track_ids();
        for (auto& trackId : trackIds) {
            auto track = database->get_track(trackId);
            if (track && track->key_id == id) {
                kd.tracks.push_back(trackId);
            }
        }

        kd.color = getKeyColor(kd.camelotNumber, kd.isMinor);
        keys.push_back(kd);
    }

    // Sort by Camelot number
    std::sort(keys.begin(), keys.end(), [](const KeyData& a, const KeyData& b) {
        if (a.isMinor != b.isMinor) return a.isMinor;  // Minor first
        return a.camelotNumber < b.camelotNumber;
    });
}

int ofApp::getKeyIndexAtPosition(float x, float y) {
    float dx = x - wheelCenterX;
    float dy = y - wheelCenterY;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < wheelRadius * 0.4f || dist > wheelRadius * 1.1f) return -1;

    float angle = std::atan2(dy, dx) + HALF_PI;  // 0 at top
    if (angle < 0) angle += TWO_PI;

    int segment = static_cast<int>((angle / TWO_PI) * 12);
    bool isInner = (dist < wheelRadius * 0.75f);

    // Find matching key
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i].camelotNumber == segment + 1 && keys[i].isMinor == isInner) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("Key Wheel (Circle of Fifths)", 20, 25);

    if (databaseLoaded) {
        ofSetColor(150);
        ofDrawBitmapString("Tracks: " + ofToString(database->track_count()) +
                          " | Keys: " + ofToString(keys.size()), 280, 25);
    }
}

void ofApp::drawKeyWheel() {
    // Draw segments
    for (size_t i = 0; i < keys.size(); ++i) {
        float innerR = keys[i].isMinor ? wheelRadius * 0.4f : wheelRadius * 0.75f;
        float outerR = keys[i].isMinor ? wheelRadius * 0.75f : wheelRadius;
        drawKeySegment(static_cast<int>(i), innerR, outerR);
    }

    // Center info
    ofSetColor(40);
    ofDrawCircle(wheelCenterX, wheelCenterY, wheelRadius * 0.38f);

    ofSetColor(200);
    ofDrawBitmapString("Camelot", wheelCenterX - 28, wheelCenterY - 10);
    ofDrawBitmapString("Wheel", wheelCenterX - 20, wheelCenterY + 10);

    // Legend
    float legendX = 20, legendY = ofGetHeight() - 120;
    ofSetColor(200);
    ofDrawBitmapString("Inner: Minor (m)", legendX, legendY);
    ofDrawBitmapString("Outer: Major", legendX, legendY + 20);
    ofSetColor(100);
    ofDrawBitmapString("Click to select", legendX, legendY + 50);
}

void ofApp::drawKeySegment(int index, float innerR, float outerR) {
    if (index < 0 || index >= static_cast<int>(keys.size())) return;

    const KeyData& kd = keys[index];
    int segment = kd.camelotNumber - 1;

    float startAngle = (segment / 12.0f) * TWO_PI - HALF_PI - (TWO_PI / 24.0f);
    float endAngle = startAngle + (TWO_PI / 12.0f);

    bool isHovered = (index == hoveredKeyIndex);
    bool isSelected = (index == selectedKeyIndex);

    // Draw segment
    if (isSelected) {
        ofSetColor(kd.color);
    } else if (isHovered) {
        ofSetColor(kd.color, 200);
    } else {
        ofSetColor(kd.color, 150);
    }

    // Draw arc segment as triangles
    int steps = 20;
    for (int i = 0; i < steps; ++i) {
        float a1 = startAngle + (i / static_cast<float>(steps)) * (endAngle - startAngle);
        float a2 = startAngle + ((i + 1) / static_cast<float>(steps)) * (endAngle - startAngle);

        ofDrawTriangle(
            wheelCenterX + std::cos(a1) * innerR, wheelCenterY + std::sin(a1) * innerR,
            wheelCenterX + std::cos(a1) * outerR, wheelCenterY + std::sin(a1) * outerR,
            wheelCenterX + std::cos(a2) * outerR, wheelCenterY + std::sin(a2) * outerR
        );
        ofDrawTriangle(
            wheelCenterX + std::cos(a1) * innerR, wheelCenterY + std::sin(a1) * innerR,
            wheelCenterX + std::cos(a2) * outerR, wheelCenterY + std::sin(a2) * outerR,
            wheelCenterX + std::cos(a2) * innerR, wheelCenterY + std::sin(a2) * innerR
        );
    }

    // Draw segment outline
    ofSetColor(20);
    ofNoFill();
    ofSetLineWidth(isSelected ? 3 : 1);
    ofBeginShape();
    for (int i = 0; i <= steps; ++i) {
        float a = startAngle + (i / static_cast<float>(steps)) * (endAngle - startAngle);
        ofVertex(wheelCenterX + std::cos(a) * outerR, wheelCenterY + std::sin(a) * outerR);
    }
    for (int i = steps; i >= 0; --i) {
        float a = startAngle + (i / static_cast<float>(steps)) * (endAngle - startAngle);
        ofVertex(wheelCenterX + std::cos(a) * innerR, wheelCenterY + std::sin(a) * innerR);
    }
    ofEndShape(true);
    ofFill();
    ofSetLineWidth(1);

    // Label
    float midAngle = (startAngle + endAngle) / 2;
    float labelR = (innerR + outerR) / 2;
    float labelX = wheelCenterX + std::cos(midAngle) * labelR;
    float labelY = wheelCenterY + std::sin(midAngle) * labelR;

    ofSetColor(isSelected || isHovered ? 255 : 200);
    std::string label = ofToString(kd.camelotNumber) + (kd.isMinor ? "A" : "B");
    ofDrawBitmapString(label, labelX - 8, labelY + 4);

    // Track count
    if (!kd.tracks.empty()) {
        ofSetColor(50);
        ofDrawBitmapString(ofToString(kd.tracks.size()), labelX - 8, labelY + 18);
    }
}

void ofApp::drawCompatibleKeys() {
    if (selectedKeyIndex < 0) return;

    const KeyData& kd = keys[selectedKeyIndex];

    float x = 20, y = 60;

    ofSetColor(kd.color);
    ofDrawBitmapString("Selected: " + kd.name + " (" + ofToString(kd.camelotNumber) +
                      (kd.isMinor ? "A" : "B") + ")", x, y);
    y += 25;

    ofSetColor(200);
    ofDrawBitmapString("Tracks in this key: " + ofToString(kd.tracks.size()), x, y);
    y += 30;

    // Compatible keys (Camelot wheel rules)
    ofSetColor(150);
    ofDrawBitmapString("Compatible keys:", x, y);
    y += 20;

    std::vector<std::pair<int, bool>> compatible;
    compatible.push_back({kd.camelotNumber, kd.isMinor});  // Same key
    compatible.push_back({kd.camelotNumber, !kd.isMinor});  // Relative major/minor
    compatible.push_back({(kd.camelotNumber % 12) + 1, kd.isMinor});  // +1
    compatible.push_back({((kd.camelotNumber - 2 + 12) % 12) + 1, kd.isMinor});  // -1

    for (auto& comp : compatible) {
        for (auto& k : keys) {
            if (k.camelotNumber == comp.first && k.isMinor == comp.second) {
                ofSetColor(k.color);
                ofDrawRectangle(x, y - 10, 12, 12);
                ofSetColor(180);
                ofDrawBitmapString(ofToString(k.camelotNumber) + (k.isMinor ? "A" : "B") +
                                  " - " + k.name + " (" + ofToString(k.tracks.size()) + ")",
                                  x + 20, y);
                y += 18;
                break;
            }
        }
    }
}

void ofApp::drawTrackList() {
    if (selectedKeyIndex < 0) return;

    const KeyData& kd = keys[selectedKeyIndex];

    float x = 700, y = 60;
    float panelW = ofGetWidth() - x - 20;

    ofSetColor(200);
    ofDrawBitmapString("== Tracks in " + kd.name + " ==", x, y);
    y += 25;

    if (kd.tracks.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No tracks in this key", x, y);
        return;
    }

    // Headers
    ofSetColor(150);
    ofDrawBitmapString("Title", x, y);
    ofDrawBitmapString("Artist", x + 250, y);
    ofDrawBitmapString("BPM", x + 420, y);
    y += 5;
    ofSetColor(60);
    ofDrawLine(x, y, x + panelW, y);
    y += 15;

    int endIndex = std::min(trackScrollOffset + 20, static_cast<int>(kd.tracks.size()));
    for (int i = trackScrollOffset; i < endIndex; ++i) {
        auto track = database->get_track(kd.tracks[i]);
        if (!track) continue;

        bool selected = (i == selectedTrackIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 3, y - 12, panelW, 18);
        }

        ofSetColor(selected ? 255 : 150);

        std::string title = track->title;
        if (title.length() > 28) title = title.substr(0, 25) + "...";
        ofDrawBitmapString(title, x, y);

        std::string artist = getArtistName(track->artist_id);
        if (artist.length() > 18) artist = artist.substr(0, 15) + "...";
        ofDrawBitmapString(artist, x + 250, y);

        if (track->bpm() > 0) {
            ofDrawBitmapString(ofToString(track->bpm(), 1), x + 420, y);
        }

        y += 18;
    }

    if (kd.tracks.size() > 20) {
        ofSetColor(80);
        ofDrawBitmapString(ofToString(trackScrollOffset + 1) + "-" + ofToString(endIndex) +
                          "/" + ofToString(kd.tracks.size()), x, y + 10);
    }
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    ofDrawBitmapString("Click wheel to select key | Up/Down: Navigate tracks | Esc: Deselect | 'q': Quit",
                       20, ofGetHeight() - 20);
}

std::string ofApp::getArtistName(cratedigger::ArtistId id) {
    if (id.value == 0) return "";
    if (auto artist = database->get_artist(id)) {
        return artist->name;
    }
    return "";
}

std::string ofApp::formatDuration(uint32_t seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return ofToString(mins) + ":" + (secs < 10 ? "0" : "") + ofToString(secs);
}

ofColor ofApp::getKeyColor(int camelotNum, bool isMinor) {
    // Colorful wheel based on position
    float hue = ((camelotNum - 1) / 12.0f) * 255;
    float sat = isMinor ? 180 : 220;
    float bri = isMinor ? 180 : 220;
    ofColor c;
    c.setHsb(hue, sat, bri);
    return c;
}
