#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("ofxBeatLink - CDJ Status Monitor");

    // Register event listeners
    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    ofAddListener(beatLink.deviceUpdateEvent, this, &ofApp::onDeviceUpdate);

    // Start listening
    if (beatLink.start()) {
        addLog("Started listening for DJ Link devices...", ofColor(80, 200, 120));
    } else {
        addLog("ERROR: Failed to start - check if ports 50000/50001 are available",
               ofColor(255, 80, 80));
    }

    // Start VirtualCdj for detailed status updates
    if (beatLink.startVirtualCdj()) {
        addLog("VirtualCdj started as device #" + ofToString(beatLink.getVirtualCdjDeviceNumber()),
               ofColor(100, 200, 255));
    } else {
        addLog("VirtualCdj failed to start (status updates unavailable)",
               ofColor(255, 150, 80));
    }
}

void ofApp::update() {
    // Process events on main thread
    beatLink.update();

    // Update beat flash animations
    for (auto& kv : players) {
        kv.second.beatAlpha *= 0.92f;
    }
}

void ofApp::draw() {
    constexpr float margin = 20.0f;
    const float panelWidth = (ofGetWidth() - margin * 3) / 2;
    constexpr float panelHeight = 180.0f;

    // Title
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - CDJ Status Monitor", margin, 25);

    // VirtualCdj status
    if (beatLink.isVirtualCdjRunning()) {
        ofSetColor(100, 200, 255);
        ofDrawBitmapString("VirtualCdj: #" + ofToString(beatLink.getVirtualCdjDeviceNumber()),
                          margin, 45);
    }

    // Master tempo
    auto tempo = beatLink.getMasterTempo();
    if (tempo > 0) {
        ofSetColor(255, 200, 80);
        ofDrawBitmapString("Master: " + ofToString(tempo, 2) + " BPM", margin + 150, 45);
    }

    ofSetColor(150);
    ofDrawBitmapString("Listening on ports 50000/50001", margin + 320, 45);

    // Draw player panels
    float y = 70;
    int col = 0;
    const int maxCols = 2;

    for (const auto& kv : players) {
        const float x = margin + col * (panelWidth + margin);
        drawPlayerPanel(kv.first, x, y, panelWidth, panelHeight);
        col++;
        if (col >= maxCols) {
            col = 0;
            y += panelHeight + margin;
        }
    }

    // Show message if no devices
    if (players.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No devices found.", margin, 80);
        ofDrawBitmapString("Make sure CDJ/XDJ equipment is connected to the same network.", margin, 100);
    }

    // Log area at bottom
    const float logY = ofGetHeight() - margin - logMessages.size() * 14 - 30;
    ofSetColor(60);
    ofDrawLine(margin, logY - 10, ofGetWidth() - margin, logY - 10);

    ofSetColor(100);
    ofDrawBitmapString("Event Log:", margin, logY);

    float lineY = logY + 16;
    for (const auto& entry : logMessages) {
        ofSetColor(80);
        ofDrawBitmapString(entry.timestamp, margin, lineY);
        ofSetColor(entry.color);
        ofDrawBitmapString(entry.message, margin + 95, lineY);
        lineY += 14;
    }

    // Instructions
    ofSetColor(60);
    ofDrawBitmapString("Press 'r' to refresh, 'q' to quit", ofGetWidth() - 250, 25);
}

