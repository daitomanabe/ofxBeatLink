#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(30);
    ofBackground(20);
    ofSetWindowTitle("ofxBeatLink TCP Test");
    
    log("=== TCP Test Example ===");
    
    // Register device events
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    
    // Step 1: Start basic listener
    log("Starting DeviceFinder/BeatFinder...");
    if (beatLink.start()) {
        log("OK: Basic listener started");
    } else {
        log("ERROR: Failed to start basic listener");
        return;
    }
    
    // Step 2: Start VirtualCdj
    log("Starting VirtualCdj (device #5)...");
    auto& vcdj = beatlink::VirtualCdj::getInstance();
    vcdj.setDeviceName("ofxTcpTest");
    
    if (vcdj.start(5)) {
        virtualCdjStarted = true;
        log("OK: VirtualCdj started as #" + ofToString((int)vcdj.getDeviceNumber()));
    } else {
        log("ERROR: VirtualCdj failed to start");
        return;
    }
    
    // Step 3: Start MetadataFinder
    log("Starting MetadataFinder...");
    auto& metadataFinder = beatlink::data::MetadataFinder::getInstance();
    metadataFinder.start();
    metadataFinderStarted = metadataFinder.isRunning();
    log(metadataFinderStarted ? "OK: MetadataFinder started" : "ERROR: MetadataFinder failed");
    
    // Step 4: Start ArtFinder
    log("Starting ArtFinder...");
    auto& artFinder = beatlink::data::ArtFinder::getInstance();
    artFinder.start();
    artFinderStarted = artFinder.isRunning();
    log(artFinderStarted ? "OK: ArtFinder started" : "ERROR: ArtFinder failed");
    
    // Step 5: Start WaveformFinder
    log("Starting WaveformFinder...");
    auto& waveformFinder = beatlink::data::WaveformFinder::getInstance();
    waveformFinder.setFindDetails(false);  // Preview only
    waveformFinder.start();
    waveformFinderStarted = waveformFinder.isRunning();
    log(waveformFinderStarted ? "OK: WaveformFinder started" : "ERROR: WaveformFinder failed");
    
    log("Setup complete. Press 'r' to refresh data.");
}

void ofApp::update() {
    beatLink.update();
    
    // Poll data for each detected device
    for (const auto& device : devices) {
        int player = device.deviceNumber;
        auto& data = playerData[player];
        
        // Update metadata
        if (metadataFinderStarted) {
            auto& finder = beatlink::data::MetadataFinder::getInstance();
            auto metadata = finder.getLatestMetadataFor(player);
            if (metadata) {
                data.title = metadata->getTitle();
                auto artist = metadata->getArtist();
                data.artist = artist ? artist->getLabel() : "";
            }
        }
        
        // Update album art
        if (artFinderStarted && !data.hasArt) {
            auto& finder = beatlink::data::ArtFinder::getInstance();
            auto art = finder.getLatestArtFor(player);
            if (art) {
                auto decoded = art->decode();
                if (decoded && decoded->isValid()) {
                    ofPixels pixels;
                    pixels.setFromPixels(decoded->pixels.data(), 
                                         decoded->width, decoded->height, 
                                         OF_PIXELS_RGBA);
                    data.albumArt.setFromPixels(pixels);
                    data.hasArt = true;
                    log("Got album art for player " + ofToString(player));
                }
            }
        }
        
        // Update waveform
        if (waveformFinderStarted && !data.hasWaveform) {
            auto& finder = beatlink::data::WaveformFinder::getInstance();
            auto preview = finder.getLatestPreviewFor(player);
            if (preview && preview->getSegmentCount() > 0) {
                // Convert to image
                int w = preview->getSegmentCount();
                int h = 64;
                ofPixels pixels;
                pixels.allocate(w, h, OF_PIXELS_RGB);
                pixels.setColor(ofColor(0));
                
                int centerY = h / 2;
                for (int i = 0; i < w; i++) {
                    int heightVal = preview->segmentHeight(i, true);
                    float normalized = heightVal / 31.0f;
                    int barHeight = (int)(normalized * (h / 2 - 2));
                    
                    ofColor color;
                    if (preview->isColor()) {
                        auto c = preview->segmentColor(i, true);
                        color.set(c.r, c.g, c.b);
                    } else {
                        color.set(100, 180, 255);
                    }
                    
                    for (int y = 0; y < barHeight; y++) {
                        if (centerY + y < h) pixels.setColor(i, centerY + y, color);
                        if (centerY - y >= 0) pixels.setColor(i, centerY - y, color);
                    }
                }
                
                data.waveform.setFromPixels(pixels);
                data.hasWaveform = true;
                log("Got waveform for player " + ofToString(player));
            }
        }
    }
}

