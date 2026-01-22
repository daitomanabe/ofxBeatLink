#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("ofxBeatLink - VirtualCdj Status Monitor");

    // Register event listeners
    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    ofAddListener(beatLink.deviceUpdateEvent, this, &ofApp::onDeviceUpdate);
    ofAddListener(beatLink.masterChangedEvent, this, &ofApp::onMasterChanged);

    // Set device name before starting
    beatLink.setVirtualCdjDeviceName("ofxBeatLink");

    // Start basic listening first
    if (beatLink.start()) {
        addLog("DeviceFinder started on ports 50000/50001", ofColor(80, 200, 120));
    } else {
        addLog("ERROR: Failed to start DeviceFinder", ofColor(255, 80, 80));
    }

    // Start VirtualCdj for full status updates
    if (beatLink.startVirtualCdj()) {
        auto deviceNum = beatLink.getVirtualCdjDeviceNumber();
        addLog("VirtualCdj started as device #" + ofToString(deviceNum), ofColor(100, 200, 255));
    } else {
        addLog("ERROR: Failed to start VirtualCdj", ofColor(255, 80, 80));
    }
}

void ofApp::update() {
    beatLink.update();

    // Update player states
    for (auto& kv : players) {
        // Decay beat flash
        kv.second.beatAlpha *= 0.9f;
    }
}

void ofApp::draw() {
    // Header
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - VirtualCdj Status Monitor", 30, 30);

    // VirtualCdj status
    ofSetColor(100, 200, 255);
    if (beatLink.isVirtualCdjRunning()) {
        ofDrawBitmapString("VirtualCdj: Device #" + ofToString(beatLink.getVirtualCdjDeviceNumber()),
                          30, 50);
    } else {
        ofSetColor(255, 100, 100);
        ofDrawBitmapString("VirtualCdj: Not Running", 30, 50);
    }

    // Master tempo
    ofSetColor(255, 200, 80);
    auto masterTempo = beatLink.getMasterTempo();
    if (masterTempo > 0) {
        ofDrawBitmapString("Master Tempo: " + ofToString(masterTempo, 2) + " BPM", 280, 50);
    }

    // Instructions
    ofSetColor(120);
    ofDrawBitmapString("Press 'r' to refresh | 'v' to toggle VirtualCdj | 'q' to quit",
                      ofGetWidth() - 420, 30);

    // Device panels
    constexpr auto marginX = 30.0f;
    constexpr auto marginY = 70.0f;
    constexpr auto gapX = 30.0f;
    constexpr auto gapY = 20.0f;

    for (int slot = 0; slot < MAX_PLAYERS; ++slot) {
        const auto col = slot % 2;
        const auto row = slot / 2;
        const auto x = marginX + static_cast<float>(col) * (PANEL_WIDTH + gapX);
        const auto y = marginY + static_cast<float>(row) * (PANEL_HEIGHT + gapY);
        const auto deviceNum = slot + 1;

        auto it = players.find(deviceNum);
        if (it != players.end()) {
            drawPlayerPanel(it->second, x, y, PANEL_WIDTH, PANEL_HEIGHT);
        } else {
            drawEmptySlot(deviceNum, x, y, PANEL_WIDTH, PANEL_HEIGHT);
        }
    }

    // Event log section
    const auto logY = marginY + 2 * (PANEL_HEIGHT + gapY) + 10;

    ofSetColor(60);
    ofDrawLine(30, logY, ofGetWidth() - 30, logY);

    ofSetColor(30);
    ofDrawRectRounded(25, logY + 10, ofGetWidth() - 50, 180, 5);

    ofSetColor(100);
    ofDrawBitmapString("Event Log:", 35, logY + 30);

    auto logLineY = logY + 55;
    for (const auto& entry : eventLog) {
        ofSetColor(80);
        ofDrawBitmapString(entry.timestamp, 35, logLineY);

        ofSetColor(entry.color);
        ofDrawBitmapString(entry.message, 130, logLineY);

        logLineY += 16;
    }
}

void ofApp::drawEmptySlot(int slotNum, float x, float y, float width, float height) {
    ofSetColor(30);
    ofDrawRectRounded(x, y, width, height, 8);

    ofSetColor(50);
    ofNoFill();
    ofDrawRectRounded(x, y, width, height, 8);
    ofFill();

    ofSetColor(60);
    ofDrawBitmapString("Player #" + ofToString(slotNum) + " - Not Connected", x + 20, y + 30);
}

