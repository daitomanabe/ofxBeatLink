#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(0);  // Black background
    
#ifdef ENABLE_TCP_FEATURES
    ofSetWindowTitle("ofxBeatLink - Deck UI (TCP Enabled)");
#else
    ofSetWindowTitle("ofxBeatLink - Deck UI");
#endif
    
    ofSetWindowShape(LayoutConfig::SCREEN_WIDTH, LayoutConfig::SCREEN_HEIGHT);

    // Load fonts
    fontLarge.load("C:/Users/okym/snippets/inconsolata/Inconsolata-Bold.ttf", 72, true, true);
    fontMedium.load("C:/Users/okym/snippets/inconsolata/Inconsolata-Bold.ttf", 36, true, true);
    fontSmall.load("C:/Users/okym/snippets/inconsolata/Inconsolata-Bold.ttf", 24, true, true);
    fontTitle.load("C:/Users/okym/snippets/inconsolata/Inconsolata-Bold.ttf", 48, true, true);
    
    // Setup FBO
    fbo.allocate(LayoutConfig::SCREEN_WIDTH, LayoutConfig::SCREEN_HEIGHT, GL_RGBA);
    fbo.begin();
    ofClear(0, 0, 0, 255);
    fbo.end();
    
    // Setup Spout
    spoutSender.init("ofxBeatLink_DeckUI");
    
    // Setup OSC (send to localhost:7000 by default)
    oscSender.setup("localhost", 7000);

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

#ifdef ENABLE_TCP_FEATURES
    // Start VirtualCdj for TCP-based features
    ofLogNotice() << "Starting VirtualCdj (this may take ~7 seconds)...";
    
    auto& vcdj = beatlink::VirtualCdj::getInstance();
    vcdj.setDeviceName("ofxDeckUI");
    
    // Use device number 5 to skip selfAssign wait
    if (vcdj.start(5)) {
        virtualCdjRunning = true;
        ofLogNotice() << "VirtualCdj started as device #" << (int)vcdj.getDeviceNumber();
        
        // Start MetadataFinder
        auto& metadataFinder = beatlink::data::MetadataFinder::getInstance();
        metadataFinder.start();
        metadataFinderRunning = metadataFinder.isRunning();
        ofLogNotice() << "MetadataFinder: " << (metadataFinderRunning ? "OK" : "FAILED");
        
        // Start WaveformFinder
        auto& waveformFinder = beatlink::data::WaveformFinder::getInstance();
        waveformFinder.setFindDetails(false);  // Preview only for performance
        waveformFinder.start();
        waveformFinderRunning = waveformFinder.isRunning();
        ofLogNotice() << "WaveformFinder: " << (waveformFinderRunning ? "OK" : "FAILED");
    } else {
        virtualCdjRunning = false;
        ofLogWarning() << "VirtualCdj failed to start - TCP features disabled";
    }
#endif
}

void ofApp::update() {
    // Process events on main thread
    beatLink.update();

    uint64_t currentTime = ofGetElapsedTimeMillis();
    
    // Update beat flash animations
    for (auto& pair : decks) {
        pair.second.beatAlpha *= 0.92f;  // Smooth decay
        
#ifndef ENABLE_TCP_FEATURES
        // Check if device is still playing (last beat within 3 seconds)
        if (pair.second.lastBeatTime > 0 && 
            (currentTime - pair.second.lastBeatTime) > 3000) {
            // Device stopped playing
            pair.second.lastBeatTime = 0;
            pair.second.displayData.isPlaying = false;
        } else if (pair.second.lastBeatTime > 0) {
            pair.second.displayData.isPlaying = true;
        }
#endif
    }

#ifdef ENABLE_TCP_FEATURES
    if (virtualCdjRunning) {
        // Update device status and metadata for all decks
        for (auto& pair : decks) {
            updateDeviceStatus(pair.first);
            updateTrackMetadata(pair.first);
            updateWaveform(pair.first);
            
            // Update display data from TCP data
            updateDisplayDataFromTCP(pair.second);
        }
    }
#endif
}

