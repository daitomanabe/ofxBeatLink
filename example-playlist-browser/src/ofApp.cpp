#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("Playlist Browser");

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
        drawPlaylistPanel();
        drawTrackPanel();
    }

    if (!errorMessage.empty()) {
        ofSetColor(255, 100, 100);
        ofDrawBitmapString("Error: " + errorMessage, 20, ofGetHeight() - 60);
    }

    drawInstructions();
}

void ofApp::keyPressed(int key) {
    if (!databaseLoaded) return;

    int maxPlaylists = static_cast<int>(playlistIds.size());
    int maxTracks = static_cast<int>(playlistTracks.size());

    if (key == OF_KEY_TAB) {
        focusPanel = (focusPanel + 1) % 2;
    } else if (key == OF_KEY_LEFT) {
        focusPanel = 0;
    } else if (key == OF_KEY_RIGHT) {
        focusPanel = 1;
    } else if (key == OF_KEY_UP) {
        if (focusPanel == 0) {
            selectedPlaylistIndex = std::max(0, selectedPlaylistIndex - 1);
            if (selectedPlaylistIndex < playlistScrollOffset) {
                playlistScrollOffset = selectedPlaylistIndex;
            }
            selectPlaylist(selectedPlaylistIndex);
        } else {
            selectedTrackIndex = std::max(0, selectedTrackIndex - 1);
            if (selectedTrackIndex < trackScrollOffset) {
                trackScrollOffset = selectedTrackIndex;
            }
        }
    } else if (key == OF_KEY_DOWN) {
        if (focusPanel == 0) {
            selectedPlaylistIndex = std::min(maxPlaylists - 1, selectedPlaylistIndex + 1);
            if (selectedPlaylistIndex >= playlistScrollOffset + 25) {
                playlistScrollOffset = selectedPlaylistIndex - 24;
            }
            selectPlaylist(selectedPlaylistIndex);
        } else {
            selectedTrackIndex = std::min(maxTracks - 1, selectedTrackIndex + 1);
            if (selectedTrackIndex >= trackScrollOffset + 25) {
                trackScrollOffset = selectedTrackIndex - 24;
            }
        }
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

    playlistIds = database->all_playlist_ids();
    selectedPlaylistIndex = 0;
    playlistScrollOffset = 0;

    if (!playlistIds.empty()) {
        selectPlaylist(0);
    }

    ofLogNotice("ofApp") << "Loaded " << playlistIds.size() << " playlists";
}

void ofApp::selectPlaylist(int index) {
    if (index < 0 || index >= static_cast<int>(playlistIds.size())) return;

    playlistTracks.clear();
    selectedTrackIndex = 0;
    trackScrollOffset = 0;

    auto tracks = database->get_playlist(playlistIds[index]);
    if (tracks) {
        playlistTracks = *tracks;
    }
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("Playlist Browser", 20, 25);

    if (databaseLoaded) {
        ofSetColor(150);
        ofDrawBitmapString("Playlists: " + ofToString(playlistIds.size()), 200, 25);
    }
}

void ofApp::drawPlaylistPanel() {
    float x = 20, y = 60;
    float panelWidth = 350;

    // Panel header
    ofSetColor(focusPanel == 0 ? ofColor(100, 180, 255) : ofColor(150));
    ofDrawBitmapString("== Playlists ==", x, y);
    y += 25;

    // Panel background
    ofSetColor(focusPanel == 0 ? 40 : 30);
    ofDrawRectangle(x - 5, y - 15, panelWidth, ofGetHeight() - y - 50);

    if (playlistIds.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No playlists found", x, y + 20);
        return;
    }

    int visibleRows = 25;
    int endIndex = std::min(playlistScrollOffset + visibleRows, static_cast<int>(playlistIds.size()));

    for (int i = playlistScrollOffset; i < endIndex; ++i) {
        bool selected = (i == selectedPlaylistIndex);

        if (selected && focusPanel == 0) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 3, y - 12, panelWidth - 5, 18);
        }

        ofSetColor(selected ? 255 : 150);

        std::string name = getPlaylistName(playlistIds[i]);
        if (name.length() > 40) name = name.substr(0, 37) + "...";
        ofDrawBitmapString(name, x, y);

        // Track count
        auto tracks = database->get_playlist(playlistIds[i]);
        if (tracks) {
            ofSetColor(100);
            ofDrawBitmapString("(" + ofToString(tracks->size()) + ")", x + 280, y);
        }

        y += 18;
    }

    // Scroll indicator
    if (playlistIds.size() > static_cast<size_t>(visibleRows)) {
        ofSetColor(80);
        ofDrawBitmapString(ofToString(playlistScrollOffset + 1) + "-" + ofToString(endIndex) +
                          "/" + ofToString(playlistIds.size()), x, ofGetHeight() - 65);
    }
}

