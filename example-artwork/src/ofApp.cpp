#include "ofApp.h"
#include <algorithm>
#include <filesystem>

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(30);
    ofSetWindowTitle("ofxBeatLink - Artwork Viewer");
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
    drawArtworkDisplay();
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - Artwork Viewer", 30, 30);

    if (databaseLoaded) {
        ofSetColor(120);
        ofDrawBitmapString("Tracks: " + ofToString(tracks.size()), 30, 50);
    }
}

void ofApp::drawInstructions() {
    ofSetColor(100);
    ofDrawBitmapString("Drag and drop a PIONEER folder to load rekordbox database", 30, 100);
    ofDrawBitmapString("The folder should contain export.pdb file", 30, 120);

    ofSetColor(60);
    ofDrawBitmapString("Controls:", 30, 180);
    ofDrawBitmapString("  Up/Down    - Navigate tracks", 30, 200);
    ofDrawBitmapString("  Enter      - Load artwork", 30, 220);
    ofDrawBitmapString("  q          - Quit", 30, 240);
}

void ofApp::drawTrackList() {
    const float listX = 30;
    const float listY = 80;
    const float listWidth = 450;
    const float rowHeight = 22;

    // Background
    ofSetColor(25);
    ofDrawRectangle(listX - 5, listY - 5, listWidth, VISIBLE_TRACKS * rowHeight + 10);

    // Track list
    for (int i = 0; i < VISIBLE_TRACKS && (scrollOffset + i) < static_cast<int>(tracks.size()); ++i) {
        int idx = scrollOffset + i;
        const auto& track = tracks[idx];
        float y = listY + i * rowHeight;

        // Selection highlight
        if (idx == selectedIndex) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(listX - 3, y - 2, listWidth - 4, rowHeight - 2);
        }

        // Track number
        ofSetColor(80);
        ofDrawBitmapString(ofToString(idx + 1, 0, 3, '0'), listX, y + 12);

        // Title
        ofSetColor(idx == selectedIndex ? 255 : 200);
        std::string title = track.title;
        if (title.length() > 30) title = title.substr(0, 27) + "...";
        ofDrawBitmapString(title, listX + 40, y + 12);

        // Artist
        ofSetColor(idx == selectedIndex ? 180 : 120);
        std::string artist = track.artist;
        if (artist.length() > 20) artist = artist.substr(0, 17) + "...";
        ofDrawBitmapString(artist, listX + 290, y + 12);

        // Artwork indicator
        if (track.artworkLoaded) {
            ofSetColor(80, 200, 120);
            ofDrawBitmapString("[IMG]", listX + 410, y + 12);
        }
    }

    // Scrollbar
    if (tracks.size() > static_cast<size_t>(VISIBLE_TRACKS)) {
        float scrollbarHeight = VISIBLE_TRACKS * rowHeight;
        float thumbHeight = scrollbarHeight * VISIBLE_TRACKS / tracks.size();
        float thumbY = listY + (scrollbarHeight - thumbHeight) * scrollOffset / (tracks.size() - VISIBLE_TRACKS);

        ofSetColor(40);
        ofDrawRectangle(listX + listWidth - 8, listY, 6, scrollbarHeight);
        ofSetColor(80);
        ofDrawRectangle(listX + listWidth - 8, thumbY, 6, thumbHeight);
    }
}

void ofApp::drawArtworkDisplay() {
    const float artX = 520;
    const float artY = 80;
    const float artSize = 400;

    // Background
    ofSetColor(20);
    ofDrawRectangle(artX, artY, artSize, artSize);

    // Artwork
    if (hasCurrentArtwork && currentArtwork.isAllocated()) {
        ofSetColor(255);
        // Maintain aspect ratio
        float scale = std::min(artSize / currentArtwork.getWidth(),
                               artSize / currentArtwork.getHeight());
        float w = currentArtwork.getWidth() * scale;
        float h = currentArtwork.getHeight() * scale;
        float x = artX + (artSize - w) / 2;
        float y = artY + (artSize - h) / 2;
        currentArtwork.draw(x, y, w, h);
    } else {
        ofSetColor(60);
        ofDrawBitmapString("No artwork", artX + artSize / 2 - 40, artY + artSize / 2);
    }

    // Border
    ofNoFill();
    ofSetColor(60);
    ofDrawRectangle(artX, artY, artSize, artSize);
    ofFill();

    // Track info
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(tracks.size())) {
        const auto& track = tracks[selectedIndex];
        float infoY = artY + artSize + 30;

        ofSetColor(255);
        ofDrawBitmapString(track.title, artX, infoY);

        ofSetColor(150);
        ofDrawBitmapString(track.artist, artX, infoY + 20);

        ofSetColor(100);
        ofDrawBitmapString(track.album, artX, infoY + 40);

        if (!track.artworkPath.empty()) {
            ofSetColor(60);
            std::string pathDisplay = track.artworkPath;
            if (pathDisplay.length() > 50) {
                pathDisplay = "..." + pathDisplay.substr(pathDisplay.length() - 47);
            }
            ofDrawBitmapString("Path: " + pathDisplay, artX, infoY + 70);
        }
    }
}