void ofApp::draw() {
    // Draw to FBO
    drawToFbo();
    
    // Send via Spout
    spoutSender.send(fbo.getTexture());
    
    // Draw to screen
    ofSetColor(255);
    fbo.draw(0, 0);
}

void ofApp::drawToFbo() {
    fbo.begin();
    ofClear(0, 0, 0, 255);
    ofBackground(0);
    
    const float screenWidth = LayoutConfig::SCREEN_WIDTH;
    const float screenHeight = LayoutConfig::SCREEN_HEIGHT;
    const float padding = LayoutConfig::PADDING;
    
    // Get device numbers (only playing devices)
    std::vector<int> deviceNumbers;
    for (const auto& pair : decks) {
        // Only show playing devices
        if (pair.second.displayData.isPlaying) {
            deviceNumbers.push_back(pair.first);
            if (deviceNumbers.size() >= LayoutConfig::MAX_DECKS) break;
        }
    }
    
    // Horizontal layout - all decks in a single row
    int cols = std::max(2, (int)deviceNumbers.size());  // Minimum 2 columns for layout
    int rows = 1;  // Always single row
    currentLayout = cols;
    
    float cellWidth = screenWidth / (float)cols;
    float cellHeight = screenHeight / (float)rows;
    
    // Draw grid dividers
    ofSetColor(40);
    for (int i = 1; i < cols; i++) {
        ofDrawLine(i * cellWidth, 0, i * cellWidth, screenHeight);
    }
    for (int i = 1; i < rows; i++) {
        ofDrawLine(0, i * cellHeight, screenWidth, i * cellHeight);
    }
    
    // Draw deck cells
    int maxCells = cols * rows;
    for (int i = 0; i < maxCells; i++) {
        int col = i % cols;
        int row = i / cols;
        
        float x = col * cellWidth + padding;
        float y = row * cellHeight + padding;
        float w = cellWidth - padding * 2;
        float h = cellHeight - padding * 2;
        
        if (i < deviceNumbers.size()) {
            drawDeckColumn(deviceNumbers[i], x, y, w, h);
        } else {
            // Draw "No Device" placeholder
            ofSetColor(60);
            float centerX = col * cellWidth + cellWidth / 2.0f;
            float centerY = row * cellHeight + cellHeight / 2.0f;
            std::string msg = "NO DEVICE";
            ofRectangle bounds = fontMedium.getStringBoundingBox(msg, 0, 0);
            fontMedium.drawString(msg, centerX - bounds.width / 2.0f, centerY);
        }
    }
    
    // Instructions at bottom
    ofSetColor(80);
    std::string deviceCount = ofToString(deviceNumbers.size());
    std::string instructions = "Press 'Q' to quit | Devices: " + deviceCount + " / " + ofToString(LayoutConfig::MAX_DECKS);
    ofRectangle bounds = fontSmall.getStringBoundingBox(instructions, 0, 0);
    fontSmall.drawString(instructions, (screenWidth - bounds.width) / 2.0f, screenHeight - 20);
    
    fbo.end();
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
    currentY += 40;

    // Status Badges (from DisplayData) - Centered
    const auto& displayData = deck.displayData;
    
    // Calculate total width of all badges
    std::vector<std::pair<std::string, ofColor>> badges;
    if (displayData.isPlaying) {
        badges.push_back({"PLAYING", ofColor(80, 255, 120)});
    } else {
        badges.push_back({"STOPPED", ofColor(100)});
    }
    if (displayData.isMaster) {
        badges.push_back({"MASTER", ofColor(255, 200, 80)});
    }
    if (displayData.isSynced) {
        badges.push_back({"SYNC", ofColor(100, 180, 255)});
    }
    if (displayData.isOnAir) {
        badges.push_back({"ON-AIR", ofColor(255, 80, 80)});
    }
    
    // Calculate total width
    float totalBadgeWidth = 0;
    for (const auto& badge : badges) {
        float badgeWidth = badge.first.length() * 8.0f + LayoutConfig::BADGE_PADDING * 2;
        totalBadgeWidth += badgeWidth;
    }
    totalBadgeWidth += (badges.size() - 1) * LayoutConfig::BADGE_SPACING;
    
    // Center badges
    float badgeX = x + (width - totalBadgeWidth) / 2.0f;
    float badgeY = currentY;
    
    for (const auto& badge : badges) {
        drawStatusBadge(badgeX, badgeY, badge.first, badge.second);
        float badgeWidth = badge.first.length() * 8.0f + LayoutConfig::BADGE_PADDING * 2;
        badgeX += badgeWidth + LayoutConfig::BADGE_SPACING;
    }
    
    currentY += LayoutConfig::BADGE_HEIGHT + LayoutConfig::SECTION_SPACING;
    
    // Album Art + Track Info (Horizontal Layout) - Centered
    // TODO: Re-enable when TCP issues are resolved
    // bool hasTrackInfo = !displayData.trackTitle.empty();
    // bool hasAlbumArt = displayData.hasAlbumArt && displayData.albumArt.isAllocated();
    // 
    // if (hasAlbumArt || hasTrackInfo) {
    //     const float artSize = LayoutConfig::ALBUM_ART_SIZE;
    //     const float trackInfoSpacing = LayoutConfig::TRACK_INFO_SPACING;
    //     
    //     // Calculate total width of the content block
    //     float totalContentWidth = artSize;
    //     float maxTextWidth = 0;
    //     
    //     if (hasTrackInfo) {
    //         std::string titleStr = displayData.trackTitle;
    //         if (titleStr.length() > 20) titleStr = titleStr.substr(0, 20) + "...";
    //         ofRectangle titleBounds = fontTitle.getStringBoundingBox(titleStr, 0, 0);
    //         maxTextWidth = titleBounds.width;
    //         
    //         if (!displayData.trackArtist.empty()) {
    //             std::string artistStr = displayData.trackArtist;
    //             if (artistStr.length() > 25) artistStr = artistStr.substr(0, 25) + "...";
    //             ofRectangle artistBounds = fontMedium.getStringBoundingBox(artistStr, 0, 0);
    //             maxTextWidth = std::max(maxTextWidth, artistBounds.width);
    //         }
    //         
    //         totalContentWidth += trackInfoSpacing + maxTextWidth;
    //     }
    //     
    //     // Center the entire content block
    //     float contentStartX = x + (width - totalContentWidth) / 2.0f;
    //     float artX = contentStartX;
    //     float textX = artX + artSize + trackInfoSpacing;
    //     
    //     // Draw album art
    //     if (hasAlbumArt) {
    //         drawAlbumArt(artX, currentY, artSize, displayData.albumArt);
    //     } else {
    //         // Placeholder
    //         ofSetColor(30);
    //         ofDrawRectangle(artX, currentY, artSize, artSize);
    //     }
    //     
    //     // Draw track info next to album art
    //     if (hasTrackInfo) {
    //         float textY = currentY + 30;
    //         
    //         // Title (Large)
    //         ofSetColor(255);
    //         std::string titleStr = displayData.trackTitle;
    //         if (titleStr.length() > 20) titleStr = titleStr.substr(0, 20) + "...";
    //         fontTitle.drawString(titleStr, textX, textY);
    //         textY += 50;
    //         
    //         // Artist
    //         if (!displayData.trackArtist.empty()) {
    //             ofSetColor(180);
    //             std::string artistStr = displayData.trackArtist;
    //             if (artistStr.length() > 25) artistStr = artistStr.substr(0, 25) + "...";
    //             fontMedium.drawString(artistStr, textX, textY);
    //         }
    //     }
    //     
    //     currentY += artSize + LayoutConfig::SECTION_SPACING;
    // }

    // Divider line
    ofSetColor(40);
    ofDrawLine(x, currentY, x + width, currentY);
    currentY += LayoutConfig::SECTION_SPACING;

    if (deck.beat.has_value()) {
        const auto& beat = deck.beat.value();
        float alpha = deck.beatAlpha;
        
#ifndef ENABLE_TCP_FEATURES
        // Check if playing (received beat within last 3 seconds)
        bool isPlaying = deck.lastBeatTime > 0;

        // Playing indicator (only when TCP features disabled)
        if (isPlaying) {
            ofSetColor(80, 255, 120);
            std::string playingLabel = "PLAYING";
            ofRectangle playingBounds = fontSmall.getStringBoundingBox(playingLabel, 0, 0);
            fontSmall.drawString(playingLabel, x + (width - playingBounds.width) / 2.0f, currentY);
            currentY += 40;
        }
#endif
        
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
        currentY += LayoutConfig::BPM_DISPLAY_HEIGHT;

        // Beat Indicator (1-4)
        ofSetColor(150);
        std::string beatLabel = "BEAT";
        ofRectangle beatLabelBounds = fontSmall.getStringBoundingBox(beatLabel, 0, 0);
        fontSmall.drawString(beatLabel, x + (width - beatLabelBounds.width) / 2.0f, currentY);
        currentY += LayoutConfig::ELEMENT_SPACING + 15;

        drawBeatIndicator(x + width / 2.0f, currentY + LayoutConfig::BEAT_CIRCLE_SIZE, 
                          LayoutConfig::BEAT_CIRCLE_SIZE, beat.beatWithinBar, alpha);
        currentY += LayoutConfig::BEAT_CIRCLE_SIZE * 2 + LayoutConfig::SECTION_SPACING;

        // Pitch Meter
        ofSetColor(150);
        std::string pitchLabel = "PITCH";
        ofRectangle pitchLabelBounds = fontSmall.getStringBoundingBox(pitchLabel, 0, 0);
        fontSmall.drawString(pitchLabel, x + (width - pitchLabelBounds.width) / 2.0f, currentY);
        currentY += LayoutConfig::ELEMENT_SPACING + 15;

        const float pitchMeterPadding = LayoutConfig::PADDING;
        drawPitchMeter(x + pitchMeterPadding, currentY, width - pitchMeterPadding * 2, 40, beat.pitchPercent);
        currentY += 50;

        // Pitch value text
        std::string pitchStr = (beat.pitchPercent >= 0 ? "+" : "") + ofToString(beat.pitchPercent, 2) + "%";
        ofSetColor(255);
        ofRectangle pitchBounds = fontMedium.getStringBoundingBox(pitchStr, 0, 0);
        fontMedium.drawString(pitchStr, x + (width - pitchBounds.width) / 2.0f, currentY + 30);
        currentY += 60;

        // Waveform (from DisplayData)
        // TODO: Re-enable when TCP issues are resolved
        // if (displayData.hasWaveform && displayData.waveformImage.isAllocated()) {
        //     ofSetColor(150);
        //     std::string waveLabel = "WAVEFORM";
        //     ofRectangle waveLabelBounds = fontSmall.getStringBoundingBox(waveLabel, 0, 0);
        //     fontSmall.drawString(waveLabel, x + (width - waveLabelBounds.width) / 2.0f, currentY);
        //     currentY += LayoutConfig::ELEMENT_SPACING + 15;
        //     
        //     const float wavePadding = LayoutConfig::PADDING;
        //     drawWaveform(x + wavePadding, currentY, width - wavePadding * 2, 
        //                 LayoutConfig::WAVEFORM_HEIGHT, displayData.waveformImage);
        //     currentY += LayoutConfig::WAVEFORM_HEIGHT + LayoutConfig::ELEMENT_SPACING;
        // }

    } else {
        // No beat data yet
        ofSetColor(100);
        std::string msg = "Waiting for beat data...";
        ofRectangle msgBounds = fontSmall.getStringBoundingBox(msg, 0, 0);
        fontSmall.drawString(msg, x + (width - msgBounds.width) / 2.0f, currentY);
    }
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
    // Draw 4 circles for beat indicator (1-4)
    const float spacing = LayoutConfig::BEAT_CIRCLE_SPACING;
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

#ifdef ENABLE_TCP_FEATURES
    if (virtualCdjRunning) {
        if (waveformFinderRunning) {
            beatlink::data::WaveformFinder::getInstance().stop();
        }
        if (metadataFinderRunning) {
            beatlink::data::MetadataFinder::getInstance().stop();
        }
        beatlink::VirtualCdj::getInstance().stop();
    }
#endif
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

    // Send BPM over OSC
    sendBpmOverOsc(beat.deviceNumber, beat.bpm);

    ofLogNotice("onBeat") << "Device #" << beat.deviceNumber
                          << " | BPM: " << beat.bpm
                          << " | Beat: " << beat.beatWithinBar << "/4"
                          << " | Pitch: " << beat.pitchPercent << "%";
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    DeckInfo info;
    info.device = device;
    
#ifdef USE_DUMMY_DATA
    // Initialize with dummy data for UI testing
    initializeDummyData(info);
#endif
    
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

#ifdef ENABLE_TCP_FEATURES
// ============================================================================
// TCP Features Implementation
// ============================================================================

void ofApp::updateDeviceStatus(int deviceNumber) {
    auto it = decks.find(deviceNumber);
    if (it == decks.end()) return;
    
    auto& vcdj = beatlink::VirtualCdj::getInstance();
    auto status = vcdj.getLatestStatusFor(deviceNumber);
    
    if (status) {
        // Cast to CdjStatus to access detailed fields
        auto cdjStatus = std::dynamic_pointer_cast<beatlink::CdjStatus>(status);
        if (cdjStatus) {
            // Status will be updated in updateDisplayDataFromTCP
            // Just store the raw data here for now
        }
    }
}

void ofApp::updateTrackMetadata(int deviceNumber) {
    if (!metadataFinderRunning) return;
    
    auto it = decks.find(deviceNumber);
    if (it == decks.end()) return;
    
    auto& metadataFinder = beatlink::data::MetadataFinder::getInstance();
    
    try {
        // Get track metadata by device number
        auto meta = metadataFinder.getLatestMetadataFor(deviceNumber);
        if (meta && meta != it->second.metadata) {
            it->second.metadata = meta;
        }
    } catch (const std::exception& e) {
        // Silently fail - metadata not available
    }
}

void ofApp::updateWaveform(int deviceNumber) {
    if (!waveformFinderRunning) return;
    
    auto it = decks.find(deviceNumber);
    if (it == decks.end()) return;
    
    // Only update if we don't have a waveform yet
    if (it->second.displayData.hasWaveform) return;
    
    auto& waveformFinder = beatlink::data::WaveformFinder::getInstance();
    
    try {
        // Get waveform preview by device number
        auto waveform = waveformFinder.getLatestPreviewFor(deviceNumber);
        if (waveform && waveform != it->second.waveformPreview) {
            it->second.waveformPreview = waveform;
        }
    } catch (const std::exception& e) {
        // Silently fail - waveform not available
    }
}

void ofApp::convertWaveformToImage(DeckInfo& deck) {
    // This function is now deprecated - waveform conversion is done in updateDisplayDataFromTCP
}

#endif // ENABLE_TCP_FEATURES

// ============================================================================
// Drawing Helper Functions (used for both TCP and dummy data)
// ============================================================================

void ofApp::drawStatusBadge(float x, float y, const std::string& text, ofColor color) {
    float width = text.length() * 8.0f + LayoutConfig::BADGE_PADDING * 2;
    float height = LayoutConfig::BADGE_HEIGHT;
    
    // Background
    ofSetColor(color.r / 4, color.g / 4, color.b / 4);
    ofDrawRectangle(x, y, width, height);
    
    // Border
    ofSetColor(color);
    ofNoFill();
    ofSetLineWidth(2);
    ofDrawRectangle(x, y, width, height);
    ofFill();
    ofSetLineWidth(1);
    
    // Text
    ofSetColor(color);
    ofRectangle textBounds = fontSmall.getStringBoundingBox(text, 0, 0);
    fontSmall.drawString(text, x + (width - textBounds.width) / 2.0f, y + 18);
}

void ofApp::drawWaveform(float x, float y, float width, float height, const ofImage& waveform) {
    // Background
    ofSetColor(20);
    ofDrawRectangle(x, y, width, height);
    
    // Draw waveform scaled
    ofSetColor(255);
    waveform.draw(x, y, width, height);
    
    // Border
    ofNoFill();
    ofSetColor(80);
    ofDrawRectangle(x, y, width, height);
    ofFill();
}

void ofApp::drawAlbumArt(float x, float y, float size, const ofImage& art) {
    // Background
    ofSetColor(30);
    ofDrawRectangle(x, y, size, size);
    
    // Draw album art
    if (art.isAllocated()) {
        ofSetColor(255);
        art.draw(x, y, size, size);
    }
    
    // Border
    ofNoFill();
    ofSetColor(80);
    ofDrawRectangle(x, y, size, size);
    ofFill();
}

#ifdef USE_DUMMY_DATA
// ============================================================================
// Dummy Data Generation (for UI testing without real devices)
// ============================================================================

void ofApp::initializeDummyData(DeckInfo& deck) {
    // Sample track titles and artists
    static const std::vector<std::string> titles = {
        "Midnight Dreams",
        "Electric Pulse",
        "Summer Vibes",
        "Deep Bass Journey"
    };
    
    static const std::vector<std::string> artists = {
        "DJ Shadow",
        "Bass Master",
        "Techno Flow",
        "House Nation"
    };
    
    // Random selection
    int titleIdx = deck.device.deviceNumber % titles.size();
    int artistIdx = deck.device.deviceNumber % artists.size();
    
    deck.displayData.trackTitle = titles[titleIdx];
    deck.displayData.trackArtist = artists[artistIdx];
    deck.displayData.isPlaying = true;
    deck.displayData.isMaster = (deck.device.deviceNumber == 1);
    deck.displayData.isSynced = false;
    deck.displayData.isOnAir = true;
    
    // Create dummy waveform
    createDummyWaveform(deck.displayData.waveformImage);
    deck.displayData.hasWaveform = true;
    
    // Create dummy album art
    createDummyAlbumArt(deck.displayData.albumArt);
    deck.displayData.hasAlbumArt = true;
    
    ofLogNotice("DummyData") << "Initialized dummy data for device #" << deck.device.deviceNumber;
}

void ofApp::createDummyWaveform(ofImage& image) {
    int width = 400;
    int height = 100;
    
    ofPixels pixels;
    pixels.allocate(width, height, OF_PIXELS_RGB);
    pixels.setColor(ofColor(0));
    
    int centerY = height / 2;
    
    // Create a sine wave pattern
    for (int x = 0; x < width; x++) {
        float freq = 0.05f + (x / (float)width) * 0.1f;
        float amplitude = 30.0f * (0.5f + 0.5f * sin(x * 0.02f));
        int waveHeight = static_cast<int>(amplitude * sin(x * freq));
        
        // Blue waveform
        ofColor waveColor(100, 150, 255);
        
        for (int y = -abs(waveHeight); y <= abs(waveHeight); y++) {
            int py = centerY + y;
            if (py >= 0 && py < height) {
                pixels.setColor(x, py, waveColor);
            }
        }
    }
    
    image.setFromPixels(pixels);
}

void ofApp::createDummyAlbumArt(ofImage& image) {
    int size = 200;
    
    ofPixels pixels;
    pixels.allocate(size, size, OF_PIXELS_RGB);
    
    // Create gradient pattern
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float r = ofMap(x, 0, size, 50, 200);
            float g = ofMap(y, 0, size, 80, 150);
            float b = 150;
            
            // Add circular pattern
            float dx = x - size / 2.0f;
            float dy = y - size / 2.0f;
            float dist = sqrt(dx * dx + dy * dy);
            float maxDist = size / 2.0f;
            
            if (dist < maxDist) {
                float brightness = 1.0f - (dist / maxDist) * 0.5f;
                r *= brightness;
                g *= brightness;
                b *= brightness;
            }
            
            pixels.setColor(x, y, ofColor(r, g, b));
        }
    }
    
    image.setFromPixels(pixels);
}
#endif // USE_DUMMY_DATA

