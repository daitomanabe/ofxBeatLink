#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("Song Structure Viewer");

    initPhraseColors();

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
    // Animate playhead
    if (songStructure && songStructure->end_beat > 0) {
        playheadBeat += 0.5f;
        if (playheadBeat > songStructure->end_beat) {
            playheadBeat = 0;
        }
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
        ofDrawBitmapString("Then drag PIONEER/USBANLZ folder for song structure data", ofGetWidth() / 2 - 200, 280);
    } else {
        drawTrackList();
        drawSongStructure();
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
    } else if (key == ' ') {
        playheadBeat = 0;
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
    databaseLoaded = false;

    auto result = cratedigger::Database::open(path);
    if (!result.has_value()) {
        errorMessage = result.error().message;
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

    if (selectedTrackIndex < static_cast<int>(trackIds.size())) {
        selectTrack(selectedTrackIndex);
    }

    ofLogNotice("ofApp") << "Loaded ANLZ data from " << path;
}

void ofApp::selectTrack(int index) {
    if (index < 0 || index >= static_cast<int>(trackIds.size())) return;

    selectedTrack = database->get_track(trackIds[index]);
    songStructure.reset();
    beatGrid.reset();
    playheadBeat = 0;

    if (database && anlzLoaded) {
        auto* structure = database->get_song_structure_for_track(trackIds[index]);
        if (structure) {
            songStructure = *structure;
        }

        auto* grid = database->get_beat_grid_for_track(trackIds[index]);
        if (grid) {
            beatGrid = *grid;
        }
    }
}

void ofApp::initPhraseColors() {
    // Phrase type colors
    phraseColors["Intro"] = ofColor(100, 200, 255);     // Light blue
    phraseColors["Outro"] = ofColor(100, 150, 200);     // Darker blue
    phraseColors["Verse"] = ofColor(100, 255, 150);     // Green
    phraseColors["Verse 1"] = ofColor(100, 255, 150);
    phraseColors["Verse 2"] = ofColor(80, 230, 130);
    phraseColors["Verse 3"] = ofColor(60, 210, 110);
    phraseColors["Verse 4"] = ofColor(50, 190, 100);
    phraseColors["Verse 5"] = ofColor(40, 170, 90);
    phraseColors["Verse 6"] = ofColor(30, 150, 80);
    phraseColors["Chorus"] = ofColor(255, 150, 100);    // Orange
    phraseColors["Bridge"] = ofColor(200, 150, 255);    // Purple
    phraseColors["Up"] = ofColor(255, 200, 100);        // Yellow-orange
    phraseColors["Down"] = ofColor(150, 200, 100);      // Yellow-green
    phraseColors["Unknown"] = ofColor(100, 100, 100);   // Gray
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("Song Structure Viewer", 20, 25);

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
            ofDrawRectangle(x - 5, y - 12, 260, 18);
        }

        ofSetColor(selected ? 255 : 150);
        std::string title = track->title;
        if (title.length() > 28) title = title.substr(0, 25) + "...";
        ofDrawBitmapString(title, x, y);

        y += 18;
    }

    if (trackIds.size() > static_cast<size_t>(visibleTrackRows)) {
        ofSetColor(80);
        ofDrawBitmapString(ofToString(trackScrollOffset + 1) + "-" + ofToString(endIndex) +
                          "/" + ofToString(trackIds.size()), x, y + 10);
    }
}

void ofApp::drawSongStructure() {
    if (!selectedTrack) return;

    float x = timelineStartX;
    float y = 90;

    // Track info
    ofSetColor(255, 200, 80);
    ofDrawBitmapString("== " + selectedTrack->title + " ==", x, y);
    y += 20;

    ofSetColor(150);
    std::string info = "BPM: " + ofToString(selectedTrack->bpm(), 1);
    if (songStructure) {
        info += "  |  Mood: " + std::string(cratedigger::track_mood_to_string(songStructure->mood));
        info += "  |  Bank: " + std::string(cratedigger::track_bank_to_string(songStructure->bank));
        info += "  |  Phrases: " + ofToString(songStructure->size());
        info += "  |  End Beat: " + ofToString(songStructure->end_beat);
    }
    ofDrawBitmapString(info, x, y);

    if (!songStructure || songStructure->empty()) {
        ofSetColor(100);
        y += 40;
        if (!anlzLoaded) {
            ofDrawBitmapString("Load ANLZ folder to see song structure", x, y);
        } else {
            ofDrawBitmapString("No song structure data for this track", x, y);
        }
        return;
    }

    drawPhraseTimeline();
    drawPhraseList();
    drawLegend();
}

