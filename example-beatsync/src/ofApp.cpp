#include "ofApp.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <ranges>

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

    // C++20 std::lerp for smooth animation
    pulseRadius = std::lerp(pulseRadius, targetRadius, 0.15f);
    pulseAlpha *= 0.92f;

    // Decay beat indicators using C++20 ranges
    std::ranges::for_each(beatIndicators, [](auto& ind) { ind *= 0.9f; });

    // Calculate beat progress using nextBeatMs
    if (latestBeat.has_value() && latestBeat->nextBeatMs > 0) [[likely]] {
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
    if (latestBeat) [[likely]] {
        const auto& beat = *latestBeat;
        auto bpmStr = "BPM: " + ofToString(beat.bpm, 2);
        ofDrawBitmapString(bpmStr, centerX - 40, 40);

        auto deviceStr = beat.deviceName + " #" + ofToString(beat.deviceNumber);
        ofSetColor(150);
        ofDrawBitmapString(deviceStr, centerX - 50, 60);
    } else [[unlikely]] {
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
        // Progress arc using C++20 std::numbers::pi
        ofSetColor(100, 200, 255);
        ofSetLineWidth(4);
        ofPolyline arc;
        constexpr auto startAngle = -std::numbers::pi_v<float> / 2.0f;
        const auto endAngle = startAngle + beatProgress * std::numbers::pi_v<float> * 2.0f;
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

    // 4 Beat indicators at bottom using C++20 ranges with enumerate-like pattern
    constexpr auto indSpacing = 60.0f;
    const auto indY = ofGetHeight() - 100.0f;
    const auto indStartX = centerX - (indSpacing * 1.5f);

    for (std::size_t i : std::views::iota(0uz, NUM_BEATS)) {
        const auto x = indStartX + static_cast<float>(i) * indSpacing;
        const auto alpha = beatIndicators[i];

        // Background
        ofSetColor(40);
        ofDrawRectRounded(x - 20, indY - 20, 40, 40, 5);

        // Fill based on indicator value (beat 1 = red, others = green)
        const auto [r, g, b] = (i == 0) ? std::tuple{255, 80, 80} : std::tuple{80, 200, 120};
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
    if (auto idx = beat.beatWithinBar - 1; idx >= 0 && idx < static_cast<int>(NUM_BEATS)) [[likely]] {
        beatIndicators[static_cast<std::size_t>(idx)] = 1.0f;
    }
}

void ofApp::onDeviceFound([[maybe_unused]] ofxBeatLinkDevice& device) {
    ++deviceCount;
}

void ofApp::onDeviceLost([[maybe_unused]] ofxBeatLinkDevice& device) {
    deviceCount = std::max(0, deviceCount - 1);
}