#ifdef ENABLE_TCP_FEATURES
void ofApp::updateDisplayDataFromTCP(DeckInfo& deck) {
    // Update status flags
    auto& vcdj = beatlink::VirtualCdj::getInstance();
    auto status = vcdj.getLatestStatusFor(deck.device.deviceNumber);
    
    if (status) {
        auto cdjStatus = std::dynamic_pointer_cast<beatlink::CdjStatus>(status);
        if (cdjStatus) {
            deck.displayData.isPlaying = cdjStatus->isPlaying();
            deck.displayData.isMaster = cdjStatus->isTempoMaster();
            deck.displayData.isSynced = cdjStatus->isSynced();
            deck.displayData.isOnAir = cdjStatus->isOnAir();
        }
    }
    
    // Update track metadata
    if (deck.metadata) {
        deck.displayData.trackTitle = deck.metadata->getTitle();
        
        auto artistOpt = deck.metadata->getArtist();
        if (artistOpt) {
            deck.displayData.trackArtist = artistOpt->getLabel();
        } else {
            deck.displayData.trackArtist = "";
        }
    }
    
    // Update waveform
    if (deck.waveformPreview) {
        auto& waveform = *deck.waveformPreview;
        int segmentCount = waveform.getSegmentCount();
        
        if (segmentCount > 0 && !deck.displayData.hasWaveform) {
            // Convert waveform to image
            int width = segmentCount;
            int height = 100;
            
            ofPixels pixels;
            pixels.allocate(width, height, OF_PIXELS_RGB);
            pixels.setColor(ofColor(0));
            
            int centerY = height / 2;
            for (int i = 0; i < segmentCount; i++) {
                int heightVal = waveform.segmentHeight(i, true);
                float normalizedHeight = static_cast<float>(heightVal) / 31.0f;
                int segmentHeight = static_cast<int>(normalizedHeight * (height / 2 - 2));
                
                ofColor color;
                if (waveform.isColor()) {
                    auto c = waveform.segmentColor(i, true);
                    color.set(c.r, c.g, c.b);
                } else {
                    color.set(100, 150, 255);
                }
                
                for (int y = 0; y < segmentHeight; y++) {
                    if (centerY + y < height) pixels.setColor(i, centerY + y, color);
                    if (centerY - y >= 0) pixels.setColor(i, centerY - y, color);
                }
            }
            
            deck.displayData.waveformImage.setFromPixels(pixels);
            deck.displayData.hasWaveform = true;
        }
    }
}

#endif // ENABLE_TCP_FEATURES

// ============================================================================
// OSC Functions
// ============================================================================

void ofApp::sendBpmOverOsc(int deviceNumber, float bpm) {
    ofxOscMessage msg;
    msg.setAddress("/deck/" + ofToString(deviceNumber) + "/bpm");
    msg.addFloatArg(bpm);
    msg.addIntArg(deviceNumber);
    oscSender.sendMessage(msg);
}
