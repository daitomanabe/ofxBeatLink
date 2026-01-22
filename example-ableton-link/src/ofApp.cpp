#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("ofxBeatLink - Ableton Link Bridge");

    // Register DJ Link event listeners
    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    // Start DJ Link
    if (!beatLink.start()) {
        ofLogError("ofApp") << "Failed to start DJ Link - check ports 50000/50001";
    }

#ifdef USE_ABLETON_LINK
    // Initialize Ableton Link
    link.setup(currentBpm);
    link.enableLink(linkEnabled);
    ofLogNotice("ofApp") << "Ableton Link initialized at " << currentBpm << " BPM";
#else
    ofLogWarning("ofApp") << "Ableton Link not enabled. Add ofxAbletonLink to use Link features.";
#endif
}

void ofApp::update() {
    beatLink.update();

    // Decay beat flash
    beatAlpha *= 0.9f;

    // Update beat phase based on time and BPM
    if (currentBpm > 0) {
        double msPerBeat = 60000.0 / currentBpm;
        beatPhase += (1000.0f / 60.0f) / msPerBeat;  // Assuming 60fps
        while (beatPhase >= 1.0f) {
            beatPhase -= 1.0f;
        }
    }

    updateLink();
}

void ofApp::updateLink() {
#ifdef USE_ABLETON_LINK
    if (!linkEnabled) return;

    // Get Link state
    connectedLinkPeers = link.getNumPeers();

    if (followDjLink && currentBpm > 0) {
        // Push DJ Link tempo to Ableton Link
        link.setTempo(currentBpm);

        // Sync beat phase on downbeats
        if (beatAlpha > 0.9f && currentBeat == 1) {
            link.setBeat(0);  // Reset to beat 0 on downbeat
        }
    } else {
        // Could pull tempo from Link and display it
        currentBpm = link.getTempo();
    }

    beatPhase = link.getPhase();
#endif
}

void ofApp::draw() {
    drawStatus();
    drawBeatVisualizer();

    // Instructions
    ofSetColor(60);
#ifdef USE_ABLETON_LINK
    ofDrawBitmapString("'l' toggle Link | 'f' toggle follow mode | 'q' quit", 20, ofGetHeight() - 20);
#else
    ofDrawBitmapString("Link not available - add ofxAbletonLink addon | 'q' quit", 20, ofGetHeight() - 20);
#endif
}

void ofApp::drawStatus() {
    float y = 30;
    float margin = 30;

    // Title
    ofSetColor(255);
    ofDrawBitmapString("DJ Link <-> Ableton Link Bridge", margin, y);
    y += 40;

    // DJ Link status
    ofSetColor(100, 200, 255);
    ofDrawBitmapString("== DJ Link ==", margin, y);
    y += 20;

    ofSetColor(connectedDjLinkDevices > 0 ? ofColor(100, 255, 100) : ofColor(100));
    ofDrawBitmapString("Devices: " + ofToString(connectedDjLinkDevices), margin, y);
    y += 18;

    ofSetColor(150);
    if (!masterDevice.empty()) {
        ofDrawBitmapString("Master: " + masterDevice, margin, y);
    } else {
        ofDrawBitmapString("Master: (none)", margin, y);
    }
    y += 18;

    ofSetColor(255, 200, 80);
    ofDrawBitmapString("BPM: " + ofToString(currentBpm, 2), margin, y);
    y += 18;

    ofSetColor(150);
    ofDrawBitmapString("Beat: " + ofToString(currentBeat) + "/4", margin, y);
    y += 40;

    // Ableton Link status
#ifdef USE_ABLETON_LINK
    ofSetColor(255, 100, 255);
    ofDrawBitmapString("== Ableton Link ==", margin, y);
    y += 20;

    ofSetColor(linkEnabled ? ofColor(100, 255, 100) : ofColor(255, 100, 100));
    ofDrawBitmapString("Status: " + std::string(linkEnabled ? "ENABLED" : "DISABLED"), margin, y);
    y += 18;

    ofSetColor(connectedLinkPeers > 0 ? ofColor(100, 255, 100) : ofColor(100));
    ofDrawBitmapString("Peers: " + ofToString(connectedLinkPeers), margin, y);
    y += 18;

    ofSetColor(150);
    ofDrawBitmapString("Mode: " + std::string(followDjLink ? "DJ -> Link" : "Link -> DJ"), margin, y);
    y += 18;

    ofSetColor(100, 200, 255);
    ofDrawBitmapString("Phase: " + ofToString(beatPhase, 2), margin, y);
#else
    ofSetColor(100);
    ofDrawBitmapString("== Ableton Link ==", margin, y);
    y += 20;
    ofDrawBitmapString("Not available", margin, y);
    y += 18;
    ofDrawBitmapString("Add ofxAbletonLink to enable", margin, y);
#endif
}