void ofApp::drawPlayerPanel(const PlayerState& player, float x, float y, float width, float height) {
    // Background with beat flash
    const auto flash = player.beatAlpha * 0.2f;
    ofSetColor(static_cast<int>(35 + flash * 60),
               static_cast<int>(35 + flash * 40), 35);
    ofDrawRectRounded(x, y, width, height, 8);

    // Border color based on status
    bool isPlaying = player.status && player.status->isPlaying;
    bool isMaster = currentMaster && *currentMaster == player.info.deviceNumber;

    if (isMaster) {
        ofSetColor(255, 200, 80, static_cast<int>(200 + player.beatAlpha * 55));
    } else if (isPlaying) {
        ofSetColor(80, 200, 120, static_cast<int>(150 + player.beatAlpha * 105));
    } else {
        ofSetColor(60);
    }
    ofNoFill();
    ofSetLineWidth(2);
    ofDrawRectRounded(x, y, width, height, 8);
    ofFill();
    ofSetLineWidth(1);

    auto px = x + 20;
    auto py = y + 28;

    // Device name and number
    ofSetColor(255);
    ofDrawBitmapString(player.info.deviceName, px, py);

    ofSetColor(150);
    ofDrawBitmapString("#" + ofToString(player.info.deviceNumber), px + 100, py);

    // IP address
    ofSetColor(100);
    ofDrawBitmapString(player.info.ipAddress, px + 150, py);

    py += 30;

    // Status badges row
    float badgeX = px;
    if (player.status) {
        const auto& status = *player.status;

        // Playing status
        if (status.isPlaying) {
            drawStatusBadge(badgeX, py, "PLAYING", ofColor(80, 255, 120));
            badgeX += 80;
        } else {
            drawStatusBadge(badgeX, py, "STOPPED", ofColor(100));
            badgeX += 80;
        }

        // Master badge
        if (status.isMaster) {
            drawStatusBadge(badgeX, py, "MASTER", ofColor(255, 200, 80));
            badgeX += 75;
        }

        // Sync badge
        if (status.isSynced) {
            if (status.isBpmOnlySynced) {
                drawStatusBadge(badgeX, py, "BPM-SYNC", ofColor(100, 180, 255));
            } else {
                drawStatusBadge(badgeX, py, "SYNC", ofColor(100, 180, 255));
            }
            badgeX += 75;
        }

        // On-Air badge
        if (status.isOnAir) {
            drawStatusBadge(badgeX, py, "ON-AIR", ofColor(255, 80, 80));
            badgeX += 70;
        }

        // At cue badge
        if (status.isAtCue) {
            drawStatusBadge(badgeX, py, "CUE", ofColor(255, 150, 50));
        }
    } else {
        ofSetColor(80);
        ofDrawBitmapString("Waiting for status...", px, py);
    }

    py += 30;

    // BPM info
    if (player.status) {
        const auto& status = *player.status;

        // Effective BPM (large)
        ofSetColor(100, 255, 150);
        ofDrawBitmapString("BPM: " + ofToString(status.effectiveBpm, 2), px, py);

        // Track BPM
        ofSetColor(100);
        ofDrawBitmapString("Track: " + ofToString(status.bpm, 1), px + 140, py);

        // Pitch percentage
        const auto pitchSign = (status.pitchPercent >= 0) ? "+" : "";
        ofSetColor(150);
        ofDrawBitmapString("Pitch: " + std::string(pitchSign) + ofToString(status.pitchPercent, 2) + "%",
                          px + 260, py);

        py += 28;

        // Beat indicators
        drawBeatIndicators(px, py, status.beatWithinBar, player.beatAlpha);

        // Beat number in track
        ofSetColor(80);
        ofDrawBitmapString("Beat #" + ofToString(status.beatNumber), px + 180, py + 15);

        // Track info
        if (status.isTrackLoaded) {
            ofSetColor(100);
            ofDrawBitmapString("Track: " + ofToString(status.trackNumber) +
                             " (rbx:" + ofToString(status.rekordboxId) + ")",
                             px + 320, py + 15);
        }
    } else if (player.lastBeat) {
        // Fallback to beat info if no status yet
        const auto& beat = *player.lastBeat;
        ofSetColor(100, 255, 150);
        ofDrawBitmapString("BPM: " + ofToString(beat.bpm, 2), px, py);

        py += 28;
        drawBeatIndicators(px, py, beat.beatWithinBar, player.beatAlpha);
    } else {
        ofSetColor(60);
        ofDrawBitmapString("Waiting for beat data...", px, py);
    }
}

