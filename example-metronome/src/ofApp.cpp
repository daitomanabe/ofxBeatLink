#include "ofApp.h"
#include <cmath>

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("ofxBeatLink - Metronome");
    ofSetCircleResolution(64);

    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    beatLink.start();
}

void ofApp::update() {
    beatLink.update();

    // Decay beat flash
    beatFlash *= 0.85f;

    // Update pendulum
    pendulumAngle = pendulumAngle + (targetAngle - pendulumAngle) * 0.15f;

    // Calculate beat progress
    if (latestBeat.has_value() && lastBeatTime > 0) {
        auto now = ofGetElapsedTimeMillis();
        auto elapsed = now - lastBeatTime;
        auto beatDuration = 60000.0f / latestBeat->bpm;
        beatProgress = std::fmod(static_cast<float>(elapsed) / beatDuration, 1.0f);
    }
}

void ofApp::draw() {
    drawBpmDisplay();

    if (!latestBeat.has_value()) {
        drawInstructions();
        return;
    }

    if (showPendulum) drawPendulum();
    if (showCircle) drawBeatCircle();
    if (showBars) drawBeatBars();
    drawCountdown();
    drawDeviceInfo();

    // Key hints
    ofSetColor(60);
    ofDrawBitmapString("Keys: [1] Pendulum  [2] Circle  [3] Bars  [q] Quit", 30, ofGetHeight() - 20);
}

void ofApp::drawBpmDisplay() {
    float centerX = ofGetWidth() / 2.0f;

    if (latestBeat.has_value()) {
        // Large BPM display
        ofSetColor(255);
        std::string bpmStr = ofToString(latestBeat->bpm, 1);

        // Calculate text width for centering (approximate)
        float textWidth = bpmStr.length() * 20;
        ofDrawBitmapString(bpmStr, centerX - textWidth / 2, 60);

        ofSetColor(100);
        ofDrawBitmapString("BPM", centerX - 15, 80);
    } else {
        ofSetColor(100);
        ofDrawBitmapString("Waiting for devices...", centerX - 90, 60);
    }
}

void ofApp::drawPendulum() {
    const float pi = 3.14159265358979f;
    float centerX = ofGetWidth() / 2.0f;
    float pivotY = 120;
    float length = 180;
    float maxAngle = pi / 6.0f;  // 30 degrees

    // Calculate pendulum position based on beat progress
    float angle = std::sin(beatProgress * pi * 2.0f) * maxAngle;

    float bobX = centerX + std::sin(angle) * length;
    float bobY = pivotY + std::cos(angle) * length;

    // Draw pivot
    ofSetColor(80);
    ofDrawCircle(centerX, pivotY, 8);

    // Draw arm
    ofSetColor(100);
    ofSetLineWidth(3);
    ofDrawLine(centerX, pivotY, bobX, bobY);
    ofSetLineWidth(1);

    // Draw bob with beat flash
    int flashColor = static_cast<int>(beatFlash * 200);
    bool isDownbeat = latestBeat && latestBeat->beatWithinBar == 1;
    if (isDownbeat && beatFlash > 0.5f) {
        ofSetColor(255, 80 + flashColor, 80 + flashColor);
    } else {
        ofSetColor(80 + flashColor, 150 + flashColor / 2, 200 + flashColor / 4);
    }
    ofDrawCircle(bobX, bobY, 20 + beatFlash * 10);

    // Draw tick marks
    ofSetColor(60);
    for (int i = -3; i <= 3; ++i) {
        float tickAngle = (i / 3.0f) * maxAngle;
        float x1 = centerX + std::sin(tickAngle) * (length + 30);
        float y1 = pivotY + std::cos(tickAngle) * (length + 30);
        float x2 = centerX + std::sin(tickAngle) * (length + 40);
        float y2 = pivotY + std::cos(tickAngle) * (length + 40);
        ofDrawLine(x1, y1, x2, y2);
    }
}

void ofApp::drawBeatCircle() {
    const float pi = 3.14159265358979f;
    float centerX = ofGetWidth() / 2.0f;
    float centerY = 420;
    float radius = 80;

    // Outer ring
    ofNoFill();
    ofSetColor(60);
    ofSetLineWidth(2);
    ofDrawCircle(centerX, centerY, radius);

    // Progress arc
    ofSetColor(100, 180, 255);
    ofSetLineWidth(4);
    ofPolyline arc;
    float startAngle = -pi / 2.0f;
    float endAngle = startAngle + beatProgress * pi * 2.0f;
    for (float a = startAngle; a <= endAngle; a += 0.05f) {
        arc.addVertex(centerX + std::cos(a) * radius, centerY + std::sin(a) * radius);
    }
    arc.draw();
    ofSetLineWidth(1);
    ofFill();

    // Center circle with beat number
    bool isDownbeat = latestBeat && latestBeat->beatWithinBar == 1;
    if (isDownbeat) {
        ofSetColor(255, 80, 80, static_cast<int>(100 + beatFlash * 155));
    } else {
        ofSetColor(80, 150, 255, static_cast<int>(100 + beatFlash * 155));
    }
    ofDrawCircle(centerX, centerY, 40 + beatFlash * 15);

    // Beat number
    ofSetColor(255);
    if (latestBeat) {
        ofDrawBitmapString(ofToString(latestBeat->beatWithinBar),
                          centerX - 4, centerY + 5);
    }

    // Beat markers around circle
    for (int i = 0; i < 4; ++i) {
        float angle = -pi / 2.0f + i * pi / 2.0f;
        float mx = centerX + std::cos(angle) * (radius + 20);
        float my = centerY + std::sin(angle) * (radius + 20);

        bool isActive = latestBeat && (latestBeat->beatWithinBar == i + 1);
        if (isActive) {
            ofSetColor(255, 200, 100);
        } else {
            ofSetColor(60);
        }
        ofDrawCircle(mx, my, 6);
    }
}

