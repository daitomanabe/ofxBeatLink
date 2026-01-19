#include "ofApp.h"
#include <algorithm>
#include <ranges>

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("ofxBeatLink - Multi Device");

    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    if (beatLink.start()) [[likely]] {
        addLog("Started listening on ports 50000/50001");
    } else [[unlikely]] {
        addLog("ERROR: Failed to start");
    }
}

void ofApp::update() {
    beatLink.update();

    // Decay animations using C++20 ranges with structured bindings
    for (auto& [deviceNum, state] : devices) {
        state.beatAlpha *= 0.9f;
    }
}

void ofApp::draw() {
    // Title
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - Multi Device Monitor", 30, 30);

    ofSetColor(120);
    ofDrawBitmapString("Connected: " + ofToString(devices.size()) + " devices", 30, 50);

    // Device panels - 2x2 grid
    constexpr auto marginX = 40.0f;
    constexpr auto marginY = 80.0f;
    constexpr auto gapX = 30.0f;
    constexpr auto gapY = 20.0f;

    // Draw 4 slots using C++20 views::iota
    for (int slot : std::views::iota(0, MAX_DEVICES)) {
        const auto col = slot % 2;
        const auto row = slot / 2;
        const auto x = marginX + static_cast<float>(col) * (PANEL_WIDTH + gapX);
        const auto y = marginY + static_cast<float>(row) * (PANEL_HEIGHT + gapY);
        const auto deviceNum = slot + 1;

        // C++20 contains() for map lookup
        if (devices.contains(deviceNum)) [[likely]] {
            drawDevicePanel(devices.at(deviceNum), x, y, PANEL_WIDTH, PANEL_HEIGHT);
        } else [[unlikely]] {
            drawEmptySlot(deviceNum, x, y, PANEL_WIDTH, PANEL_HEIGHT);
        }
    }

    // Event log
    const auto logY = marginY + 2 * (PANEL_HEIGHT + gapY) + 20;
    ofSetColor(80);
    ofDrawLine(30, logY, ofGetWidth() - 30, logY);

    ofSetColor(100);
    ofDrawBitmapString("Event Log:", 30, logY + 20);

    auto logLineY = logY + 40;
    for (const auto& msg : logMessages) {
        ofSetColor(80);
        ofDrawBitmapString(msg, 30, logLineY);
        logLineY += 15;
    }
}

void ofApp::drawEmptySlot(int deviceNum, float x, float y, float width, float height) {
    ofSetColor(35);
    ofDrawRectRounded(x, y, width, height, 8);

    ofSetColor(60);
    ofNoFill();
    ofSetLineWidth(1);
    ofDrawRectRounded(x, y, width, height, 8);
    ofFill();

    ofSetColor(80);
    ofDrawBitmapString("Device #" + ofToString(deviceNum), x + 20, y + 30);
    ofSetColor(50);
    ofDrawBitmapString("Not connected", x + 20, y + 55);
}

void ofApp::drawDevicePanel(const DeviceState& state, float x, float y, float width, float height) {
    // Background with beat flash
    const auto flashIntensity = state.beatAlpha * 0.3f;
    ofSetColor(static_cast<int>(35 + flashIntensity * 50),
               static_cast<int>(35 + flashIntensity * 30), 35);
    ofDrawRectRounded(x, y, width, height, 8);

    // Border
    if (state.beatAlpha > 0.1f) [[likely]] {
        ofSetColor(100, 200, 255, static_cast<int>(100 + state.beatAlpha * 155));
    } else [[unlikely]] {
        ofSetColor(70);
    }
    ofNoFill();
    ofSetLineWidth(2);
    ofDrawRectRounded(x, y, width, height, 8);
    ofFill();
    ofSetLineWidth(1);

    auto px = x + 20;
    auto py = y + 30;

    // Device name and number
    ofSetColor(255);
    ofDrawBitmapString(state.info.deviceName + " #" + ofToString(state.info.deviceNumber), px, py);

    // IP address
    ofSetColor(120);
    ofDrawBitmapString(state.info.ipAddress, px + 200, py);

    py += 30;

    if (state.lastBeat) [[likely]] {
        const auto& beat = *state.lastBeat;

        // BPM (large)
        ofSetColor(100, 255, 150);
        ofDrawBitmapString(ofToString(beat.bpm, 2) + " BPM", px, py);

        // Track BPM
        ofSetColor(100);
        ofDrawBitmapString("Track: " + ofToString(beat.trackBpm, 1), px + 150, py);

        py += 25;

        // Pitch
        ofSetColor(150);
        const auto pitchSign = (beat.pitchPercent >= 0) ? "+" : "";
        ofDrawBitmapString("Pitch: " + std::string(pitchSign) + ofToString(beat.pitchPercent, 2) + "%", px, py);

        py += 30;

        // Beat indicators using C++20 views::iota
        auto indX = px;
        constexpr auto size = 25.0f;
        constexpr auto gap = 10.0f;

        for (int i : std::views::iota(1, 5)) {
            const auto isActive = (i == beat.beatWithinBar);

            if (isActive) {
                // C++20 ternary with structured binding alternative
                const auto [r, g, b] = (i == 1) ? std::tuple{255, 80, 80} : std::tuple{80, 255, 120};
                ofSetColor(r, g, b, static_cast<int>(150 + state.beatAlpha * 105));
                ofDrawRectRounded(indX, py, size, size, 4);
            } else {
                ofSetColor(50);
                ofDrawRectRounded(indX, py, size, size, 4);
            }

            // Number
            ofSetColor(isActive ? 255 : 80);
            ofDrawBitmapString(ofToString(i), indX + 9, py + 17);

            indX += size + gap;
        }

        // Next beat/bar timing
        py += 40;
        ofSetColor(80);
        ofDrawBitmapString("Next Beat: " + ofToString(beat.nextBeatMs) + "ms", px, py);
        ofDrawBitmapString("Next Bar: " + ofToString(beat.nextBarMs) + "ms", px + 180, py);

    } else [[unlikely]] {
        ofSetColor(60);
        ofDrawBitmapString("Waiting for beat data...", px, py);
    }
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    beatLink.stop();
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    // C++20 contains() check
    if (devices.contains(beat.deviceNumber)) [[likely]] {
        auto& state = devices[beat.deviceNumber];
        state.lastBeat = beat;
        state.beatAlpha = 1.0f;
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    // C++20 designated initializers
    devices[device.deviceNumber] = DeviceState{.info = device};

    addLog("Found: " + device.deviceName + " #" + ofToString(device.deviceNumber) +
           " (" + device.ipAddress + ")");
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    devices.erase(device.deviceNumber);
    addLog("Lost: " + device.deviceName + " #" + ofToString(device.deviceNumber));
}

void ofApp::addLog(std::string_view msg) {
    auto timestamp = ofGetTimestampString("%H:%M:%S");
    logMessages.emplace_back("[" + timestamp + "] " + std::string(msg));

    // C++20 std::erase_if alternative: keep only MAX_LOG entries
    while (logMessages.size() > MAX_LOG) {
        logMessages.pop_front();
    }
}