void ofApp::keyPressed(int key) {
    if (key == 'q' || key == 'Q') {
        ofExit();
        return;
    }

    if (!databaseLoaded || tracks.empty()) return;

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
        selectTrack(static_cast<int>(tracks.size()) - 1);
    } else if (key == OF_KEY_RETURN || key == ' ') {
        // Load artwork for current track
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(tracks.size())) {
            loadArtworkForTrack(tracks[selectedIndex]);
        }
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
    tracks.clear();
    databaseLoaded = false;
    hasCurrentArtwork = false;

    // Find export.pdb
    fs::path dbPath;
    fs::path inputPath(path);

    if (fs::is_directory(inputPath)) {
        // Try PIONEER/rekordbox/export.pdb
        dbPath = inputPath / "rekordbox" / "export.pdb";
        if (!fs::exists(dbPath)) {
            dbPath = inputPath / "export.pdb";
        }
        basePath = inputPath.string();
    } else if (inputPath.filename() == "export.pdb") {
        dbPath = inputPath;
        basePath = inputPath.parent_path().parent_path().string();
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
    databaseLoaded = true;

    buildTrackList();

    if (!tracks.empty()) {
        selectTrack(0);
    }
}

void ofApp::buildTrackList() {
    tracks.clear();

    auto trackIds = database->all_track_ids();

    for (const auto& id : trackIds) {
        auto trackOpt = database->get_track(id);
        if (!trackOpt) continue;

        const auto& track = *trackOpt;

        TrackWithArtwork twa;
        twa.id = id;
        twa.title = track.title;
        twa.artist = getArtistName(track.artist_id);
        twa.album = getAlbumName(track.album_id);

        // Get artwork path
        if (track.artwork_id.value > 0) {
            auto artworkOpt = database->get_artwork(track.artwork_id);
            if (artworkOpt) {
                twa.artworkPath = artworkOpt->path;
            }
        }

        tracks.push_back(std::move(twa));
    }

    // Sort by title
    std::sort(tracks.begin(), tracks.end(),
              [](const TrackWithArtwork& a, const TrackWithArtwork& b) {
                  return a.title < b.title;
              });
}

void ofApp::loadArtworkForTrack(TrackWithArtwork& track) {
    hasCurrentArtwork = false;

    if (track.artworkPath.empty()) {
        return;
    }

    namespace fs = std::filesystem;

    // Build full path to artwork
    // Artwork path is relative to USB root, typically like:
    // /PIONEER/ARTWORK/01/0123abcd/0123abcd.jpg
    std::string artworkFullPath;

    if (track.artworkPath[0] == '/') {
        // Absolute path on USB
        artworkFullPath = basePath + track.artworkPath;
    } else {
        artworkFullPath = basePath + "/" + track.artworkPath;
    }

    // Try to load image
    if (track.artwork.load(artworkFullPath)) {
        track.artworkLoaded = true;
        currentArtwork = track.artwork;
        hasCurrentArtwork = true;
    } else {
        // Try alternate path formats
        fs::path altPath = fs::path(basePath).parent_path() / track.artworkPath;
        if (track.artwork.load(altPath.string())) {
            track.artworkLoaded = true;
            currentArtwork = track.artwork;
            hasCurrentArtwork = true;
        }
    }
}

void ofApp::selectTrack(int index) {
    if (tracks.empty()) return;

    selectedIndex = std::max(0, std::min(index, static_cast<int>(tracks.size()) - 1));

    // Adjust scroll
    if (selectedIndex < scrollOffset) {
        scrollOffset = selectedIndex;
    } else if (selectedIndex >= scrollOffset + VISIBLE_TRACKS) {
        scrollOffset = selectedIndex - VISIBLE_TRACKS + 1;
    }

    // Load artwork if already loaded
    auto& track = tracks[selectedIndex];
    if (track.artworkLoaded) {
        currentArtwork = track.artwork;
        hasCurrentArtwork = true;
    } else {
        hasCurrentArtwork = false;
    }
}

std::string ofApp::getArtistName(cratedigger::ArtistId id) {
    if (id.value == 0) return "";
    auto artist = database->get_artist(id);
    return artist ? artist->name : "";
}

std::string ofApp::getAlbumName(cratedigger::AlbumId id) {
    if (id.value == 0) return "";
    auto album = database->get_album(id);
    return album ? album->name : "";
}