void ofApp::drawBeatBars() {
    float startX = 100;
    float y = 550;
    float barWidth = (ofGetWidth() - 200) / 4.0f;
    float barHeight = 40;
    float gap = 10;

    for (int i = 0; i < 4; ++i) {
        float x = startX + i * (barWidth + gap);
        bool isActive = latestBeat && (latestBeat->beatWithinBar == i + 1);

        // Background
        ofSetColor(40);
        ofDrawRectRounded(x, y, barWidth - gap, barHeight, 5);

        // Fill
        if (isActive) {
            int r, g, b;
            if (i == 0) {
                r = 255; g = 80; b = 80;
            } else {
                r = 80; g = 200; b = 120;
            }
            ofSetColor(r, g, b, static_cast<int>(150 + beatFlash * 105));
            ofDrawRectRounded(x + 3, y + 3, barWidth - gap - 6, barHeight - 6, 4);
        }

        // Number
        ofSetColor(isActive ? 255 : 80);
        ofDrawBitmapString(ofToString(i + 1), x + barWidth / 2 - 8, y + barHeight / 2 + 5);
    }
}

void ofApp::drawCountdown() {
    if (!latestBeat.has_value()) return;

    float x = 30;
    float y = ofGetHeight() - 100;

    ofSetColor(100);
    ofDrawBitmapString("Next Beat:", x, y);
    ofSetColor(150);
    ofDrawBitmapString(ofToString(latestBeat->nextBeatMs) + " ms", x + 90, y);

    ofSetColor(100);
    ofDrawBitmapString("Next Bar:", x, y + 20);
    ofSetColor(150);
    ofDrawBitmapString(ofToString(latestBeat->nextBarMs) + " ms", x + 90, y + 20);

    // Pitch
    ofSetColor(100);
    ofDrawBitmapString("Pitch:", x, y + 40);
    auto pitchSign = (latestBeat->pitchPercent >= 0) ? "+" : "";
    ofSetColor(150);
    ofDrawBitmapString(std::string(pitchSign) + ofToString(latestBeat->pitchPercent, 2) + "%",
                      x + 90, y + 40);
}

void ofApp::drawDeviceInfo() {
    if (!latestBeat.has_value()) return;

    float x = ofGetWidth() - 200;
    float y = ofGetHeight() - 100;

    ofSetColor(100);
    ofDrawBitmapString("Device:", x, y);
    ofSetColor(150);
    ofDrawBitmapString(latestBeat->deviceName + " #" + ofToString(latestBeat->deviceNumber),
                      x, y + 20);

    ofSetColor(100);
    ofDrawBitmapString("Devices:", x, y + 50);
    ofSetColor(150);
    ofDrawBitmapString(ofToString(deviceCount), x + 70, y + 50);
}

void ofApp::drawInstructions() {
    float centerX = ofGetWidth() / 2.0f;
    float centerY = ofGetHeight() / 2.0f;

    ofSetColor(80);
    ofDrawBitmapString("Connect CDJ/XDJ to the same network", centerX - 140, centerY);
    ofDrawBitmapString("Waiting for beat data...", centerX - 90, centerY + 30);
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    beatLink.stop();
}

void ofApp::keyPressed(int key) {
    if (key == 'q' || key == 'Q') {
        ofExit();
    } else if (key == '1') {
        showPendulum = !showPendulum;
    } else if (key == '2') {
        showCircle = !showCircle;
    } else if (key == '3') {
        showBars = !showBars;
    }
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    latestBeat = beat;
    lastBeatTime = ofGetElapsedTimeMillis();
    beatFlash = 1.0f;

    // Update pendulum target angle (alternates on each beat)
    const float pi = 3.14159265358979f;
    targetAngle = (beat.beatWithinBar % 2 == 1) ? (pi / 6.0f) : (-pi / 6.0f);
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    (void)device;
    ++deviceCount;
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    (void)device;
    deviceCount = std::max(0, deviceCount - 1);
}
