#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(0);
    ofSetWindowTitle("BPM Display");

    // Load fonts - use default if custom font not available
    fontBpm.load(OF_TTF_MONO, 120);
    fontInfo.load(OF_TTF_MONO, 24);

    // Register event listeners
    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    // Start listening
    if (!beatLink.start()) {
        ofLogError("ofApp") << "Failed to start - check ports 50000/50001";
    }
}

void ofApp::update() {
    beatLink.update();

    // Decay beat flash
    beatAlpha *= 0.85f;

    // Update BPM from latest beat
    auto beat = beatLink.getLatestBeat();
    if (beat.bpm > 0) {
        currentBpm = beat.bpm;
        masterDeviceName = beat.deviceName;
    }
}

void ofApp::draw() {
    // Background with subtle beat flash
    float bgFlash = beatAlpha * 0.1f;
    ofBackground(static_cast<int>(bgFlash * 30), static_cast<int>(bgFlash * 30), static_cast<int>(bgFlash * 40));

    drawBpmDisplay();
    drawBeatIndicators();

    // Connection status
    ofSetColor(80);
    std::string status = "Devices: " + ofToString(connectedDevices);
    if (!masterDeviceName.empty()) {
        status += " | Master: " + masterDeviceName;
    }
    fontInfo.drawString(status, 20, ofGetHeight() - 20);

    // Instructions
    ofSetColor(40);
    fontInfo.drawString("Press 'f' for fullscreen | 'q' to quit", ofGetWidth() - 380, ofGetHeight() - 20);
}

void ofApp::drawBpmDisplay() {
    std::string bpmStr;
    if (currentBpm > 0) {
        bpmStr = ofToString(currentBpm, 1);
    } else {
        bpmStr = "---.-";
    }

    // Center the BPM text
    float textWidth = fontBpm.stringWidth(bpmStr);
    float textHeight = fontBpm.stringHeight(bpmStr);
    float x = (ofGetWidth() - textWidth) / 2;
    float y = (ofGetHeight() + textHeight) / 2 - 40;

    // Draw BPM with beat flash
    int brightness = static_cast<int>(200 + beatAlpha * 55);
    ofSetColor(brightness, brightness, brightness);
    fontBpm.drawString(bpmStr, x, y);

    // "BPM" label
    ofSetColor(100);
    fontInfo.drawString("BPM", (ofGetWidth() - fontInfo.stringWidth("BPM")) / 2, y + 50);
}

void ofApp::drawBeatIndicators() {
    float centerX = ofGetWidth() / 2;
    float y = ofGetHeight() / 2 + 100;
    float spacing = 60;
    float startX = centerX - spacing * 1.5f;

    for (int i = 1; i <= 4; ++i) {
        float x = startX + (i - 1) * spacing;
        float radius = 20;

        bool isActive = (i == currentBeat);
        bool isDownbeat = (i == 1);

        if (isActive) {
            // Active beat - bright with flash
            if (isDownbeat) {
                // Downbeat - red/orange
                ofSetColor(255, static_cast<int>(100 + beatAlpha * 100),
                          static_cast<int>(50 + beatAlpha * 50),
                          static_cast<int>(200 + beatAlpha * 55));
            } else {
                // Other beats - green
                ofSetColor(static_cast<int>(100 + beatAlpha * 100), 255,
                          static_cast<int>(100 + beatAlpha * 100),
                          static_cast<int>(200 + beatAlpha * 55));
            }
            ofDrawCircle(x, y, radius + beatAlpha * 5);
        } else {
            // Inactive beat - dim outline
            ofSetColor(60);
            ofNoFill();
            ofSetLineWidth(2);
            ofDrawCircle(x, y, radius);
            ofFill();
            ofSetLineWidth(1);
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
    if (key == 'f' || key == 'F') {
        ofToggleFullscreen();
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::windowResized(int w, int h) {
    // Could reload fonts at different sizes here
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    currentBpm = beat.bpm;
    currentBeat = beat.beatWithinBar;
    beatAlpha = 1.0f;
    masterDeviceName = beat.deviceName;
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    connectedDevices++;
    ofLogNotice("ofApp") << "Device found: " << device.deviceName;
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    connectedDevices--;
    if (connectedDevices < 0) connectedDevices = 0;
    ofLogNotice("ofApp") << "Device lost: " << device.deviceName;
}
