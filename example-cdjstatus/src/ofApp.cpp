#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("ofxBeatLink Beat Monitor");

    // Register event listeners
    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    // Start listening
    if (beatLink.start()) {
        addLog("Started listening for DJ Link devices...");
    } else {
        addLog("ERROR: Failed to start - check if ports 50000/50001 are available");
    }
}

void ofApp::update() {
    // Process events on main thread
    beatLink.update();

    // Update beat flash animations
    for (auto& pair : beatAlpha) {
        pair.second *= 0.92f;  // Decay
    }
}

void ofApp::draw() {
    float margin = 20;
    float panelWidth = (ofGetWidth() - margin * 3) / 2;
    float panelHeight = 180;

    // Title
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink Beat Monitor", margin, 25);

    ofSetColor(150);
    ofDrawBitmapString("Listening for Pioneer DJ Link devices on ports 50000/50001", margin, 45);

    // Draw player panels
    float y = 70;
    int col = 0;

    for (const auto& device : devices) {
        float x = margin + col * (panelWidth + margin);
        drawPlayerPanel(device.deviceNumber, x, y, panelWidth, panelHeight);
        col++;
        if (col >= 2) {
            col = 0;
            y += panelHeight + margin;
        }
    }

    // Show message if no devices
    if (devices.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No devices found.", margin, 80);
        ofDrawBitmapString("Make sure CDJ/XDJ equipment is connected to the same network.", margin, 100);
    }

    // Log area at bottom
    float logY = ofGetHeight() - margin - logMessages.size() * 14;
    ofSetColor(60);
    ofDrawLine(margin, logY - 10, ofGetWidth() - margin, logY - 10);

    ofSetColor(100);
    ofDrawBitmapString("Event Log:", margin, logY);
    logY += 16;

    for (const auto& msg : logMessages) {
        ofSetColor(80);
        ofDrawBitmapString(msg, margin, logY);
        logY += 14;
    }

    // Instructions
    ofSetColor(60);
    ofDrawBitmapString("Press 'r' to refresh, 'q' to quit", ofGetWidth() - 250, 25);
}

void ofApp::drawPlayerPanel(int deviceNumber, float x, float y, float width, float height) {
    // Find device info
    ofxBeatLinkDevice* devicePtr = nullptr;
    for (auto& d : devices) {
        if (d.deviceNumber == deviceNumber) {
            devicePtr = &d;
            break;
        }
    }
    if (!devicePtr) return;

    auto& device = *devicePtr;

    // Panel background
    ofSetColor(35);
    ofDrawRectRounded(x, y, width, height, 8);

    // Header with device name
    ofSetColor(80, 150, 255);
    ofDrawRectRounded(x, y, width, 30, 8);
    ofDrawRectangle(x, y + 22, width, 8);

    ofSetColor(255);
    std::string headerText = device.deviceName + " (#" + ofToString(device.deviceNumber) + ")";
    ofDrawBitmapString(headerText, x + 10, y + 20);

    float contentY = y + 45;
    float leftCol = x + 15;
    float rightCol = x + width / 2 + 10;

    // Get beat info
    auto beatIt = deviceBeats.find(deviceNumber);
    if (beatIt != deviceBeats.end()) {
        const auto& beat = beatIt->second;

        // Beat indicators
        float alpha = 0;
        auto alphaIt = beatAlpha.find(deviceNumber);
        if (alphaIt != beatAlpha.end()) {
            alpha = alphaIt->second;
        }
        drawBeatIndicators(beat.beatWithinBar, alpha, rightCol + 60, contentY - 4);

        // BPM display
        ofSetColor(100, 255, 100);
        ofDrawBitmapString("BPM: " + ofToString(beat.bpm, 1), leftCol, contentY);
        contentY += 25;

        // Track BPM
        ofSetColor(150);
        ofDrawBitmapString("Track BPM: " + ofToString(beat.trackBpm, 1), leftCol, contentY);
        contentY += 25;

        // Pitch
        ofSetColor(180);
        std::string pitchStr = (beat.pitchPercent >= 0 ? "+" : "") + ofToString(beat.pitchPercent, 2) + "%";
        ofDrawBitmapString("Pitch: " + pitchStr, leftCol, contentY);
        contentY += 25;

        // Next beat timing
        ofSetColor(120);
        ofDrawBitmapString("Next beat: " + ofToString(beat.nextBeatMs) + "ms", leftCol, contentY);
        ofDrawBitmapString("Next bar: " + ofToString(beat.nextBarMs) + "ms", rightCol, contentY);
    } else {
        ofSetColor(100);
        ofDrawBitmapString("Waiting for beat data...", leftCol, contentY);
    }

    // Device info
    contentY = y + height - 25;
    ofSetColor(80);
    ofDrawBitmapString("IP: " + device.ipAddress, leftCol, contentY);
}

void ofApp::drawBeatIndicators(int beatWithinBar, float alpha, float x, float y) {
    for (int i = 1; i <= 4; i++) {
        float circleX = x + (i - 1) * 22;
        bool isActive = (i == beatWithinBar);

        if (isActive) {
            if (i == 1) {
                // Downbeat - red
                ofSetColor(255, 80, 80, 150 + 105 * alpha);
            } else {
                // Other beats - green
                ofSetColor(80, 255, 80, 150 + 105 * alpha);
            }
            ofDrawCircle(circleX, y, 8);
        } else {
            ofSetColor(60);
            ofNoFill();
            ofDrawCircle(circleX, y, 8);
            ofFill();
        }
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
        devices = beatLink.getCurrentDevices();
        addLog("Refreshed: " + ofToString(devices.size()) + " devices");
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    deviceBeats[beat.deviceNumber] = beat;
    beatAlpha[beat.deviceNumber] = 1.0f;

    // Log downbeats only
    if (beat.beatWithinBar == 1) {
        addLog("Beat: " + beat.deviceName + " BPM=" + ofToString(beat.bpm, 1));
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    devices.push_back(device);
    addLog("Found: " + device.deviceName + " (#" + ofToString(device.deviceNumber) + ")");
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    devices.erase(
        std::remove_if(devices.begin(), devices.end(),
            [&device](const ofxBeatLinkDevice& d) {
                return d.deviceNumber == device.deviceNumber;
            }),
        devices.end()
    );
    deviceBeats.erase(device.deviceNumber);
    beatAlpha.erase(device.deviceNumber);

    addLog("Lost: " + device.deviceName + " (#" + ofToString(device.deviceNumber) + ")");
}

void ofApp::addLog(const std::string& message) {
    std::string timestamp = ofGetTimestampString("%H:%M:%S");
    logMessages.push_back("[" + timestamp + "] " + message);
    while (logMessages.size() > MAX_LOG_MESSAGES) {
        logMessages.pop_front();
    }
}
