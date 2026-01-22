#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(30);
    ofSetWindowTitle("Rekordbox Database Browser");

    // Try loading from data folder
    std::string defaultPath = ofToDataPath("export.pdb");
    if (ofFile::doesFileExist(defaultPath)) {
        loadDatabase(defaultPath);
    }
}

void ofApp::update() {
    // Nothing to update
}

void ofApp::draw() {
    drawHeader();

    if (!databaseLoaded) {
        // Show drop zone
        ofSetColor(80);
        ofNoFill();
        ofSetLineWidth(2);
        ofDrawRectangle(100, 150, ofGetWidth() - 200, ofGetHeight() - 250);
        ofFill();
        ofSetLineWidth(1);

        ofSetColor(120);
        std::string msg = "Drag and drop export.pdb here";
        ofDrawBitmapString(msg, ofGetWidth() / 2 - msg.length() * 4, ofGetHeight() / 2);

        if (!errorMessage.empty()) {
            ofSetColor(255, 100, 100);
            ofDrawBitmapString("Error: " + errorMessage, 100, ofGetHeight() - 60);
        }
    } else {
        drawTabs();

        switch (currentView) {
            case ViewMode::Tracks:
                drawTrackList();
                drawTrackDetails();
                break;
            case ViewMode::Artists:
                drawArtistList();
                break;
            case ViewMode::Albums:
                drawAlbumList();
                break;
            case ViewMode::Genres:
                drawGenreList();
                break;
            case ViewMode::Playlists:
                // TODO: Implement playlist view
                ofSetColor(100);
                ofDrawBitmapString("Playlist view not yet implemented", 100, 150);
                break;
        }
    }

    drawInstructions();
}