void ofApp::drawPlayerPanel(int deviceNumber, float x, float y, float width, float height) {
    auto it = players.find(deviceNumber);
    if (it == players.end()) {
        return;
    }

    const auto& player = it->second;
    const auto& device = player.device;

    // Panel background with beat flash
    const float flash = player.beatAlpha * 0.15f;
    ofSetColor(static_cast<int>(35 + flash * 50),
               static_cast<int>(35 + flash * 30), 35);
    ofDrawRectRounded(x, y, width, height, 8);

    // Header with device name
    bool isPlaying = player.status && player.status->isPlaying;
    bool isMaster = player.status && player.status->isMaster;

    if (isMaster) {
        ofSetColor(180, 140, 50);
    } else if (isPlaying) {
        ofSetColor(60, 120, 200);
    } else {
        ofSetColor(80, 120, 180);
    }
    ofDrawRectRounded(x, y, width, 30, 8);
    ofDrawRectangle(x, y + 22, width, 8);

    ofSetColor(255);
    std::string headerText = device.deviceName + " (#" + ofToString(device.deviceNumber) + ")";
    ofDrawBitmapString(headerText, x + 10, y + 20);

    float contentY = y + 45;
    const float leftCol = x + 15;
    const float rightCol = x + width / 2 + 10;

    // Status badges
    float badgeX = leftCol;
    if (player.status) {
        const auto& status = *player.status;

        if (status.isPlaying) {
            drawStatusBadge(badgeX, contentY, "PLAYING", ofColor(80, 255, 120));
            badgeX += 80;
        } else {
            drawStatusBadge(badgeX, contentY, "STOPPED", ofColor(100));
            badgeX += 80;
        }

        if (status.isMaster) {
            drawStatusBadge(badgeX, contentY, "MASTER", ofColor(255, 200, 80));
            badgeX += 75;
        }

        if (status.isSynced) {
            drawStatusBadge(badgeX, contentY, "SYNC", ofColor(100, 180, 255));
            badgeX += 60;
        }

        if (status.isOnAir) {
            drawStatusBadge(badgeX, contentY, "ON-AIR", ofColor(255, 80, 80));
        }
    }

    contentY += 25;

    // BPM and beat info
    if (player.status) {
        const auto& status = *player.status;

        // Beat indicators
        drawBeatIndicators(status.beatWithinBar, player.beatAlpha, rightCol + 40, contentY - 4);

        // BPM display
        ofSetColor(100, 255, 100);
        ofDrawBitmapString("BPM: " + ofToString(status.effectiveBpm, 1), leftCol, contentY);
        contentY += 22;

        // Track BPM
        ofSetColor(150);
        ofDrawBitmapString("Track BPM: " + ofToString(status.bpm, 1), leftCol, contentY);
        contentY += 22;

        // Pitch
        ofSetColor(180);
        std::string pitchStr = (status.pitchPercent >= 0 ? "+" : "") +
                               ofToString(status.pitchPercent, 2) + "%";
        ofDrawBitmapString("Pitch: " + pitchStr, leftCol, contentY);
    } else if (player.lastBeat) {
        // Fallback to beat data
        const auto& beat = *player.lastBeat;

        drawBeatIndicators(beat.beatWithinBar, player.beatAlpha, rightCol + 40, contentY - 4);

        ofSetColor(100, 255, 100);
        ofDrawBitmapString("BPM: " + ofToString(beat.bpm, 1), leftCol, contentY);
        contentY += 22;

        ofSetColor(150);
        ofDrawBitmapString("Track BPM: " + ofToString(beat.trackBpm, 1), leftCol, contentY);
        contentY += 22;

        ofSetColor(180);
        std::string pitchStr = (beat.pitchPercent >= 0 ? "+" : "") +
                               ofToString(beat.pitchPercent, 2) + "%";
        ofDrawBitmapString("Pitch: " + pitchStr, leftCol, contentY);
    } else {
        ofSetColor(100);
        ofDrawBitmapString("Waiting for data...", leftCol, contentY);
    }

    // Device info at bottom
    contentY = y + height - 25;
    ofSetColor(80);
    ofDrawBitmapString("IP: " + device.ipAddress, leftCol, contentY);
}

void ofApp::drawBeatIndicators(int beatWithinBar, float alpha, float x, float y) {
    for (int i = 1; i <= 4; ++i) {
        const float circleX = x + (i - 1) * 22;
        const bool isActive = (i == beatWithinBar);

        if (isActive) {
            int r = (i == 1) ? 255 : 80;
            int g = (i == 1) ? 80 : 255;
            int b = (i == 1) ? 80 : 80;
            ofSetColor(r, g, b, static_cast<int>(150 + 105 * alpha));
            ofDrawCircle(circleX, y, 8);
        } else {
            ofSetColor(60);
            ofNoFill();
            ofDrawCircle(circleX, y, 8);
            ofFill();
        }
    }
}

void ofApp::drawStatusBadge(float x, float y, std::string_view text, const ofColor& color) {
    const float width = text.length() * 8.0f + 10.0f;
    constexpr float height = 16.0f;

    ofSetColor(color.r / 5, color.g / 5, color.b / 5, 200);
    ofDrawRectRounded(x, y - 11, width, height, 3);

    ofSetColor(color);
    ofDrawBitmapString(std::string(text), x + 5, y);
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    ofRemoveListener(beatLink.deviceUpdateEvent, this, &ofApp::onDeviceUpdate);

    beatLink.stopVirtualCdj();
    beatLink.stop();
}

void ofApp::keyPressed(int key) {
    if (key == 'r' || key == 'R') {
        auto currentDevices = beatLink.getCurrentDevices();
        addLog("Refreshed: " + ofToString(currentDevices.size()) + " devices");
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    auto it = players.find(beat.deviceNumber);
    if (it != players.end()) {
        it->second.lastBeat = beat;
        it->second.beatAlpha = 1.0f;
    }

    // Log downbeats only
    if (beat.beatWithinBar == 1) {
        addLog("Beat: " + beat.deviceName + " BPM=" + ofToString(beat.bpm, 1));
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    PlayerInfo info;
    info.device = device;
    players[device.deviceNumber] = info;
    addLog("Found: " + device.deviceName + " (#" + ofToString(device.deviceNumber) + ")",
           ofColor(100, 200, 255));
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    players.erase(device.deviceNumber);
    addLog("Lost: " + device.deviceName + " (#" + ofToString(device.deviceNumber) + ")",
           ofColor(255, 150, 80));
}

void ofApp::onDeviceUpdate(ofxBeatLinkCdjStatus& status) {
    auto it = players.find(status.deviceNumber);
    if (it != players.end()) {
        it->second.status = status;
    }
}

void ofApp::addLog(std::string_view message, const ofColor& color) {
    LogEntry entry;
    entry.timestamp = ofGetTimestampString("%H:%M:%S");
    entry.message = std::string(message);
    entry.color = color;
    logMessages.emplace_front(entry);

    while (logMessages.size() > MAX_LOG_MESSAGES) {
        logMessages.pop_back();
    }
}