void ofApp::drawPhraseTimeline() {
    if (!songStructure || songStructure->end_beat == 0) return;

    // Background
    ofSetColor(35);
    ofDrawRectangle(timelineStartX, timelineY, timelineWidth, timelineHeight);

    // Draw phrases as colored blocks
    for (const auto& phrase : songStructure->phrases) {
        float startX = beatToX(phrase.beat);
        float endX = beatToX(phrase.end_beat);
        float width = endX - startX;

        ofColor color = getPhraseColor(phrase);
        ofSetColor(color);
        ofDrawRectangle(startX, timelineY, width, timelineHeight);

        // Phrase name
        ofSetColor(255);
        std::string name = phrase.phrase_name(songStructure->mood);
        if (width > name.length() * 8 + 10) {
            ofDrawBitmapString(name, startX + 5, timelineY + 20);
        }

        // Fill indicator
        if (phrase.has_fill) {
            float fillX = beatToX(phrase.fill_beat);
            ofSetColor(255, 255, 255, 100);
            ofDrawRectangle(fillX, timelineY, endX - fillX, timelineHeight);
            ofSetColor(255);
            ofDrawBitmapString("F", fillX + 2, timelineY + timelineHeight - 5);
        }

        // Phrase boundary
        ofSetColor(20);
        ofDrawLine(startX, timelineY, startX, timelineY + timelineHeight);
    }

    // Beat markers every 16 beats
    ofSetColor(80);
    uint16_t totalBeats = songStructure->end_beat;
    for (uint16_t beat = 0; beat <= totalBeats; beat += 16) {
        float markerX = beatToX(beat);
        ofDrawLine(markerX, timelineY + timelineHeight, markerX, timelineY + timelineHeight + 10);
        if (beat % 64 == 0) {
            ofDrawBitmapString(ofToString(beat), markerX - 10, timelineY + timelineHeight + 25);
        }
    }

    // Playhead
    float playheadX = beatToX(static_cast<uint16_t>(playheadBeat));
    ofSetColor(255, 100, 100);
    ofDrawLine(playheadX, timelineY - 5, playheadX, timelineY + timelineHeight + 5);

    // Current beat display
    ofSetColor(255);
    ofDrawBitmapString("Beat: " + formatBeat(static_cast<uint16_t>(playheadBeat)), timelineStartX, timelineY - 10);
}

void ofApp::drawPhraseList() {
    if (!songStructure) return;

    float x = timelineStartX;
    float y = timelineY + timelineHeight + 60;

    ofSetColor(200);
    ofDrawBitmapString("== Phrase List ==", x, y);
    y += 20;

    // Headers
    ofSetColor(150);
    ofDrawBitmapString("#", x, y);
    ofDrawBitmapString("Phrase", x + 30, y);
    ofDrawBitmapString("Start", x + 120, y);
    ofDrawBitmapString("End", x + 180, y);
    ofDrawBitmapString("Duration", x + 240, y);
    ofDrawBitmapString("Fill", x + 320, y);
    y += 5;

    ofSetColor(60);
    ofDrawLine(x, y, x + 370, y);
    y += 15;

    // Phrase rows
    int maxRows = 15;
    int count = 0;
    for (const auto& phrase : songStructure->phrases) {
        if (count >= maxRows) {
            ofSetColor(100);
            ofDrawBitmapString("... and " + ofToString(songStructure->size() - maxRows) + " more", x, y);
            break;
        }

        // Highlight current phrase
        bool isCurrent = (playheadBeat >= phrase.beat && playheadBeat < phrase.end_beat);
        if (isCurrent) {
            ofSetColor(getPhraseColor(phrase), 80);
            ofDrawRectangle(x - 5, y - 12, 380, 18);
        }

        // Color indicator
        ofSetColor(getPhraseColor(phrase));
        ofDrawRectangle(x - 15, y - 10, 10, 12);

        ofSetColor(isCurrent ? 255 : 180);
        ofDrawBitmapString(ofToString(phrase.index), x, y);
        ofDrawBitmapString(phrase.phrase_name(songStructure->mood), x + 30, y);
        ofDrawBitmapString(formatBeat(phrase.beat), x + 120, y);
        ofDrawBitmapString(formatBeat(phrase.end_beat), x + 180, y);
        ofDrawBitmapString(ofToString(phrase.end_beat - phrase.beat) + " beats", x + 240, y);

        if (phrase.has_fill) {
            ofSetColor(255, 220, 100);
            ofDrawBitmapString("@ " + formatBeat(phrase.fill_beat), x + 320, y);
        }

        y += 18;
        count++;
    }
}

void ofApp::drawLegend() {
    float x = timelineStartX + 450;
    float y = timelineY + timelineHeight + 60;

    ofSetColor(200);
    ofDrawBitmapString("== Legend ==", x, y);
    y += 25;

    std::vector<std::string> types = {"Intro", "Verse", "Chorus", "Bridge", "Up", "Down", "Outro"};
    for (const auto& type : types) {
        auto it = phraseColors.find(type);
        if (it != phraseColors.end()) {
            ofSetColor(it->second);
            ofDrawRectangle(x, y - 10, 15, 12);
            ofSetColor(180);
            ofDrawBitmapString(type, x + 25, y);
            y += 18;
        }
    }
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    std::string instr = "Up/Down: Select track | Space: Reset playhead | 'q': Quit";
    ofDrawBitmapString(instr, 20, ofGetHeight() - 20);
}

ofColor ofApp::getPhraseColor(const cratedigger::PhraseEntry& phrase) {
    if (!songStructure) return ofColor(100);

    std::string name = phrase.phrase_name(songStructure->mood);
    auto it = phraseColors.find(name);
    if (it != phraseColors.end()) {
        return it->second;
    }

    // Try base name (remove numbers)
    if (name.find("Verse") != std::string::npos) {
        return phraseColors["Verse"];
    }

    return phraseColors["Unknown"];
}

std::string ofApp::formatBeat(uint16_t beat) {
    return ofToString(beat);
}

float ofApp::beatToX(uint16_t beat) {
    if (!songStructure || songStructure->end_beat == 0) return timelineStartX;
    float ratio = static_cast<float>(beat) / static_cast<float>(songStructure->end_beat);
    return timelineStartX + ratio * timelineWidth;
}