void ofApp::drawBeatVisualizer() {
    float centerX = ofGetWidth() * 0.65f;
    float centerY = ofGetHeight() * 0.5f;
    float radius = 100;

    // Outer circle
    ofSetColor(40);
    ofNoFill();
    ofSetLineWidth(2);
    ofDrawCircle(centerX, centerY, radius);
    ofFill();
    ofSetLineWidth(1);

    // Beat indicators (4 beats)
    for (int i = 0; i < 4; ++i) {
        float angle = (static_cast<float>(i) / 4.0f) * TWO_PI - HALF_PI;
        float x = centerX + std::cos(angle) * radius;
        float y = centerY + std::sin(angle) * radius;

        bool isActive = (i + 1 == currentBeat);
        bool isDownbeat = (i == 0);

        if (isActive) {
            if (isDownbeat) {
                ofSetColor(255, static_cast<int>(100 + beatAlpha * 155),
                          static_cast<int>(50 + beatAlpha * 50));
            } else {
                ofSetColor(static_cast<int>(100 + beatAlpha * 155), 255,
                          static_cast<int>(100 + beatAlpha * 155));
            }
            ofDrawCircle(x, y, 15 + beatAlpha * 5);
        } else {
            ofSetColor(60);
            ofNoFill();
            ofDrawCircle(x, y, 12);
            ofFill();
        }

        // Beat number
        ofSetColor(isActive ? 255 : 80);
        ofDrawBitmapString(ofToString(i + 1), x - 4, y + 4);
    }

    // Phase indicator (rotating line)
    float phaseAngle = beatPhase * TWO_PI - HALF_PI;
    float lineEndX = centerX + std::cos(phaseAngle) * (radius - 20);
    float lineEndY = centerY + std::sin(phaseAngle) * (radius - 20);

    ofSetColor(255, 200, 80, 200);
    ofSetLineWidth(3);
    ofDrawLine(centerX, centerY, lineEndX, lineEndY);
    ofSetLineWidth(1);

    // Center circle
    ofSetColor(30);
    ofDrawCircle(centerX, centerY, 25);
    ofSetColor(255, 200, 80);
    ofDrawBitmapString(ofToString(static_cast<int>(currentBpm)), centerX - 12, centerY + 4);
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    beatLink.stop();

#ifdef USE_ABLETON_LINK
    link.enableLink(false);
#endif
}

void ofApp::keyPressed(int key) {
    if (key == 'q' || key == 'Q') {
        ofExit();
    }
#ifdef USE_ABLETON_LINK
    else if (key == 'l' || key == 'L') {
        linkEnabled = !linkEnabled;
        link.enableLink(linkEnabled);
        ofLogNotice("ofApp") << "Link " << (linkEnabled ? "enabled" : "disabled");
    } else if (key == 'f' || key == 'F') {
        followDjLink = !followDjLink;
        ofLogNotice("ofApp") << "Mode: " << (followDjLink ? "DJ -> Link" : "Link -> DJ");
    }
#endif
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    currentBpm = beat.bpm;
    currentBeat = beat.beatWithinBar;
    beatAlpha = 1.0f;
    beatPhase = 0.0f;  // Reset phase on beat
    masterDevice = beat.deviceName;
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    connectedDjLinkDevices++;
    ofLogNotice("ofApp") << "DJ Link device found: " << device.deviceName;
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    connectedDjLinkDevices--;
    if (connectedDjLinkDevices < 0) connectedDjLinkDevices = 0;
    ofLogNotice("ofApp") << "DJ Link device lost: " << device.deviceName;
}
