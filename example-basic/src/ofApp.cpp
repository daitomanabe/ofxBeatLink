#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(30);
    ofSetWindowTitle("ofxBeatLink Example");

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
        pair.second *= 0.9f;  // Decay
    }
}

void ofApp::draw() {
    float y = 30;
    float x = 30;

    // Title
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink Example", x, y);
    y += 20;

    ofSetColor(150);
    ofDrawBitmapString("Listening for Pioneer DJ Link devices on ports 50000/50001", x, y);
    y += 30;

    // Device list
    ofSetColor(255);
    ofDrawBitmapString("Detected Devices: " + ofToString(devices.size()), x, y);
    y += 25;

    for (const auto& device : devices) {
        ofSetColor(200);
        ofDrawBitmapString(device.deviceName + " (#" + ofToString(device.deviceNumber) + ")", x, y);
        ofDrawBitmapString("IP: " + device.ipAddress, x + 200, y);
        y += 20;

        // Show beat info for this device
        auto beatIt = deviceBeats.find(device.deviceNumber);
        if (beatIt != deviceBeats.end()) {
            const auto& beat = beatIt->second;

            // BPM
            ofSetColor(100, 255, 100);
            ofDrawBitmapString("BPM: " + ofToString(beat.bpm, 1), x + 20, y);

            // Beat indicators (4 circles)
            float circleX = x + 150;
            for (int i = 1; i <= 4; i++) {
                if (i == beat.beatWithinBar) {
                    // Active beat - flash color based on alpha
                    float alpha = beatAlpha[device.deviceNumber];
                    if (i == 1) {
                        ofSetColor(255, 50, 50, 100 + 155 * alpha);  // Red for downbeat
                    } else {
                        ofSetColor(50, 255, 50, 100 + 155 * alpha);  // Green for other beats
                    }
                    ofDrawCircle(circleX, y - 4, 8);
                } else {
                    ofSetColor(80);
                    ofNoFill();
                    ofDrawCircle(circleX, y - 4, 8);
                    ofFill();
                }
                circleX += 25;
            }

            // Pitch
            ofSetColor(150);
            std::string pitchStr = (beat.pitchPercent >= 0 ? "+" : "") + ofToString(beat.pitchPercent, 2) + "%";
            ofDrawBitmapString("Pitch: " + pitchStr, x + 280, y);

            y += 20;
        }
        y += 10;
    }

    if (devices.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No devices found. Make sure CDJ/XDJ/DJM is connected to the same network.", x + 20, y);
        y += 20;
    }

    // Log area
    y = ofGetHeight() - 30 - logMessages.size() * 15;
    ofSetColor(80);
    ofDrawLine(x, y - 10, ofGetWidth() - x, y - 10);

    ofSetColor(120);
    ofDrawBitmapString("Event Log:", x, y);
    y += 20;

    for (const auto& msg : logMessages) {
        ofSetColor(100);
        ofDrawBitmapString(msg, x, y);
        y += 15;
    }

    // Instructions
    ofSetColor(80);
    ofDrawBitmapString("Press 'r' to refresh devices, 'q' to quit", ofGetWidth() - 300, 30);
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
        addLog("Refreshed device list: " + ofToString(devices.size()) + " devices");
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    deviceBeats[beat.deviceNumber] = beat;
    beatAlpha[beat.deviceNumber] = 1.0f;  // Flash

    // Log downbeats
    if (beat.beatWithinBar == 1) {
        addLog("Beat: " + beat.deviceName + " (#" + ofToString(beat.deviceNumber) +
               ") BPM=" + ofToString(beat.bpm, 1));
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    devices.push_back(device);
    addLog("Device found: " + device.deviceName + " (#" + ofToString(device.deviceNumber) +
           ") at " + device.ipAddress);
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    // Remove from list
    devices.erase(
        std::remove_if(devices.begin(), devices.end(),
            [&device](const ofxBeatLinkDevice& d) {
                return d.deviceNumber == device.deviceNumber;
            }),
        devices.end()
    );
    deviceBeats.erase(device.deviceNumber);
    beatAlpha.erase(device.deviceNumber);

    addLog("Device lost: " + device.deviceName + " (#" + ofToString(device.deviceNumber) + ")");
}

void ofApp::addLog(const std::string& message) {
    std::string timestamp = ofGetTimestampString("%H:%M:%S");
    logMessages.push_back("[" + timestamp + "] " + message);
    while (logMessages.size() > MAX_LOG_MESSAGES) {
        logMessages.pop_front();
    }
}
