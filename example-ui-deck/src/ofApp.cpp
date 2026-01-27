#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(0);  // Black background
    ofSetWindowTitle("ofxBeatLink - Deck UI");
    ofSetWindowShape(1920, 1080);

    // Load fonts
    fontLarge.load("C:/Users/okym/snippets/inconsolata/Inconsolata-Bold.ttf", 72, true, true);
    fontMedium.load("C:/Users/okym/snippets/inconsolata/Inconsolata-Bold.ttf", 36, true, true);
    fontSmall.load("C:/Users/okym/snippets/inconsolata/Inconsolata-Bold.ttf", 24, true, true);

    // Register event listeners
    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    // Start listening
    if (beatLink.start()) {
        ofLogNotice() << "Started listening for DJ Link devices...";
    } else {
        ofLogError() << "Failed to start - check if ports 50000/50001 are available";
    }
}

void ofApp::update() {
    // Process events on main thread
    beatLink.update();

    uint64_t currentTime = ofGetElapsedTimeMillis();
    
    // Update beat flash animations
    for (auto& pair : decks) {
        pair.second.beatAlpha *= 0.92f;  // Smooth decay
        
        // Check if device is still playing (last beat within 3 seconds)
        if (pair.second.lastBeatTime > 0 && 
            (currentTime - pair.second.lastBeatTime) > 3000) {
            // Device stopped playing
            pair.second.lastBeatTime = 0;
        }
    }
}

void ofApp::draw() {
    ofBackground(0);  // Black background
    
    float screenWidth = ofGetWidth();
    float screenHeight = ofGetHeight();
    float columnWidth = screenWidth / 2.0f;
    float padding = 40;

    // Draw center divider line
    ofSetColor(40);
    ofDrawLine(screenWidth / 2.0f, 0, screenWidth / 2.0f, screenHeight);

    // Get first two devices
    std::vector<int> deviceNumbers;
    for (const auto& pair : decks) {
        deviceNumbers.push_back(pair.first);
        if (deviceNumbers.size() >= 2) break;
    }

    // Draw deck columns
    for (int i = 0; i < 2; i++) {
        float x = i * columnWidth + padding;
        float y = padding;
        float w = columnWidth - padding * 2;
        float h = screenHeight - padding * 2;

        if (i < deviceNumbers.size()) {
            drawDeckColumn(deviceNumbers[i], x, y, w, h);
        } else {
            // Draw "No Device" placeholder
            ofSetColor(60);
            float centerY = screenHeight / 2.0f;
            std::string msg = "NO DEVICE";
            ofRectangle bounds = fontMedium.getStringBoundingBox(msg, 0, 0);
            fontMedium.drawString(msg, x + (w - bounds.width) / 2.0f, centerY);
        }
    }

    // Instructions at bottom
    ofSetColor(80);
    std::string instructions = "Press 'Q' to quit";
    ofRectangle bounds = fontSmall.getStringBoundingBox(instructions, 0, 0);
    fontSmall.drawString(instructions, (screenWidth - bounds.width) / 2.0f, screenHeight - 20);
}

