#include "ofApp.h"
#include <algorithm>
#include <ranges>

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(25);
    ofSetWindowTitle("ofxBeatLink - OSC Output");

    // Setup OSC sender
    oscSender.setup(oscHost, oscPort);

    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);

    if (beatLink.start()) [[likely]] {
        addOscLog("/status", "Started listening");
    } else [[unlikely]] {
        addOscLog("/error", "Failed to start");
    }

    lastSecondTime = ofGetElapsedTimef();
}

void ofApp::update() {
    beatLink.update();

    // Update messages per second (C++20 [[likely]]/[[unlikely]])
    if (auto now = ofGetElapsedTimef(); now - lastSecondTime >= 1.0f) [[unlikely]] {
        messagesPerSecond = static_cast<float>(messageCountLastSecond);
        messageCountLastSecond = 0;
        lastSecondTime = now;
    }

    // Decay log entry alphas using C++20 ranges
    std::ranges::for_each(oscLog, [](auto& entry) { entry.alpha *= 0.995f; });
}

void ofApp::draw() {
    auto y = 30.0f;

    // Title
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - OSC Output", 30, y);
    y += 30;

    // OSC settings
    ofSetColor(100, 200, 255);
    ofDrawBitmapString("OSC Target: " + oscHost + ":" + ofToString(oscPort), 30, y);

    // Status indicator
    ofSetColor(80, 255, 120);
    ofDrawCircle(ofGetWidth() - 50, y - 5, 6);
    ofSetColor(150);
    ofDrawBitmapString("Sending", ofGetWidth() - 120, y);

    y += 25;

    // Statistics
    ofSetColor(150);
    ofDrawBitmapString("Messages sent: " + ofToString(messagesSent), 30, y);
    ofDrawBitmapString("Rate: " + ofToString(messagesPerSecond, 1) + " msg/sec", 250, y);
    ofDrawBitmapString("Devices: " + ofToString(deviceCount), 450, y);
    y += 40;

    // OSC address reference
    ofSetColor(80);
    ofDrawLine(30, y, ofGetWidth() - 30, y);
    y += 20;

    ofSetColor(120);
    ofDrawBitmapString("OSC Addresses:", 30, y);
    y += 20;

    ofSetColor(80);
    ofDrawBitmapString("/beatlink/beat      [device:i] [bpm:f] [beat:i] [nextBeatMs:i] [nextBarMs:i]", 30, y);
    y += 18;
    ofDrawBitmapString("/beatlink/device/found   [device:i] [name:s] [ip:s]", 30, y);
    y += 18;
    ofDrawBitmapString("/beatlink/device/lost    [device:i] [name:s]", 30, y);
    y += 35;

    // OSC Monitor
    ofSetColor(80);
    ofDrawLine(30, y, ofGetWidth() - 30, y);
    y += 5;

    ofSetColor(40);
    ofDrawRectRounded(25, y, ofGetWidth() - 50, 280, 5);

    y += 25;
    ofSetColor(100);
    ofDrawBitmapString("OSC Monitor:", 35, y);
    y += 25;

    // Log entries using C++20 structured bindings
    for (const auto& [address, args, alpha] : oscLog) {
        // Address
        ofSetColor(100, 200, 255, static_cast<int>(50 + alpha * 205));
        ofDrawBitmapString(address, 35, y);

        // Arguments
        ofSetColor(200, 200, 200, static_cast<int>(50 + alpha * 205));
        ofDrawBitmapString(args, 280, y);

        y += 16;
    }

    // Instructions
    ofSetColor(60);
    ofDrawBitmapString("Press 'q' to quit", 30, ofGetHeight() - 20);
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    beatLink.stop();
}

void ofApp::keyPressed(int key) {
    if (key == 'q' || key == 'Q') [[unlikely]] {
        ofExit();
    }
}

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    ofxOscMessage msg;
    msg.setAddress("/beatlink/beat");
    msg.addIntArg(beat.deviceNumber);
    msg.addFloatArg(static_cast<float>(beat.bpm));
    msg.addIntArg(beat.beatWithinBar);
    msg.addIntArg(static_cast<int>(beat.nextBeatMs));
    msg.addIntArg(static_cast<int>(beat.nextBarMs));
    oscSender.sendMessage(msg);

    ++messagesSent;
    ++messageCountLastSecond;

    // Log using string building
    auto args = ofToString(beat.deviceNumber) + " " +
                ofToString(beat.bpm, 2) + " " +
                ofToString(beat.beatWithinBar) + " " +
                ofToString(beat.nextBeatMs) + " " +
                ofToString(beat.nextBarMs);
    addOscLog("/beatlink/beat", args);
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    ++deviceCount;

    ofxOscMessage msg;
    msg.setAddress("/beatlink/device/found");
    msg.addIntArg(device.deviceNumber);
    msg.addStringArg(device.deviceName);
    msg.addStringArg(device.ipAddress);
    oscSender.sendMessage(msg);

    ++messagesSent;
    ++messageCountLastSecond;

    auto args = ofToString(device.deviceNumber) + " \"" +
                device.deviceName + "\" " + device.ipAddress;
    addOscLog("/beatlink/device/found", args);
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    deviceCount = std::max(0, deviceCount - 1);

    ofxOscMessage msg;
    msg.setAddress("/beatlink/device/lost");
    msg.addIntArg(device.deviceNumber);
    msg.addStringArg(device.deviceName);
    oscSender.sendMessage(msg);

    ++messagesSent;
    ++messageCountLastSecond;

    auto args = ofToString(device.deviceNumber) + " \"" + device.deviceName + "\"";
    addOscLog("/beatlink/device/lost", args);
}

void ofApp::addOscLog(std::string_view address, std::string_view args) {
    // C++20 designated initializers with emplace
    oscLog.emplace_front(OscLogEntry{
        .address = std::string(address),
        .args = std::string(args),
        .alpha = 1.0f
    });

    // Keep only MAX_OSC_LOG entries
    while (oscLog.size() > MAX_OSC_LOG) {
        oscLog.pop_back();
    }
}
