#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(30);
    ofBackground(20);
    ofSetWindowTitle("ofxBeatLink TCP Test");
    
    log("=== TCP Test Example ===");
    
    // Register device events
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    
    // Step 1: Start DeviceFinder/BeatFinder (mirror beat-link-trigger try-going-online)
    log("Starting DeviceFinder/BeatFinder...");
    if (beatLink.start()) {
        log("OK: Basic listener started");
    } else {
        log("ERROR: Failed to start basic listener");
        return;
    }
    
    // Step 2: Wait for at least one DJ Link device (like beat-link-trigger loop until getCurrentDevices)
    log("Waiting for DJ Link devices...");
    for (int i = 0; i < 200; ++i) {
        if (!beatLink.getCurrentDevices().empty()) break;
        ofSleepMillis(100);
    }
    if (beatLink.getCurrentDevices().empty()) {
        log("ERROR: No DJ Link devices found after 20s. Connect CDJs/mixer and retry.");
        return;
    }
    log("Found " + ofToString(beatLink.getCurrentDevices().size()) + " device(s)");
    
    // Step 3: Start VirtualCdj like beat-link-trigger (setUseStandardPlayerNumber then start() with no arg)
    log("Starting VirtualCdj...");
    auto& vcdj = beatlink::VirtualCdj::getInstance();
    vcdj.setDeviceName("ofxTcpTest");
    vcdj.setUseStandardPlayerNumber(true);  // "Use Real Player Number" -> 1-4

    if (vcdj.start()) {
        virtualCdjStarted = true;
        log("OK: VirtualCdj started as #" + ofToString((int)vcdj.getDeviceNumber()));
        
        // Debug: Show network info
        try {
            auto localAddr = vcdj.getLocalAddress();
            auto broadcastAddr = vcdj.getBroadcastAddress();
            log("  Local IP: " + localAddr.to_string());
            log("  Broadcast: " + broadcastAddr.to_string());
        } catch (const std::exception& e) {
            log("  Network info error: " + std::string(e.what()));
        }
        
        // Add update listener to see if we're receiving status
        updateListener = std::make_shared<beatlink::DeviceUpdateCallbacks>(
            [this](const beatlink::DeviceUpdate& update) {
                int player = update.getDeviceNumber();
                if (player > 6) return;  // Skip mixers
                
                // Check if it's a CdjStatus
                const auto* cdjStatus = dynamic_cast<const beatlink::CdjStatus*>(&update);
                if (cdjStatus) {
                    static std::set<int> loggedPlayers;
                    if (loggedPlayers.find(player) == loggedPlayers.end()) {
                        std::string info = "CdjStatus from #" + std::to_string(player);
                        info += " rekordboxId=" + std::to_string(cdjStatus->getRekordboxId());
                        info += " loaded=" + std::string(cdjStatus->isTrackLoaded() ? "yes" : "no");
                        ofLogNotice("TcpTest") << info;
                        loggedPlayers.insert(player);
                    }
                }
            }
        );
        vcdj.addUpdateListener(updateListener);
        log("Added VirtualCdj update listener");
    } else {
        log("ERROR: VirtualCdj failed to start");
        return;
    }
    
    // Step 4: Start MetadataFinder (beat-link-trigger: start-other-finders then actively-request-metadata)
    log("Starting MetadataFinder...");
    auto& metadataFinder = beatlink::data::MetadataFinder::getInstance();
    metadataFinder.start();
    metadataFinderStarted = metadataFinder.isRunning();
    if (metadataFinderStarted && vcdj.getDeviceNumber() >= 1 && vcdj.getDeviceNumber() <= 4) {
        metadataFinder.setPassive(false);  // actively request metadata from CDJs (like actively-request-metadata-from-cdj-3000s)
    }
    log(metadataFinderStarted ? "OK: MetadataFinder started" : "ERROR: MetadataFinder failed");
    
    // Step 5: Start ArtFinder
    log("Starting ArtFinder...");
    auto& artFinder = beatlink::data::ArtFinder::getInstance();
    artFinder.start();
    artFinderStarted = artFinder.isRunning();
    log(artFinderStarted ? "OK: ArtFinder started" : "ERROR: ArtFinder failed");
    
    // Step 6: Start WaveformFinder
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
    
    auto& vcdj = beatlink::VirtualCdj::getInstance();
    const bool haveStatus = virtualCdjStarted && !vcdj.getLatestStatus().empty();
    
    // Check VirtualCdj status (debug)
    static uint64_t lastStatusCheck = 0;
    uint64_t now = ofGetElapsedTimeMillis();
    if (virtualCdjStarted && (now - lastStatusCheck > 2000)) {
        lastStatusCheck = now;
        auto allStatus = vcdj.getLatestStatus();
        log("VirtualCdj has " + ofToString(allStatus.size()) + " status entries");
        
        for (const auto& status : allStatus) {
            if (status) {
                const auto* cdjStatus = dynamic_cast<const beatlink::CdjStatus*>(status.get());
                if (cdjStatus) {
                    std::string info = "  #" + std::to_string(cdjStatus->getDeviceNumber());
                    info += " rekordboxId=" + std::to_string(cdjStatus->getRekordboxId());
                    info += " loaded=" + std::string(cdjStatus->isTrackLoaded() ? "yes" : "no");
                    log(info);
                }
            }
        }
    }
    
    // Poll Finder data only when VirtualCdj has received at least one status.
    // Otherwise Metadata/Art/Waveform have no source; Finders also depend on status.
    if (!haveStatus) {
        return;
    }
    
    // Poll data for each detected device
    for (const auto& device : devices) {
        int player = device.deviceNumber;
        
        // Skip non-player devices (like mixers)
        if (player > 6) continue;
        
        auto& data = playerData[player];
        
        // Update metadata
        if (metadataFinderStarted) {
            try {
                auto& finder = beatlink::data::MetadataFinder::getInstance();
                auto metadata = finder.getLatestMetadataFor(player);
                if (metadata) {
                    data.title = metadata->getTitle();
                    auto artist = metadata->getArtist();
                    data.artist = artist ? artist->getLabel() : "";
                }
            } catch (const std::system_error& e) {
                static std::set<int> loggedMetadataErrors;
                if (loggedMetadataErrors.find(player) == loggedMetadataErrors.end()) {
                    log("Metadata system_error (player " + ofToString(player) + "): " 
                        + e.what() + " [code=" + ofToString(e.code().value()) + "]");
                    loggedMetadataErrors.insert(player);
                }
            } catch (const std::exception& e) {
                static std::set<int> loggedMetadataErrors2;
                if (loggedMetadataErrors2.find(player) == loggedMetadataErrors2.end()) {
                    log("Metadata error (player " + ofToString(player) + "): " + e.what());
                    loggedMetadataErrors2.insert(player);
                }
            }
        }
        
        // Update album art
        if (artFinderStarted && !data.hasArt) {
            try {
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
            } catch (const std::system_error& e) {
                static std::set<int> loggedArtErrors;
                if (loggedArtErrors.find(player) == loggedArtErrors.end()) {
                    log("Art system_error (player " + ofToString(player) + "): "
                        + e.what() + " [code=" + ofToString(e.code().value()) + "]");
                    loggedArtErrors.insert(player);
                }
            } catch (const std::exception& e) {
                static std::set<int> loggedArtErrors2;
                if (loggedArtErrors2.find(player) == loggedArtErrors2.end()) {
                    log("Art error (player " + ofToString(player) + "): " + e.what());
                    loggedArtErrors2.insert(player);
                }
            }
        }
        
        // Update waveform
        if (waveformFinderStarted && !data.hasWaveform) {
            try {
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
            } catch (const std::system_error& e) {
                static std::set<int> loggedWaveformErrors;
                if (loggedWaveformErrors.find(player) == loggedWaveformErrors.end()) {
                    log("Waveform system_error (player " + ofToString(player) + "): "
                        + e.what() + " [code=" + ofToString(e.code().value()) + "]");
                    loggedWaveformErrors.insert(player);
                }
            } catch (const std::exception& e) {
                static std::set<int> loggedWaveformErrors2;
                if (loggedWaveformErrors2.find(player) == loggedWaveformErrors2.end()) {
                    log("Waveform error (player " + ofToString(player) + "): " + e.what());
                    loggedWaveformErrors2.insert(player);
                }
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
    
    // Hint when no status yet
    if (virtualCdjStarted && beatlink::VirtualCdj::getInstance().getLatestStatus().empty()) {
        ofSetColor(255, 200, 100);
        ofDrawBitmapString("VirtualCdj is #1. Turn OFF real CDJ#1 so status (port 50002) reaches this app.", x, y);
        y += 14;
        ofDrawBitmapString("If CDJ#1 is on, use a different player number and turn that CDJ off.", x, y);
        y += 20;
    }
    
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
        auto& vcdj = beatlink::VirtualCdj::getInstance();
        if (updateListener) {
            vcdj.removeUpdateListener(updateListener);
        }
        vcdj.stop();
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