void ofApp::drawDeckColumn(int deviceNumber, float x, float y, float width, float height) {
    auto deckIt = decks.find(deviceNumber);
    if (deckIt == decks.end()) return;

    const auto& deck = deckIt->second;
    const auto& device = deck.device;
    
    float currentY = y;

    // Device Name (Large)
    ofSetColor(255);
    std::string deviceName = device.deviceName;
    ofRectangle nameBounds = fontMedium.getStringBoundingBox(deviceName, 0, 0);
    fontMedium.drawString(deviceName, x + (width - nameBounds.width) / 2.0f, currentY + 50);
    currentY += 80;

    // Device Number badge
    ofSetColor(100);
    ofDrawRectangle(x + width - 60, currentY - 60, 50, 40);
    ofSetColor(255);
    std::string numStr = "#" + ofToString(device.deviceNumber);
    ofRectangle numBounds = fontSmall.getStringBoundingBox(numStr, 0, 0);
    fontSmall.drawString(numStr, x + width - 60 + (50 - numBounds.width) / 2.0f, currentY - 30);

    // IP Address
    ofSetColor(150);
    std::string ipStr = device.ipAddress;
    ofRectangle ipBounds = fontSmall.getStringBoundingBox(ipStr, 0, 0);
    fontSmall.drawString(ipStr, x + (width - ipBounds.width) / 2.0f, currentY);
    currentY += 60;

    // Divider line
    ofSetColor(40);
    ofDrawLine(x, currentY, x + width, currentY);
    currentY += 40;

    if (deck.beat.has_value()) {
        const auto& beat = deck.beat.value();
        float alpha = deck.beatAlpha;
        
        // Check if playing (received beat within last 3 seconds)
        bool isPlaying = deck.lastBeatTime > 0;

        // Playing indicator
        if (isPlaying) {
            ofSetColor(80, 255, 120);
            std::string playingLabel = "PLAYING";
            ofRectangle playingBounds = fontSmall.getStringBoundingBox(playingLabel, 0, 0);
            fontSmall.drawString(playingLabel, x + (width - playingBounds.width) / 2.0f, currentY);
            currentY += 40;
        }
        
        // BPM (Very Large)
        std::string bpmStr = ofToString(beat.bpm, 1);
        ofRectangle bpmBounds = fontLarge.getStringBoundingBox(bpmStr, 0, 0);
        
        // BPM background flash
        if (alpha > 0.5f) {
            ofSetColor(20, 20, 20, 100 * alpha);
            ofDrawRectangle(x, currentY - 20, width, 100);
        }
        
        ofSetColor(255);
        fontLarge.drawString(bpmStr, x + (width - bpmBounds.width) / 2.0f, currentY + 60);
        
        // BPM label
        ofSetColor(150);
        std::string bpmLabel = "BPM";
        ofRectangle labelBounds = fontSmall.getStringBoundingBox(bpmLabel, 0, 0);
        fontSmall.drawString(bpmLabel, x + (width - labelBounds.width) / 2.0f, currentY + 90);
        currentY += 110;
        
        // Track BPM (if different)
        if (abs(beat.bpm - beat.trackBpm) > 0.1f) {
            ofSetColor(120);
            std::string trackBpmLabel = "TRACK BPM: " + ofToString(beat.trackBpm, 1);
            ofRectangle trackBpmBounds = fontSmall.getStringBoundingBox(trackBpmLabel, 0, 0);
            fontSmall.drawString(trackBpmLabel, x + (width - trackBpmBounds.width) / 2.0f, currentY);
            currentY += 40;
        } else {
            currentY += 30;
        }

        // Beat Indicator (1-4)
        ofSetColor(150);
        std::string beatLabel = "BEAT";
        ofRectangle beatLabelBounds = fontSmall.getStringBoundingBox(beatLabel, 0, 0);
        fontSmall.drawString(beatLabel, x + (width - beatLabelBounds.width) / 2.0f, currentY);
        currentY += 30;

        drawBeatIndicator(x + width / 2.0f, currentY + 40, 40, beat.beatWithinBar, alpha);
        currentY += 120;

        // Next Beat Progress Bar
        ofSetColor(150);
        std::string progressLabel = "NEXT BEAT";
        ofRectangle progressLabelBounds = fontSmall.getStringBoundingBox(progressLabel, 0, 0);
        fontSmall.drawString(progressLabel, x + (width - progressLabelBounds.width) / 2.0f, currentY);
        currentY += 30;

        // Calculate progress (rough estimate based on BPM)
        float beatIntervalMs = (60.0f / beat.bpm) * 1000.0f;
        float progress = 1.0f - (beat.nextBeatMs / beatIntervalMs);
        progress = ofClamp(progress, 0.0f, 1.0f);
        
        ofColor progressColor = beat.beatWithinBar == 1 ? ofColor(255, 80, 80) : ofColor(80, 255, 80);
        drawProgressBar(x + 40, currentY, width - 80, 20, progress, progressColor);
        currentY += 50;

        // Pitch Meter
        ofSetColor(150);
        std::string pitchLabel = "PITCH";
        ofRectangle pitchLabelBounds = fontSmall.getStringBoundingBox(pitchLabel, 0, 0);
        fontSmall.drawString(pitchLabel, x + (width - pitchLabelBounds.width) / 2.0f, currentY);
        currentY += 30;

        drawPitchMeter(x + 40, currentY, width - 80, 40, beat.pitchPercent);
        currentY += 50;

        // Pitch value text
        std::string pitchStr = (beat.pitchPercent >= 0 ? "+" : "") + ofToString(beat.pitchPercent, 2) + "%";
        ofSetColor(255);
        ofRectangle pitchBounds = fontMedium.getStringBoundingBox(pitchStr, 0, 0);
        fontMedium.drawString(pitchStr, x + (width - pitchBounds.width) / 2.0f, currentY + 30);
        currentY += 60;

    } else {
        // No beat data yet
        ofSetColor(100);
        std::string msg = "Waiting for beat data...";
        ofRectangle msgBounds = fontSmall.getStringBoundingBox(msg, 0, 0);
        fontSmall.drawString(msg, x + (width - msgBounds.width) / 2.0f, currentY);
    }
}

