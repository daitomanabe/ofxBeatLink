#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetWindowTitle("ofxBeatLink - Timeline");

    // Register event listeners
    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    // Predefined device colors
    deviceColors[1] = ofColor(255, 100, 100);  // Red
    deviceColors[2] = ofColor(100, 255, 100);  // Green
    deviceColors[3] = ofColor(100, 100, 255);  // Blue
    deviceColors[4] = ofColor(255, 255, 100);  // Yellow

    // Start listening
    if (!beatLink.start()) {
        ofLogError("ofApp") << "Failed to start - check ports 50000/50001";
    }
}

void ofApp::update() {
    beatLink.update();
}

void ofApp::draw() {
    drawTimeline();

    if (showBpmGraph) {
        drawBpmGraph();
    }

    if (showBeatMarkers) {
        drawBeatMarkers();
    }

    drawLegend();

    // Instructions
    ofSetColor(60);
    ofDrawBitmapString("'b' toggle BPM graph | 'm' toggle beat markers | '+'/'-' zoom | 'c' clear | 'q' quit",
                      20, ofGetHeight() - 10);
}

void ofApp::drawTimeline() {
    float margin = 60;
    float timelineWidth = ofGetWidth() - margin * 2;
    float timelineY = 50;
    float timelineHeight = 30;

    // Timeline background
    ofSetColor(40);
    ofDrawRectangle(margin, timelineY, timelineWidth, timelineHeight);

    // Time markers
    uint64_t now = ofGetElapsedTimeMillis();
    uint64_t timeWindowMs = static_cast<uint64_t>(timelineSeconds * 1000);

    ofSetColor(80);
    for (int sec = 0; sec <= static_cast<int>(timelineSeconds); sec += 5) {
        float x = margin + (1.0f - static_cast<float>(sec) / timelineSeconds) * timelineWidth;
        ofDrawLine(x, timelineY, x, timelineY + timelineHeight);

        ofSetColor(60);
        ofDrawBitmapString("-" + ofToString(sec) + "s", x - 10, timelineY + timelineHeight + 15);
        ofSetColor(80);
    }

    // Current time marker
    ofSetColor(255, 200, 80);
    ofDrawLine(margin + timelineWidth, timelineY - 5, margin + timelineWidth, timelineY + timelineHeight + 5);

    // Title
    ofSetColor(150);
    ofDrawBitmapString("Timeline (" + ofToString(timelineSeconds, 0) + "s)", margin, timelineY - 10);
}

void ofApp::drawBpmGraph() {
    float margin = 60;
    float graphWidth = ofGetWidth() - margin * 2;
    float graphY = 120;
    float graphHeight = 150;

    // Graph background
    ofSetColor(30);
    ofDrawRectangle(margin, graphY, graphWidth, graphHeight);

    // Find BPM range
    double minBpm = 200, maxBpm = 60;
    for (const auto& sample : bpmHistory) {
        if (sample.bpm < minBpm) minBpm = sample.bpm;
        if (sample.bpm > maxBpm) maxBpm = sample.bpm;
    }

    // Add padding to range
    double bpmRange = maxBpm - minBpm;
    if (bpmRange < 10) {
        double center = (minBpm + maxBpm) / 2;
        minBpm = center - 5;
        maxBpm = center + 5;
        bpmRange = 10;
    }
    minBpm -= 2;
    maxBpm += 2;
    bpmRange = maxBpm - minBpm;

    // Grid lines
    ofSetColor(50);
    for (double bpm = std::ceil(minBpm / 10) * 10; bpm <= maxBpm; bpm += 10) {
        float y = graphY + graphHeight - ((bpm - minBpm) / bpmRange) * graphHeight;
        ofDrawLine(margin, y, margin + graphWidth, y);
        ofSetColor(70);
        ofDrawBitmapString(ofToString(static_cast<int>(bpm)), margin - 30, y + 4);
        ofSetColor(50);
    }

    // Draw BPM line
    if (bpmHistory.size() > 1) {
        uint64_t now = ofGetElapsedTimeMillis();
        uint64_t timeWindowMs = static_cast<uint64_t>(timelineSeconds * 1000);

        ofSetColor(100, 200, 255);
        ofNoFill();
        ofBeginShape();
        for (const auto& sample : bpmHistory) {
            if (now - sample.timestamp > timeWindowMs) continue;

            float t = 1.0f - static_cast<float>(now - sample.timestamp) / timeWindowMs;
            float x = margin + t * graphWidth;
            float y = graphY + graphHeight - ((sample.bpm - minBpm) / bpmRange) * graphHeight;
            ofVertex(x, y);
        }
        ofEndShape(false);
        ofFill();
    }

    // Current BPM
    if (!bpmHistory.empty()) {
        double currentBpm = bpmHistory.back().bpm;
        ofSetColor(255);
        ofDrawBitmapString("BPM: " + ofToString(currentBpm, 1), margin + graphWidth - 100, graphY + 20);
    }

    // Label
    ofSetColor(150);
    ofDrawBitmapString("BPM History", margin, graphY - 10);
}