void ofApp::drawTrackPanel() {
    float x = 400, y = 60;
    float panelWidth = ofGetWidth() - x - 20;

    // Panel header
    ofSetColor(focusPanel == 1 ? ofColor(100, 180, 255) : ofColor(150));
    std::string header = "== Tracks";
    if (selectedPlaylistIndex < static_cast<int>(playlistIds.size())) {
        header += " in \"" + getPlaylistName(playlistIds[selectedPlaylistIndex]) + "\"";
    }
    header += " ==";
    ofDrawBitmapString(header, x, y);
    y += 25;

    // Panel background
    ofSetColor(focusPanel == 1 ? 40 : 30);
    ofDrawRectangle(x - 5, y - 15, panelWidth, ofGetHeight() - y - 50);

    if (playlistTracks.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No tracks in playlist", x, y + 20);
        return;
    }

    // Column headers
    ofSetColor(200);
    ofDrawBitmapString("#", x, y);
    ofDrawBitmapString("Title", x + 40, y);
    ofDrawBitmapString("Artist", x + 350, y);
    ofDrawBitmapString("BPM", x + 550, y);
    ofDrawBitmapString("Duration", x + 610, y);
    y += 5;
    ofSetColor(60);
    ofDrawLine(x, y, x + panelWidth - 10, y);
    y += 15;

    int visibleRows = 25;
    int endIndex = std::min(trackScrollOffset + visibleRows, static_cast<int>(playlistTracks.size()));

    for (int i = trackScrollOffset; i < endIndex; ++i) {
        auto track = database->get_track(playlistTracks[i]);
        if (!track) continue;

        bool selected = (i == selectedTrackIndex);

        if (selected && focusPanel == 1) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 3, y - 12, panelWidth - 5, 18);
        }

        ofSetColor(selected ? 255 : 150);
        ofDrawBitmapString(ofToString(i + 1), x, y);

        std::string title = track->title;
        if (title.length() > 35) title = title.substr(0, 32) + "...";
        ofDrawBitmapString(title, x + 40, y);

        if (auto artist = database->get_artist(track->artist_id)) {
            std::string artistName = artist->name;
            if (artistName.length() > 22) artistName = artistName.substr(0, 19) + "...";
            ofDrawBitmapString(artistName, x + 350, y);
        }

        if (track->bpm() > 0) {
            ofDrawBitmapString(ofToString(track->bpm(), 1), x + 550, y);
        }

        ofDrawBitmapString(formatDuration(track->duration_seconds), x + 610, y);

        y += 18;
    }

    // Scroll indicator
    if (playlistTracks.size() > static_cast<size_t>(visibleRows)) {
        ofSetColor(80);
        ofDrawBitmapString(ofToString(trackScrollOffset + 1) + "-" + ofToString(endIndex) +
                          "/" + ofToString(playlistTracks.size()), x, ofGetHeight() - 65);
    }
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    ofDrawBitmapString("Tab/Left/Right: Switch panel | Up/Down: Navigate | 'q': Quit", 20, ofGetHeight() - 20);
}

std::string ofApp::getPlaylistName(cratedigger::PlaylistId id) {
    // Try to get from folder entries or use ID
    auto folder = database->get_playlist_folder(id);
    if (folder && !folder->empty()) {
        return folder->front().name;
    }
    return "Playlist " + ofToString(id.value);
}

std::string ofApp::formatDuration(uint32_t seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return ofToString(mins) + ":" + (secs < 10 ? "0" : "") + ofToString(secs);
}