void ofApp::keyPressed(int key) {
    if (!databaseLoaded) return;

    size_t listSize = 0;
    switch (currentView) {
        case ViewMode::Tracks: listSize = trackIds.size(); break;
        case ViewMode::Artists: listSize = artistIds.size(); break;
        case ViewMode::Albums: listSize = albumIds.size(); break;
        case ViewMode::Genres: listSize = genreIds.size(); break;
        default: break;
    }

    if (key == OF_KEY_UP) {
        selectedIndex = std::max(0, selectedIndex - 1);
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    } else if (key == OF_KEY_DOWN) {
        selectedIndex = std::min(static_cast<int>(listSize) - 1, selectedIndex + 1);
        if (selectedIndex >= scrollOffset + visibleRows) {
            scrollOffset = selectedIndex - visibleRows + 1;
        }
    } else if (key == OF_KEY_PAGE_UP) {
        selectedIndex = std::max(0, selectedIndex - visibleRows);
        scrollOffset = std::max(0, scrollOffset - visibleRows);
    } else if (key == OF_KEY_PAGE_DOWN) {
        selectedIndex = std::min(static_cast<int>(listSize) - 1, selectedIndex + visibleRows);
        scrollOffset = std::min(static_cast<int>(listSize) - visibleRows, scrollOffset + visibleRows);
        if (scrollOffset < 0) scrollOffset = 0;
    } else if (key == OF_KEY_TAB) {
        // Cycle through views
        int view = static_cast<int>(currentView);
        view = (view + 1) % 5;
        currentView = static_cast<ViewMode>(view);
        selectedIndex = 0;
        scrollOffset = 0;
    } else if (key == '1') {
        currentView = ViewMode::Tracks;
        selectedIndex = 0;
        scrollOffset = 0;
    } else if (key == '2') {
        currentView = ViewMode::Artists;
        selectedIndex = 0;
        scrollOffset = 0;
    } else if (key == '3') {
        currentView = ViewMode::Albums;
        selectedIndex = 0;
        scrollOffset = 0;
    } else if (key == '4') {
        currentView = ViewMode::Genres;
        selectedIndex = 0;
        scrollOffset = 0;
    } else if (key == '5') {
        currentView = ViewMode::Playlists;
        selectedIndex = 0;
        scrollOffset = 0;
    } else if (key == OF_KEY_RETURN) {
        // Select track for details
        if (currentView == ViewMode::Tracks && selectedIndex < static_cast<int>(trackIds.size())) {
            selectedTrack = database->get_track(trackIds[selectedIndex]);
        }
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (!dragInfo.files.empty()) {
        loadDatabase(dragInfo.files[0]);
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

    refreshLists();

    ofLogNotice("ofApp") << "Loaded database: " << path;
    ofLogNotice("ofApp") << "  Tracks: " << trackIds.size();
    ofLogNotice("ofApp") << "  Artists: " << artistIds.size();
    ofLogNotice("ofApp") << "  Albums: " << albumIds.size();
    ofLogNotice("ofApp") << "  Genres: " << genreIds.size();
}

void ofApp::refreshLists() {
    if (!database) return;

    trackIds = database->all_track_ids();
    artistIds = database->all_artist_ids();
    albumIds = database->all_album_ids();
    genreIds = database->all_genre_ids();

    selectedIndex = 0;
    scrollOffset = 0;
    selectedTrack.reset();
}

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("Rekordbox Database Browser", 20, 25);

    if (databaseLoaded) {
        ofSetColor(100, 200, 100);
        ofDrawBitmapString("Loaded: " + databasePath, 20, 45);

        ofSetColor(150);
        std::string stats = "Tracks: " + ofToString(trackIds.size()) +
                           " | Artists: " + ofToString(artistIds.size()) +
                           " | Albums: " + ofToString(albumIds.size()) +
                           " | Genres: " + ofToString(genreIds.size());
        ofDrawBitmapString(stats, 20, 65);
    }
}

void ofApp::drawTabs() {
    float tabWidth = 150;
    float y = 85;

    std::vector<std::string> tabs = {"1:Tracks", "2:Artists", "3:Albums", "4:Genres", "5:Playlists"};

    for (size_t i = 0; i < tabs.size(); ++i) {
        float x = 20 + i * tabWidth;
        bool active = (static_cast<int>(currentView) == static_cast<int>(i));

        if (active) {
            ofSetColor(60, 120, 180);
            ofDrawRectangle(x, y, tabWidth - 5, 25);
            ofSetColor(255);
        } else {
            ofSetColor(50);
            ofDrawRectangle(x, y, tabWidth - 5, 25);
            ofSetColor(150);
        }
        ofDrawBitmapString(tabs[i], x + 10, y + 17);
    }
}

void ofApp::drawTrackList() {
    float y = 130;
    float x = 20;
    float rowHeight = 20;

    // Column headers
    ofSetColor(200);
    ofDrawBitmapString("#", x, y);
    ofDrawBitmapString("Title", x + 50, y);
    ofDrawBitmapString("Artist", x + 350, y);
    ofDrawBitmapString("BPM", x + 550, y);
    ofDrawBitmapString("Duration", x + 620, y);
    y += 5;

    ofSetColor(60);
    ofDrawLine(x, y, x + 700, y);
    y += 15;

    // Track rows
    int endIndex = std::min(scrollOffset + visibleRows, static_cast<int>(trackIds.size()));
    for (int i = scrollOffset; i < endIndex; ++i) {
        auto track = database->get_track(trackIds[i]);
        if (!track) continue;

        bool selected = (i == selectedIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 5, y - 12, 710, rowHeight);
        }

        ofSetColor(selected ? 255 : 180);
        ofDrawBitmapString(ofToString(i + 1), x, y);

        // Truncate title if too long
        std::string title = track->title;
        if (title.length() > 35) title = title.substr(0, 32) + "...";
        ofDrawBitmapString(title, x + 50, y);

        std::string artist = getArtistName(track->artist_id);
        if (artist.length() > 22) artist = artist.substr(0, 19) + "...";
        ofDrawBitmapString(artist, x + 350, y);

        if (track->tempo > 0) {
            ofDrawBitmapString(ofToString(track->tempo / 100.0f, 1), x + 550, y);
        }

        ofDrawBitmapString(formatDuration(track->duration), x + 620, y);

        y += rowHeight;
    }

    // Scroll indicator
    if (trackIds.size() > static_cast<size_t>(visibleRows)) {
        ofSetColor(100);
        ofDrawBitmapString("Showing " + ofToString(scrollOffset + 1) + "-" +
                          ofToString(endIndex) + " of " + ofToString(trackIds.size()),
                          x, ofGetHeight() - 80);
    }
}

void ofApp::drawArtistList() {
    float y = 130;
    float x = 20;
    float rowHeight = 20;

    ofSetColor(200);
    ofDrawBitmapString("#", x, y);
    ofDrawBitmapString("Artist Name", x + 50, y);
    y += 5;

    ofSetColor(60);
    ofDrawLine(x, y, x + 500, y);
    y += 15;

    int endIndex = std::min(scrollOffset + visibleRows, static_cast<int>(artistIds.size()));
    for (int i = scrollOffset; i < endIndex; ++i) {
        auto artist = database->get_artist(artistIds[i]);
        if (!artist) continue;

        bool selected = (i == selectedIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 5, y - 12, 510, rowHeight);
        }

        ofSetColor(selected ? 255 : 180);
        ofDrawBitmapString(ofToString(i + 1), x, y);
        ofDrawBitmapString(artist->name, x + 50, y);

        y += rowHeight;
    }
}

void ofApp::drawAlbumList() {
    float y = 130;
    float x = 20;
    float rowHeight = 20;

    ofSetColor(200);
    ofDrawBitmapString("#", x, y);
    ofDrawBitmapString("Album Name", x + 50, y);
    ofDrawBitmapString("Artist", x + 400, y);
    y += 5;

    ofSetColor(60);
    ofDrawLine(x, y, x + 600, y);
    y += 15;

    int endIndex = std::min(scrollOffset + visibleRows, static_cast<int>(albumIds.size()));
    for (int i = scrollOffset; i < endIndex; ++i) {
        auto album = database->get_album(albumIds[i]);
        if (!album) continue;

        bool selected = (i == selectedIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 5, y - 12, 610, rowHeight);
        }

        ofSetColor(selected ? 255 : 180);
        ofDrawBitmapString(ofToString(i + 1), x, y);

        std::string name = album->name;
        if (name.length() > 40) name = name.substr(0, 37) + "...";
        ofDrawBitmapString(name, x + 50, y);

        ofDrawBitmapString(getArtistName(album->artist_id), x + 400, y);

        y += rowHeight;
    }
}