void ofApp::drawBeatMarkers() {
    float margin = 60;
    float markerWidth = ofGetWidth() - margin * 2;
    float markerY = 290;
    float markerHeight = 60;

    // Background
    ofSetColor(30);
    ofDrawRectangle(margin, markerY, markerWidth, markerHeight);

    // Draw beat markers
    uint64_t now = ofGetElapsedTimeMillis();
    uint64_t timeWindowMs = static_cast<uint64_t>(timelineSeconds * 1000);

    for (const auto& beat : beatHistory) {
        if (now - beat.timestamp > timeWindowMs) continue;

        float t = 1.0f - static_cast<float>(now - beat.timestamp) / timeWindowMs;
        float x = margin + t * markerWidth;

        // Different height for downbeats
        float height = (beat.beatWithinBar == 1) ? markerHeight : markerHeight * 0.5f;
        float y = markerY + (markerHeight - height) / 2;

        // Color based on device
        ofColor color = beat.color;
        float age = static_cast<float>(now - beat.timestamp) / timeWindowMs;
        color.a = static_cast<int>(255 * (1.0f - age * 0.7f));

        ofSetColor(color);
        ofDrawRectangle(x - 2, y, 4, height);
    }

    // Label
    ofSetColor(150);
    ofDrawBitmapString("Beat Markers", margin, markerY - 10);
}

void ofApp::drawLegend() {
    float x = ofGetWidth() - 200;
    float y = 20;

    ofSetColor(100);
    ofDrawBitmapString("Devices:", x, y);
    y += 20;

    for (const auto& kv : deviceColors) {
        // Check if device is active
        bool active = false;
        for (const auto& beat : beatHistory) {
            if (beat.deviceNumber == kv.first) {
                active = true;
                break;
            }
        }

        if (active) {
            ofSetColor(kv.second);
            ofDrawRectangle(x, y - 10, 15, 15);
            ofSetColor(150);
            ofDrawBitmapString("Device #" + ofToString(kv.first), x + 20, y);
            y += 20;
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
    if (key == 'b' || key == 'B') {
        showBpmGraph = !showBpmGraph;
    } else if (key == 'm' || key == 'M') {
        showBeatMarkers = !showBeatMarkers;
    } else if (key == '+' || key == '=') {
        timelineSeconds = std::max(10.0f, timelineSeconds - 10.0f);
    } else if (key == '-' || key == '_') {
        timelineSeconds = std::min(120.0f, timelineSeconds + 10.0f);
    } else if (key == 'c' || key == 'C') {
        beatHistory.clear();
        bpmHistory.clear();
    } else if (key == 'q' || key == 'Q') {
        ofExit();
    }
}

ofColor ofApp::getDeviceColor(int deviceNumber) {
    auto it = deviceColors.find(deviceNumber);
    if (it != deviceColors.end()) {
        return it->second;
    }
    // Generate color for unknown devices
    ofColor color;
    color.setHsb((deviceNumber * 60) % 255, 200, 255);
    deviceColors[deviceNumber] = color;
    return color;
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    uint64_t now = ofGetElapsedTimeMillis();

    // Add to beat history
    BeatEvent event;
    event.timestamp = now;
    event.deviceNumber = beat.deviceNumber;
    event.deviceName = beat.deviceName;
    event.bpm = beat.bpm;
    event.beatWithinBar = beat.beatWithinBar;
    event.color = getDeviceColor(beat.deviceNumber);

    beatHistory.push_back(event);
    while (beatHistory.size() > MAX_HISTORY) {
        beatHistory.pop_front();
    }

    // Add to BPM history
    BpmSample sample;
    sample.timestamp = now;
    sample.bpm = beat.bpm;
    bpmHistory.push_back(sample);
    while (bpmHistory.size() > MAX_BPM_SAMPLES) {
        bpmHistory.pop_front();
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    ofLogNotice("ofApp") << "Device found: " << device.deviceName << " #" << device.deviceNumber;
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    ofLogNotice("ofApp") << "Device lost: " << device.deviceName << " #" << device.deviceNumber;
}
