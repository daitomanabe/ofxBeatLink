#include "ofApp.h"
#include <algorithm>
#include <cmath>

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("ofxBeatLink - Beat Sync");
    ofSetCircleResolution(64);

    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    beatLink.start();
}

void ofApp::update() {
    beatLink.update();

    // Smooth animation (C++17 compatible lerp)
    pulseRadius = pulseRadius + (targetRadius - pulseRadius) * 0.15f;
    pulseAlpha *= 0.92f;

    // Decay beat indicators
    for (auto& ind : beatIndicators) {
        ind *= 0.9f;
    }

    // Calculate beat progress using nextBeatMs
    if (latestBeat.has_value() && latestBeat->nextBeatMs > 0) {
        if (auto now = ofGetElapsedTimeMillis(); lastBeatTime > 0) {
            auto elapsed = now - lastBeatTime;
            auto beatDuration = 60000.0f / latestBeat->bpm;
            beatProgress = std::fmod(static_cast<float>(elapsed) / beatDuration, 1.0f);
        }
    }
}

void ofApp::draw() {
    const auto centerX = ofGetWidth() / 2.0f;
    const auto centerY = ofGetHeight() / 2.0f - 50.0f;

    // BPM display
    ofSetColor(255);
    if (latestBeat) {
        const auto& beat = *latestBeat;
        auto bpmStr = "BPM: " + ofToString(beat.bpm, 2);
        ofDrawBitmapString(bpmStr, centerX - 40, 40);

        auto deviceStr = beat.deviceName + " #" + ofToString(beat.deviceNumber);
        ofSetColor(150);
        ofDrawBitmapString(deviceStr, centerX - 50, 60);
    } else {
        ofSetColor(100);
        ofDrawBitmapString("Waiting for devices...", centerX - 80, 40);
        ofDrawBitmapString("Connect CDJ/XDJ to the same network", centerX - 130, 60);
    }

    // Outer ring (progress indicator)
    ofNoFill();
    ofSetColor(60);
    ofSetLineWidth(3);
    ofDrawCircle(centerX, centerY, 150);

    if (latestBeat) {
        // Progress arc
        ofSetColor(100, 200, 255);
        ofSetLineWidth(4);
        ofPolyline arc;
        const float pi = 3.14159265358979f;
        const float startAngle = -pi / 2.0f;
        const float endAngle = startAngle + beatProgress * pi * 2.0f;
        for (auto a = startAngle; a <= endAngle; a += 0.05f) {
            arc.addVertex(centerX + std::cos(a) * 150, centerY + std::sin(a) * 150);
        }
        arc.draw();
    }

    // Pulse circle
    ofFill();
    const bool isDownbeat = latestBeat && latestBeat->beatWithinBar == 1;
    if (isDownbeat) {
        ofSetColor(255, 80, 80, static_cast<int>(50 + pulseAlpha * 150));
    } else {
        ofSetColor(80, 150, 255, static_cast<int>(50 + pulseAlpha * 150));
    }
    ofDrawCircle(centerX, centerY, pulseRadius);

    // Inner circle
    ofSetColor(40);
    ofDrawCircle(centerX, centerY, 60);

    // Beat number in center
    if (latestBeat) {
        ofSetColor(255);
        ofDrawBitmapString(ofToString(latestBeat->beatWithinBar), centerX - 4, centerY + 5);
    }

    // 4 Beat indicators at bottom
    constexpr auto indSpacing = 60.0f;
    const auto indY = ofGetHeight() - 100.0f;
    const auto indStartX = centerX - (indSpacing * 1.5f);

    for (std::size_t i = 0; i < NUM_BEATS; ++i) {
        const auto x = indStartX + static_cast<float>(i) * indSpacing;
        const auto alpha = beatIndicators[i];

        // Background
        ofSetColor(40);
        ofDrawRectRounded(x - 20, indY - 20, 40, 40, 5);

        // Fill based on indicator value (beat 1 = red, others = green)
        int r, g, b;
        if (i == 0) {
            r = 255; g = 80; b = 80;
        } else {
            r = 80; g = 200; b = 120;
        }
        ofSetColor(r, g, b, static_cast<int>(50 + alpha * 200));
        ofDrawRectRounded(x - 18, indY - 18, 36, 36, 4);

        // Beat number label
        ofSetColor(255, static_cast<int>(150 + alpha * 105));
        ofDrawBitmapString(ofToString(i + 1), x - 4, indY + 5);
    }

    // Timing info
    if (latestBeat) {
        const auto& beat = *latestBeat;
        ofSetColor(120);
        ofDrawBitmapString("Next Beat: " + ofToString(beat.nextBeatMs) + " ms", 30, ofGetHeight() - 40);
        ofDrawBitmapString("Next Bar: " + ofToString(beat.nextBarMs) + " ms", 30, ofGetHeight() - 20);

        // Pitch
        auto pitchSign = (beat.pitchPercent >= 0) ? "+" : "";
        ofDrawBitmapString("Pitch: " + std::string(pitchSign) + ofToString(beat.pitchPercent, 2) + "%",
                          ofGetWidth() - 150, ofGetHeight() - 40);
    }

    // Device count
    ofSetColor(80);
    ofDrawBitmapString("Devices: " + ofToString(deviceCount), ofGetWidth() - 150, 30);
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    beatLink.stop();
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    latestBeat = beat;
    lastBeatTime = ofGetElapsedTimeMillis();

    // Trigger pulse animation
    targetRadius = 130.0f;
    pulseAlpha = 1.0f;
    targetRadius = 100.0f;

    // Light up the corresponding beat indicator
    int idx = beat.beatWithinBar - 1;
    if (idx >= 0 && idx < static_cast<int>(NUM_BEATS)) {
        beatIndicators[static_cast<std::size_t>(idx)] = 1.0f;
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    (void)device;  // Unused
    ++deviceCount;
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    (void)device;  // Unused
    deviceCount = std::max(0, deviceCount - 1);
}