void ofApp::drawGenreList() {
    float y = 130;
    float x = 20;
    float rowHeight = 20;

    ofSetColor(200);
    ofDrawBitmapString("#", x, y);
    ofDrawBitmapString("Genre Name", x + 50, y);
    y += 5;

    ofSetColor(60);
    ofDrawLine(x, y, x + 400, y);
    y += 15;

    int endIndex = std::min(scrollOffset + visibleRows, static_cast<int>(genreIds.size()));
    for (int i = scrollOffset; i < endIndex; ++i) {
        auto genre = database->get_genre(genreIds[i]);
        if (!genre) continue;

        bool selected = (i == selectedIndex);
        if (selected) {
            ofSetColor(60, 100, 140);
            ofDrawRectangle(x - 5, y - 12, 410, rowHeight);
        }

        ofSetColor(selected ? 255 : 180);
        ofDrawBitmapString(ofToString(i + 1), x, y);
        ofDrawBitmapString(genre->name, x + 50, y);

        y += rowHeight;
    }
}

void ofApp::drawTrackDetails() {
    if (!selectedTrack) return;

    float x = 750;
    float y = 130;

    ofSetColor(100, 180, 255);
    ofDrawBitmapString("== Track Details ==", x, y);
    y += 25;

    ofSetColor(180);
    ofDrawBitmapString("Title:", x, y);
    ofSetColor(255);
    ofDrawBitmapString(selectedTrack->title, x + 80, y);
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("Artist:", x, y);
    ofSetColor(255);
    ofDrawBitmapString(getArtistName(selectedTrack->artist_id), x + 80, y);
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("Album:", x, y);
    ofSetColor(255);
    ofDrawBitmapString(getAlbumName(selectedTrack->album_id), x + 80, y);
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("Genre:", x, y);
    ofSetColor(255);
    ofDrawBitmapString(getGenreName(selectedTrack->genre_id), x + 80, y);
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("BPM:", x, y);
    ofSetColor(255, 200, 80);
    if (selectedTrack->tempo > 0) {
        ofDrawBitmapString(ofToString(selectedTrack->tempo / 100.0f, 2), x + 80, y);
    }
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("Duration:", x, y);
    ofSetColor(255);
    ofDrawBitmapString(formatDuration(selectedTrack->duration), x + 80, y);
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("Rating:", x, y);
    ofSetColor(255, 220, 100);
    std::string stars;
    for (int i = 0; i < selectedTrack->rating; ++i) stars += "*";
    if (stars.empty()) stars = "-";
    ofDrawBitmapString(stars, x + 80, y);
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("Track #:", x, y);
    ofSetColor(255);
    ofDrawBitmapString(ofToString(selectedTrack->track_number), x + 80, y);
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("Year:", x, y);
    ofSetColor(255);
    if (selectedTrack->year > 0) {
        ofDrawBitmapString(ofToString(selectedTrack->year), x + 80, y);
    }
    y += 20;

    ofSetColor(180);
    ofDrawBitmapString("Key:", x, y);
    ofSetColor(100, 255, 200);
    if (selectedTrack->key_id.value > 0) {
        if (auto key = database->get_key(selectedTrack->key_id)) {
            ofDrawBitmapString(key->name, x + 80, y);
        }
    }
    y += 30;

    ofSetColor(180);
    ofDrawBitmapString("File:", x, y);
    y += 15;
    ofSetColor(120);
    std::string path = selectedTrack->file_path;
    // Wrap long paths
    while (path.length() > 50) {
        ofDrawBitmapString(path.substr(0, 50), x, y);
        path = path.substr(50);
        y += 15;
    }
    ofDrawBitmapString(path, x, y);
}

void ofApp::drawInstructions() {
    ofSetColor(60);
    std::string instructions = "Up/Down: Navigate | PgUp/PgDn: Page | Tab/1-5: Switch view | Enter: Details | 'q': Quit";
    ofDrawBitmapString(instructions, 20, ofGetHeight() - 20);
}

std::string ofApp::formatDuration(int seconds) {
    if (seconds <= 0) return "--:--";
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

std::string ofApp::getAlbumName(cratedigger::AlbumId id) {
    if (id.value == 0) return "";
    if (auto album = database->get_album(id)) {
        return album->name;
    }
    return "";
}

std::string ofApp::getGenreName(cratedigger::GenreId id) {
    if (id.value == 0) return "";
    if (auto genre = database->get_genre(id)) {
        return genre->name;
    }
    return "";
}