void ofApp::drawBeatIndicators(float x, float y, int currentBeat, float alpha) {
    constexpr auto size = 28.0f;
    constexpr auto gap = 8.0f;

    for (int i = 1; i <= 4; ++i) {
        const auto bx = x + static_cast<float>(i - 1) * (size + gap);
        const auto active = (i == currentBeat);

        // Background
        ofSetColor(40);
        ofDrawRectRounded(bx, y, size, size, 4);

        // Fill
        if (active) {
            int r = (i == 1) ? 255 : 80;
            int g = (i == 1) ? 80 : 200;
            int b = (i == 1) ? 80 : 120;
            ofSetColor(r, g, b, static_cast<int>(150 + alpha * 105));
            ofDrawRectRounded(bx + 2, y + 2, size - 4, size - 4, 3);
        }

        // Number
        ofSetColor(active ? 255 : 70);
        ofDrawBitmapString(ofToString(i), bx + 10, y + 18);
    }
}

void ofApp::drawStatusBadge(float x, float y, std::string_view text, const ofColor& color) {
    const auto width = text.length() * 8.0f + 12.0f;
    const auto height = 18.0f;

    // Background
    ofSetColor(color.r / 4, color.g / 4, color.b / 4, 180);
    ofDrawRectRounded(x, y - 12, width, height, 3);

    // Border
    ofSetColor(color, 150);
    ofNoFill();
    ofDrawRectRounded(x, y - 12, width, height, 3);
    ofFill();

    // Text
    ofSetColor(color);
    ofDrawBitmapString(std::string(text), x + 6, y);
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    ofRemoveListener(beatLink.deviceUpdateEvent, this, &ofApp::onDeviceUpdate);
    ofRemoveListener(beatLink.masterChangedEvent, this, &ofApp::onMasterChanged);

    beatLink.stopVirtualCdj();
    beatLink.stop();
}

void ofApp::keyPressed(int key) {
    if (key == 'r' || key == 'R') {
        auto currentDevices = beatLink.getCurrentDevices();
        addLog("Refreshed: " + ofToString(currentDevices.size()) + " devices");
    } else if (key == 'v' || key == 'V') {
        if (beatLink.isVirtualCdjRunning()) {
            beatLink.stopVirtualCdj();
            addLog("VirtualCdj stopped", ofColor(255, 150, 80));
        } else {
            if (beatLink.startVirtualCdj()) {
                addLog("VirtualCdj started as #" + ofToString(beatLink.getVirtualCdjDeviceNumber()),
                      ofColor(100, 200, 255));
            } else {
                addLog("Failed to start VirtualCdj", ofColor(255, 80, 80));
            }
        }
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    auto it = players.find(beat.deviceNumber);
    if (it != players.end()) {
        it->second.lastBeat = beat;
        it->second.beatAlpha = 1.0f;
        it->second.lastUpdateTime = ofGetElapsedTimeMillis();
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    PlayerState state;
    state.info = device;
    state.lastUpdateTime = ofGetElapsedTimeMillis();
    players[device.deviceNumber] = state;

    addLog(device.deviceName + " #" + ofToString(device.deviceNumber) +
           " connected (" + device.ipAddress + ")", ofColor(100, 200, 255));
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    if (currentMaster && *currentMaster == device.deviceNumber) {
        currentMaster = std::nullopt;
    }
    players.erase(device.deviceNumber);

    addLog(device.deviceName + " #" + ofToString(device.deviceNumber) + " disconnected",
           ofColor(255, 150, 80));
}

void ofApp::onDeviceUpdate(ofxBeatLinkCdjStatus& status) {
    // Update player status
    auto it = players.find(status.deviceNumber);
    if (it != players.end()) {
        it->second.status = status;
        it->second.lastUpdateTime = ofGetElapsedTimeMillis();
    }
}

void ofApp::onMasterChanged(ofxBeatLinkCdjStatus& status) {
    auto prevMaster = currentMaster;
    currentMaster = status.deviceNumber;

    if (!prevMaster || *prevMaster != status.deviceNumber) {
        addLog(status.deviceName + " #" + ofToString(status.deviceNumber) + " is now MASTER",
               ofColor(255, 200, 80));
    }
}

void ofApp::addLog(std::string_view msg, const ofColor& color) {
    LogEntry entry;
    entry.timestamp = ofGetTimestampString("%H:%M:%S");
    entry.message = std::string(msg);
    entry.color = color;
    eventLog.emplace_front(entry);

    // Keep only MAX_LOG entries
    while (eventLog.size() > MAX_LOG) {
        eventLog.pop_back();
    }
}