void ofApp::draw() {
    float x = 20;
    float y = 30;
    
    // Status
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink TCP Test", x, y);
    y += 25;
    
    ofSetColor(virtualCdjStarted ? ofColor(100, 255, 100) : ofColor(255, 100, 100));
    ofDrawBitmapString("VirtualCdj: " + std::string(virtualCdjStarted ? "OK" : "FAILED"), x, y);
    y += 15;
    
    ofSetColor(metadataFinderStarted ? ofColor(100, 255, 100) : ofColor(255, 100, 100));
    ofDrawBitmapString("MetadataFinder: " + std::string(metadataFinderStarted ? "OK" : "FAILED"), x, y);
    y += 15;
    
    ofSetColor(artFinderStarted ? ofColor(100, 255, 100) : ofColor(255, 100, 100));
    ofDrawBitmapString("ArtFinder: " + std::string(artFinderStarted ? "OK" : "FAILED"), x, y);
    y += 15;
    
    ofSetColor(waveformFinderStarted ? ofColor(100, 255, 100) : ofColor(255, 100, 100));
    ofDrawBitmapString("WaveformFinder: " + std::string(waveformFinderStarted ? "OK" : "FAILED"), x, y);
    y += 30;
    
    // Devices
    ofSetColor(255);
    ofDrawBitmapString("Devices: " + ofToString(devices.size()), x, y);
    y += 25;
    
    for (const auto& device : devices) {
        int player = device.deviceNumber;
        auto& data = playerData[player];
        
        // Device info
        ofSetColor(200);
        ofDrawBitmapString(device.deviceName + " #" + ofToString(player) + " (" + device.ipAddress + ")", x, y);
        y += 20;
        
        // Track info
        ofSetColor(255, 255, 100);
        ofDrawBitmapString("Title: " + (data.title.empty() ? "(none)" : data.title), x + 20, y);
        y += 15;
        ofSetColor(100, 255, 100);
        ofDrawBitmapString("Artist: " + (data.artist.empty() ? "(none)" : data.artist), x + 20, y);
        y += 20;
        
        // Album art
        if (data.hasArt && data.albumArt.isAllocated()) {
            ofSetColor(255);
            data.albumArt.draw(x + 20, y, 80, 80);
        } else {
            ofSetColor(50);
            ofDrawRectangle(x + 20, y, 80, 80);
            ofSetColor(100);
            ofDrawBitmapString("No Art", x + 35, y + 45);
        }
        
        // Waveform
        if (data.hasWaveform && data.waveform.isAllocated()) {
            ofSetColor(255);
            data.waveform.draw(x + 110, y, 300, 80);
        } else {
            ofSetColor(30);
            ofDrawRectangle(x + 110, y, 300, 80);
            ofSetColor(100);
            ofDrawBitmapString("No Waveform", x + 200, y + 45);
        }
        
        y += 100;
    }
    
    if (devices.empty()) {
        ofSetColor(100);
        ofDrawBitmapString("No devices found. Connect a CDJ/XDJ to the network.", x + 20, y);
        y += 20;
    }
    
    // Log area
    y = ofGetHeight() - 20 - logs.size() * 12;
    ofSetColor(60);
    ofDrawLine(x, y - 10, ofGetWidth() - x, y - 10);
    
    ofSetColor(80);
    for (const auto& msg : logs) {
        ofDrawBitmapString(msg, x, y);
        y += 12;
    }
    
    // Instructions
    ofSetColor(80);
    ofDrawBitmapString("'r' = refresh | 'c' = clear cache | 'q' = quit", ofGetWidth() - 350, 30);
}

void ofApp::exit() {
    log("Stopping...");
    
    if (waveformFinderStarted) {
        beatlink::data::WaveformFinder::getInstance().stop();
    }
    if (artFinderStarted) {
        beatlink::data::ArtFinder::getInstance().stop();
    }
    if (metadataFinderStarted) {
        beatlink::data::MetadataFinder::getInstance().stop();
    }
    if (virtualCdjStarted) {
        beatlink::VirtualCdj::getInstance().stop();
    }
    
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    beatLink.stop();
}

void ofApp::keyPressed(int key) {
    if (key == 'q' || key == 'Q') {
        ofExit();
    } else if (key == 'r' || key == 'R') {
        devices = beatLink.getCurrentDevices();
        log("Refreshed device list: " + ofToString(devices.size()) + " devices");
    } else if (key == 'c' || key == 'C') {
        // Clear cached data to force refresh
        for (auto& pair : playerData) {
            pair.second.hasArt = false;
            pair.second.hasWaveform = false;
        }
        log("Cleared cache - will refetch data");
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    devices.push_back(device);
    log("Device found: " + device.deviceName + " #" + ofToString(device.deviceNumber));
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    devices.erase(
        std::remove_if(devices.begin(), devices.end(),
            [&](const ofxBeatLinkDevice& d) { return d.deviceNumber == device.deviceNumber; }),
        devices.end()
    );
    playerData.erase(device.deviceNumber);
    log("Device lost: " + device.deviceName + " #" + ofToString(device.deviceNumber));
}

void ofApp::log(const std::string& msg) {
    std::string timestamp = ofGetTimestampString("%H:%M:%S");
    logs.push_back("[" + timestamp + "] " + msg);
    while (logs.size() > 15) {
        logs.pop_front();
    }
    ofLogNotice("TcpTest") << msg;
}
