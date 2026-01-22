#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(15);
    ofSetWindowTitle("ofxBeatLink - Waveform Display");

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

    // Update all players
    for (auto& kv : players) {
        auto& player = kv.second;
        player.beatAlpha *= 0.9f;
        updatePlayhead(player);
    }
}

void ofApp::draw() {
    // Title
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - Waveform Display", 20, 25);
    ofSetColor(80);
    ofDrawBitmapString("(Simulated waveform - enable VirtualRekordbox for real data)", 20, 45);

    // Draw waveforms for each player
    float margin = 30;
    float waveformHeight = 120;
    float gap = 40;
    float y = 80;

    int playerCount = 0;
    for (auto& kv : players) {
        auto& player = kv.second;
        float width = ofGetWidth() - margin * 2;

        // Player header
        ofSetColor(player.beatAlpha > 0.1f ? 255 : 150);
        std::string header = player.device.deviceName + " #" + ofToString(player.device.deviceNumber);
        if (player.bpm > 0) {
            header += " | " + ofToString(player.bpm, 1) + " BPM";
        }
        ofDrawBitmapString(header, margin, y);

        // Waveform area
        float waveY = y + 15;
        drawWaveform(player, margin, waveY, width, waveformHeight);
        drawBeatGrid(player, margin, waveY, width, waveformHeight);

        y += waveformHeight + gap + 20;
        playerCount++;
    }

    if (playerCount == 0) {
        ofSetColor(80);
        ofDrawBitmapString("No devices connected.", margin, 100);
        ofDrawBitmapString("Connect CDJ/XDJ equipment on the same network.", margin, 120);
    }

    // Instructions
    ofSetColor(50);
    ofDrawBitmapString("'q' to quit", ofGetWidth() - 100, ofGetHeight() - 20);
}

void ofApp::generateWaveform(PlayerWaveform& player) {
    // Generate simulated waveform data
    player.waveformData.clear();
    player.waveformData.resize(400);

    // Create a procedural waveform pattern
    for (size_t i = 0; i < player.waveformData.size(); ++i) {
        float t = static_cast<float>(i) / player.waveformData.size();

        // Combine multiple frequencies for interesting pattern
        float value = 0.0f;
        value += 0.5f * std::sin(t * 20.0f * M_PI);
        value += 0.3f * std::sin(t * 47.0f * M_PI);
        value += 0.2f * std::sin(t * 13.0f * M_PI);

        // Add some randomness
        value += 0.1f * ofRandomf();

        // Normalize to 0-1
        value = std::abs(value);
        value = std::min(1.0f, value);

        // Add beat emphasis every 4 "beats"
        int beatPos = static_cast<int>(t * 64) % 4;
        if (beatPos == 0) {
            value = std::min(1.0f, value + 0.3f);
        }

        player.waveformData[i] = value;
    }
}

void ofApp::updatePlayhead(PlayerWaveform& player) {
    if (player.bpm <= 0 || player.lastBeatTime == 0) {
        return;
    }

    uint64_t now = ofGetElapsedTimeMillis();
    uint64_t timeSinceBeat = now - player.lastBeatTime;

    // Calculate position based on BPM
    double msPerBeat = 60000.0 / player.bpm;
    double beatFraction = static_cast<double>(timeSinceBeat) / msPerBeat;

    // Move playhead (loop through waveform)
    float beatsPerWaveform = 32.0f;  // Assume 32 beats visible
    player.playheadPosition += static_cast<float>(beatFraction / beatsPerWaveform);

    // Keep in 0-1 range (loop)
    while (player.playheadPosition > 1.0f) {
        player.playheadPosition -= 1.0f;
    }

    player.lastBeatTime = now;
}

void ofApp::drawWaveform(const PlayerWaveform& player, float x, float y, float width, float height) {
    // Background
    ofSetColor(25);
    ofDrawRectangle(x, y, width, height);

    if (player.waveformData.empty()) {
        return;
    }

    float segmentWidth = width / player.waveformData.size();
    float centerY = y + height / 2;

    // Draw waveform segments
    for (size_t i = 0; i < player.waveformData.size(); ++i) {
        float value = player.waveformData[i];
        float segmentHeight = value * (height / 2 - 5);

        float segX = x + i * segmentWidth;

        // Color based on "frequency" (simulated)
        float hue = 0.6f - value * 0.4f;  // Blue to cyan
        ofColor color;
        color.setHsb(static_cast<int>(hue * 255), 200, static_cast<int>(150 + value * 105));

        // Highlight near playhead
        float distToPlayhead = std::abs((static_cast<float>(i) / player.waveformData.size()) - player.playheadPosition);
        if (distToPlayhead < 0.02f) {
            color = ofColor(255, 255, 255, 200);
        }

        ofSetColor(color);

        // Draw symmetric waveform
        ofDrawRectangle(segX, centerY - segmentHeight, segmentWidth - 1, segmentHeight * 2);
    }

    // Draw playhead
    float playheadX = x + player.playheadPosition * width;
    drawPlayhead(playheadX, y, height, player.beatAlpha);

    // Border
    ofSetColor(40);
    ofNoFill();
    ofDrawRectangle(x, y, width, height);
    ofFill();
}

void ofApp::drawPlayhead(float x, float y, float height, float alpha) {
    // Playhead line
    ofSetColor(255, 50, 50);
    ofSetLineWidth(2);
    ofDrawLine(x, y, x, y + height);
    ofSetLineWidth(1);

    // Playhead glow on beat
    if (alpha > 0.1f) {
        ofSetColor(255, 100, 100, static_cast<int>(alpha * 100));
        ofDrawRectangle(x - 5, y, 10, height);
    }
}

void ofApp::drawBeatGrid(const PlayerWaveform& player, float x, float y, float width, float height) {
    if (player.bpm <= 0) return;

    // Draw beat markers
    int beatsToShow = 32;
    float beatWidth = width / beatsToShow;

    for (int i = 0; i < beatsToShow; ++i) {
        float beatX = x + i * beatWidth;
        bool isDownbeat = (i % 4 == 0);

        if (isDownbeat) {
            ofSetColor(60, 60, 80);
        } else {
            ofSetColor(35, 35, 45);
        }
        ofDrawLine(beatX, y, beatX, y + height);
    }
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
    }
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    auto it = players.find(beat.deviceNumber);
    if (it != players.end()) {
        auto& player = it->second;
        player.lastBeat = beat;
        player.bpm = beat.bpm;
        player.beatWithinBar = beat.beatWithinBar;
        player.beatAlpha = 1.0f;
        player.lastBeatTime = ofGetElapsedTimeMillis();

        // Reset playhead on downbeat (bar start)
        if (beat.beatWithinBar == 1) {
            // Keep relative position within the waveform
            float barLength = 4.0f / 32.0f;  // 4 beats out of 32
            player.playheadPosition = std::fmod(player.playheadPosition, barLength);
        }
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    PlayerWaveform player;
    player.device = device;
    generateWaveform(player);
    players[device.deviceNumber] = player;

    ofLogNotice("ofApp") << "Device found: " << device.deviceName;
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    players.erase(device.deviceNumber);
    ofLogNotice("ofApp") << "Device lost: " << device.deviceName;
}
