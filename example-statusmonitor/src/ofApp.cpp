#include "ofApp.h"
#include <algorithm>

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("ofxBeatLink - Status Monitor");

    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    if (beatLink.start()) {
        addLog("Started monitoring on ports 50000/50001", ofColor(80, 200, 120));
    } else {
        addLog("ERROR: Failed to start", ofColor(255, 80, 80));
    }
}

void ofApp::update() {
    beatLink.update();

    const auto now = ofGetElapsedTimeMillis();

    // Update device states
    for (auto& kv : devices) {
        auto& status = kv.second;
        // Decay beat flash
        status.beatAlpha *= 0.9f;

        // Update playing status based on beat activity
        if (status.lastBeat) {
            status.isPlaying = (now - status.lastUpdateTime) < PLAYING_TIMEOUT_MS;
        }
    }
}

void ofApp::draw() {
    // Header
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - Status Monitor", 30, 30);

    ofSetColor(120);
    ofDrawBitmapString("Devices: " + ofToString(devices.size()), 30, 50);
    ofDrawBitmapString("Press 'r' to refresh | 'q' to quit", ofGetWidth() - 280, 30);

    // Device panels
    constexpr auto marginX = 30.0f;
    constexpr auto marginY = 70.0f;
    constexpr auto gapX = 30.0f;
    constexpr auto gapY = 20.0f;

    for (int slot = 0; slot < MAX_DEVICES; ++slot) {
        const auto col = slot % 2;
        const auto row = slot / 2;
        const auto x = marginX + static_cast<float>(col) * (PANEL_WIDTH + gapX);
        const auto y = marginY + static_cast<float>(row) * (PANEL_HEIGHT + gapY);
        const auto deviceNum = slot + 1;

        if (devices.find(deviceNum) != devices.end()) {
            drawDevicePanel(devices.at(deviceNum), x, y, PANEL_WIDTH, PANEL_HEIGHT);
        } else {
            drawEmptySlot(deviceNum, x, y, PANEL_WIDTH, PANEL_HEIGHT);
        }
    }

    // Event log section
    const auto logY = marginY + 2 * (PANEL_HEIGHT + gapY) + 10;

    ofSetColor(60);
    ofDrawLine(30, logY, ofGetWidth() - 30, logY);

    ofSetColor(30);
    ofDrawRectRounded(25, logY + 10, ofGetWidth() - 50, 220, 5);

    ofSetColor(100);
    ofDrawBitmapString("Event Log:", 35, logY + 30);

    auto logLineY = logY + 55;
    for (const auto& entry : eventLog) {
        ofSetColor(80);
        ofDrawBitmapString(entry.timestamp, 35, logLineY);

        ofSetColor(entry.color);
        ofDrawBitmapString(entry.message, 130, logLineY);

        logLineY += 17;
    }
}

void ofApp::drawEmptySlot(int deviceNum, float x, float y, float width, float height) {
    ofSetColor(30);
    ofDrawRectRounded(x, y, width, height, 8);

    ofSetColor(50);
    ofNoFill();
    ofDrawRectRounded(x, y, width, height, 8);
    ofFill();

    ofSetColor(60);
    ofDrawBitmapString("Device #" + ofToString(deviceNum) + " - Not Connected", x + 20, y + 30);
}