void ofApp::drawProgressBar(float x, float y, float width, float height, float progress, ofColor color) {
    // Background
    ofSetColor(30);
    ofDrawRectangle(x, y, width, height);
    
    // Progress fill
    ofSetColor(color);
    ofDrawRectangle(x, y, width * progress, height);
    
    // Border
    ofNoFill();
    ofSetColor(80);
    ofDrawRectangle(x, y, width, height);
    ofFill();
}

void ofApp::drawPitchMeter(float x, float y, float width, float height, float pitchPercent) {
    // Background
    ofSetColor(30);
    ofDrawRectangle(x, y, width, height);
    
    // Center line
    float centerX = x + width / 2.0f;
    ofSetColor(60);
    ofDrawLine(centerX, y, centerX, y + height);
    
    // Pitch indicator
    // Assuming pitch range is -10% to +10%
    float normalizedPitch = ofClamp(pitchPercent / 10.0f, -1.0f, 1.0f);
    float indicatorX = centerX + (normalizedPitch * (width / 2.0f - 10));
    
    ofColor pitchColor = pitchPercent < 0 ? ofColor(100, 150, 255) : ofColor(255, 150, 100);
    ofSetColor(pitchColor);
    
    // Draw bar from center to indicator
    float barWidth = abs(indicatorX - centerX);
    float barX = min(centerX, indicatorX);
    ofDrawRectangle(barX, y + 2, barWidth, height - 4);
    
    // Border
    ofNoFill();
    ofSetColor(80);
    ofDrawRectangle(x, y, width, height);
    ofFill();
}

void ofApp::drawBeatIndicator(float centerX, float centerY, float size, int currentBeat, float alpha) {
    float spacing = size * 1.8f;
    float startX = centerX - (spacing * 3.0f / 2.0f);
    
    for (int i = 1; i <= 4; i++) {
        float x = startX + (i - 1) * spacing;
        
        if (i == currentBeat) {
            // Active beat - flash
            if (i == 1) {
                // Downbeat - red
                ofSetColor(255, 80, 80, 200 + 55 * alpha);
            } else {
                // Other beats - green
                ofSetColor(80, 255, 80, 200 + 55 * alpha);
            }
            ofDrawCircle(x, centerY, size);
            
            // Glow effect
            ofSetColor(255, 255, 255, 100 * alpha);
            ofDrawCircle(x, centerY, size * 1.3f);
        } else {
            // Inactive beat
            ofNoFill();
            ofSetColor(80);
            ofDrawCircle(x, centerY, size);
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
    if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    auto it = decks.find(beat.deviceNumber);
    if (it != decks.end()) {
        it->second.beat = beat;
        it->second.beatAlpha = 1.0f;  // Flash
        it->second.lastBeatTime = ofGetElapsedTimeMillis();  // Update playing timestamp
    }

    ofLogNotice("onBeat") << "Device #" << beat.deviceNumber
                          << " | BPM: " << beat.bpm
                          << " | Beat: " << beat.beatWithinBar << "/4"
                          << " | Pitch: " << beat.pitchPercent << "%";
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    DeckInfo info;
    info.device = device;
    decks[device.deviceNumber] = info;
    
    ofLogNotice("onDeviceFound") << "Device found: " << device.deviceName
                                  << " (#" << device.deviceNumber << ")"
                                  << " at " << device.ipAddress;
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    decks.erase(device.deviceNumber);

    ofLogNotice("onDeviceLost") << "Device lost: " << device.deviceName
                                 << " (#" << device.deviceNumber << ")";
}