void ofApp::drawDevicePanel(const DeviceStatus& status, float x, float y, float width, float height) {
    // Background
    const auto flash = status.beatAlpha * 0.2f;
    ofSetColor(static_cast<int>(35 + flash * 60),
               static_cast<int>(35 + flash * 40), 35);
    ofDrawRectRounded(x, y, width, height, 8);

    // Border
    if (status.isPlaying) {
        ofSetColor(80, 200, 120, static_cast<int>(150 + status.beatAlpha * 105));
    } else {
        ofSetColor(60);
    }
    ofNoFill();
    ofSetLineWidth(2);
    ofDrawRectRounded(x, y, width, height, 8);
    ofFill();
    ofSetLineWidth(1);

    auto px = x + 20;
    auto py = y + 30;

    // Device name and number
    ofSetColor(255);
    ofDrawBitmapString(status.info.deviceName, px, py);

    ofSetColor(150);
    ofDrawBitmapString("#" + ofToString(status.info.deviceNumber), px + 120, py);

    // IP address
    ofSetColor(100);
    ofDrawBitmapString(status.info.ipAddress, px + 180, py);

    py += 35;

    // Status row
    if (status.isPlaying) {
        ofSetColor(80, 255, 120);
        ofDrawBitmapString("[PLAYING]", px, py);
    } else {
        ofSetColor(80);
        ofDrawBitmapString("[STOPPED]", px, py);
    }

    if (status.isMaster) {
        ofSetColor(255, 200, 80);
        ofDrawBitmapString("MASTER", px + 100, py);
    }

    if (status.isSynced) {
        ofSetColor(100, 180, 255);
        ofDrawBitmapString("SYNC", px + 170, py);
    }

    if (status.isOnAir) {
        ofSetColor(255, 80, 80);
        ofDrawBitmapString("ON-AIR", px + 230, py);
    }

    py += 35;

    // BPM info
    if (status.lastBeat) {
        const auto& beat = *status.lastBeat;

        ofSetColor(100, 255, 150);
        ofDrawBitmapString("BPM: " + ofToString(beat.bpm, 2), px, py);

        ofSetColor(100);
        ofDrawBitmapString("Track: " + ofToString(beat.trackBpm, 1), px + 130, py);

        // Pitch
        const auto pitchSign = (beat.pitchPercent >= 0) ? "+" : "";
        ofSetColor(150);
        ofDrawBitmapString("Pitch: " + std::string(pitchSign) + ofToString(beat.pitchPercent, 2) + "%",
                          px + 250, py);

        py += 30;

        // Beat indicators
        drawBeatIndicators(px, py, beat.beatWithinBar, status.beatAlpha);

        // Timing info
        ofSetColor(80);
        ofDrawBitmapString("Next Beat: " + ofToString(beat.nextBeatMs) + "ms", px + 180, py + 15);
        ofDrawBitmapString("Next Bar: " + ofToString(beat.nextBarMs) + "ms", px + 340, py + 15);
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

        // Fill with color
        if (active) {
            int r, g, b;
            if (i == 1) {
                r = 255; g = 80; b = 80;
            } else {
                r = 80; g = 200; b = 120;
            }
            ofSetColor(r, g, b, static_cast<int>(150 + alpha * 105));
            ofDrawRectRounded(bx + 2, y + 2, size - 4, size - 4, 3);
        }

        // Number
        ofSetColor(active ? 255 : 70);
        ofDrawBitmapString(ofToString(i), bx + 10, y + 18);
    }
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
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
    if (devices.find(beat.deviceNumber) != devices.end()) {
        auto& status = devices[beat.deviceNumber];
        const auto wasPlaying = status.isPlaying;

        status.lastBeat = beat;
        status.beatAlpha = 1.0f;
        status.lastUpdateTime = ofGetElapsedTimeMillis();
        status.isPlaying = true;

        // Log state changes
        if (!wasPlaying) {
            addLog(status.info.deviceName + " #" + ofToString(beat.deviceNumber) + " started playing",
                   ofColor(80, 200, 120));
        }
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    DeviceStatus status;
    status.info = device;
    status.lastUpdateTime = ofGetElapsedTimeMillis();
    devices[device.deviceNumber] = status;

    addLog(device.deviceName + " #" + ofToString(device.deviceNumber) +
           " connected (" + device.ipAddress + ")", ofColor(100, 200, 255));
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    devices.erase(device.deviceNumber);
    addLog(device.deviceName + " #" + ofToString(device.deviceNumber) + " disconnected",
           ofColor(255, 150, 80));
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
