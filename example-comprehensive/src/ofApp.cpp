#include "ofApp.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

// ============================================================================
// Setup / Update / Exit
// ============================================================================

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    ofSetCircleResolution(32);

    // Register event listeners
    ofAddListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofAddListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofAddListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    ofAddListener(beatLink.deviceUpdateEvent, this, &ofApp::onDeviceUpdate);
    ofAddListener(beatLink.masterChangedEvent, this, &ofApp::onMasterChanged);

    // Start DJ Link
    if (beatLink.start()) {
        // Start VirtualCdj for detailed status
        if (beatLink.startVirtualCdj()) {
            virtualCdjRunning = true;
        }
    }
}

void ofApp::update() {
    beatLink.update();

    auto now = ofGetElapsedTimeMillis();

    // Update each device
    for (int i = 0; i < MAX_DEVICES; ++i) {
        auto& device = devices[i];

        // Decay animations
        device.beatAlpha *= 0.9f;
        device.flashAlpha *= 0.85f;

        // Calculate beat progress
        if (device.lastBeat.has_value() && device.lastBeatTime > 0) {
            auto elapsed = now - device.lastBeatTime;
            auto beatDuration = 60000.0 / device.lastBeat->bpm;
            device.beatProgress = std::fmod(static_cast<float>(elapsed) / beatDuration, 1.0f);
        }

        // Update playhead position from status
        if (device.status.has_value() && device.trackDurationMs > 0) {
            // Estimate playhead from beat number and tempo
            if (device.lastBeat.has_value()) {
                auto beatMs = 60000.0 / device.lastBeat->bpm;
                auto estimatedPositionMs = device.status->beatNumber * beatMs;
                device.playheadPosition = static_cast<float>(estimatedPositionMs) / device.trackDurationMs;
                device.playheadPosition = std::clamp(device.playheadPosition, 0.0f, 1.0f);
            }
        }
    }

    // Get master info
    if (virtualCdjRunning) {
        masterTempo = beatLink.getMasterTempo();
        auto master = beatLink.getTempoMaster();
        if (master.has_value()) {
            masterDeviceNumber = master->deviceNumber;
        }
    }

    lastUpdateTime = now;
}

void ofApp::exit() {
    ofRemoveListener(beatLink.beatEvent, this, &ofApp::onBeat);
    ofRemoveListener(beatLink.deviceFoundEvent, this, &ofApp::onDeviceFound);
    ofRemoveListener(beatLink.deviceLostEvent, this, &ofApp::onDeviceLost);
    ofRemoveListener(beatLink.deviceUpdateEvent, this, &ofApp::onDeviceUpdate);
    ofRemoveListener(beatLink.masterChangedEvent, this, &ofApp::onMasterChanged);

    if (virtualCdjRunning) {
        beatLink.stopVirtualCdj();
    }
    beatLink.stop();
}

void ofApp::draw() {
    drawHeader();

    // Calculate panel layout (2x2 grid)
    const float marginX = 20;
    const float marginY = 70;
    const float gapX = 15;
    const float gapY = 15;
    const float panelWidth = (ofGetWidth() - marginX * 2 - gapX) / 2;
    const float panelHeight = (ofGetHeight() - marginY - 40 - gapY) / 2;

    // Draw 4 device panels
    for (int i = 0; i < MAX_DEVICES; ++i) {
        int col = i % 2;
        int row = i / 2;
        float x = marginX + col * (panelWidth + gapX);
        float y = marginY + row * (panelHeight + gapY);

        if (devices[i].connected) {
            drawDevicePanel(i + 1, x, y, panelWidth, panelHeight);
        } else {
            drawEmptySlot(i + 1, x, y, panelWidth, panelHeight);
        }
    }

    drawMasterInfo();
    drawDatabaseStatus();
}

// ============================================================================
// Event Handlers
// ============================================================================

void ofApp::onBeat(ofxBeatLinkBeat& beat) {
    int idx = beat.deviceNumber - 1;
    if (idx < 0 || idx >= MAX_DEVICES) return;

    auto& device = devices[idx];
    device.lastBeat = beat;
    device.lastBeatTime = ofGetElapsedTimeMillis();
    device.beatAlpha = 1.0f;

    // Add to beat history
    BeatHistoryEntry entry;
    entry.timestamp = device.lastBeatTime;
    entry.bpm = beat.bpm;
    device.beatHistory.push_back(entry);

    while (device.beatHistory.size() > DeviceState::MAX_BEAT_HISTORY) {
        device.beatHistory.pop_front();
    }

    // Flash on downbeat
    if (beat.beatWithinBar == 1) {
        device.flashAlpha = 1.0f;
    }
}

void ofApp::onDeviceFound(ofxBeatLinkDevice& device) {
    int idx = device.deviceNumber - 1;
    if (idx < 0 || idx >= MAX_DEVICES) return;

    devices[idx].connected = true;
    devices[idx].info = device;
}

void ofApp::onDeviceLost(ofxBeatLinkDevice& device) {
    int idx = device.deviceNumber - 1;
    if (idx < 0 || idx >= MAX_DEVICES) return;

    devices[idx].connected = false;
    devices[idx].status.reset();
    devices[idx].lastBeat.reset();
}

void ofApp::onDeviceUpdate(ofxBeatLinkCdjStatus& status) {
    int idx = status.deviceNumber - 1;
    if (idx < 0 || idx >= MAX_DEVICES) return;

    devices[idx].status = status;

    // Try to load track data if database is loaded
    if (databaseLoaded && status.rekordboxId > 0) {
        // In a real implementation, you would look up track by rekordboxId
        // For now, we just store the device name as title placeholder
        if (devices[idx].trackTitle.empty()) {
            devices[idx].trackTitle = "Track #" + ofToString(status.rekordboxId);
        }
    }
}

void ofApp::onMasterChanged(ofxBeatLinkCdjStatus& status) {
    masterDeviceNumber = status.deviceNumber;
}

// ============================================================================
// Key / Drag Events
// ============================================================================

void ofApp::keyPressed(int key) {
    switch (key) {
        case 'q':
        case 'Q':
            ofExit();
            break;
        case 'w':
        case 'W':
            showWaveforms = !showWaveforms;
            break;
        case 'c':
        case 'C':
            showCuePoints = !showCuePoints;
            break;
        case 'p':
        case 'P':
            showPhrases = !showPhrases;
            break;
        case 'h':
        case 'H':
            showBeatHistory = !showBeatHistory;
            break;
        case 'm':
        case 'M':
            compactMode = !compactMode;
            break;
        case 'v':
        case 'V':
            // Toggle VirtualCdj
            if (virtualCdjRunning) {
                beatLink.stopVirtualCdj();
                virtualCdjRunning = false;
            } else {
                virtualCdjRunning = beatLink.startVirtualCdj();
            }
            break;
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (!dragInfo.files.empty()) {
        loadDatabase(dragInfo.files[0]);
    }
}

// ============================================================================
// Database Loading
// ============================================================================

void ofApp::loadDatabase(const std::string& path) {
    namespace fs = std::filesystem;

    fs::path dbPath;
    fs::path anlzPath;
    fs::path inputPath(path);

    if (fs::is_directory(inputPath)) {
        dbPath = inputPath / "rekordbox" / "export.pdb";
        anlzPath = inputPath / "PIONEER" / "USBANLZ";
        if (!fs::exists(dbPath)) {
            dbPath = inputPath / "export.pdb";
        }
        databasePath = inputPath.string();
    } else if (inputPath.filename() == "export.pdb") {
        dbPath = inputPath;
        anlzPath = inputPath.parent_path().parent_path() / "PIONEER" / "USBANLZ";
        databasePath = inputPath.parent_path().parent_path().string();
    }

    if (!fs::exists(dbPath)) {
        return;
    }

    auto result = cratedigger::Database::open(dbPath);
    if (!result) {
        return;
    }

    database = std::make_unique<cratedigger::Database>(std::move(*result));

    // Load ANLZ data
    if (fs::exists(anlzPath)) {
        database->load_cue_points(anlzPath);
    }

    databaseLoaded = true;
}

void ofApp::loadWaveformForDevice(int deviceNumber, const std::string& trackPath) {
    if (!databaseLoaded || !database) return;

    int idx = deviceNumber - 1;
    if (idx < 0 || idx >= MAX_DEVICES) return;

    auto waveforms = database->find_waveforms_by_filename(trackPath);
    if (!waveforms) return;

    auto& cache = devices[idx].waveform;

    // Prefer 3-band, then color, then blue
    if (waveforms->detail.has_value()) {
        const auto& detail = *waveforms->detail;
        cache.style = detail.style;
        cache.entryCount = detail.entry_count;

        if (detail.style == cratedigger::WaveformStyle::ThreeBand) {
            cache.threeBandData = detail.data;
        } else if (detail.style == cratedigger::WaveformStyle::RGB) {
            cache.colorData = detail.data;
        } else {
            cache.preview = detail.data;
        }
        cache.loaded = true;
    } else if (waveforms->preview.has_value()) {
        cache.preview = waveforms->preview->data;
        cache.style = cratedigger::WaveformStyle::Blue;
        cache.entryCount = waveforms->preview->entry_count;
        cache.loaded = true;
    }
}

void ofApp::loadCuePointsForDevice(int deviceNumber, const std::string& trackPath) {
    if (!databaseLoaded || !database) return;

    int idx = deviceNumber - 1;
    if (idx < 0 || idx >= MAX_DEVICES) return;

    auto cuePoints = database->find_cue_points_by_filename(trackPath);
    auto& device = devices[idx];

    device.cuePoints.clear();

    if (device.trackDurationMs == 0) return;

    for (const auto& cue : cuePoints) {
        CuePointDisplay display;
        display.positionPercent = static_cast<float>(cue.time_ms) / device.trackDurationMs;
        display.colorId = cue.color_id;
        display.isHotCue = cue.is_hot_cue();
        display.hotCueNumber = cue.hot_cue_number;
        display.isLoop = cue.is_loop();
        if (display.isLoop) {
            display.loopEndPercent = static_cast<float>(cue.loop_time_ms) / device.trackDurationMs;
        }
        device.cuePoints.push_back(display);
    }
}

void ofApp::loadSongStructureForDevice(int deviceNumber, const std::string& trackPath) {
    if (!databaseLoaded || !database) return;

    int idx = deviceNumber - 1;
    if (idx < 0 || idx >= MAX_DEVICES) return;

    auto structure = database->find_song_structure_by_filename(trackPath);
    if (!structure || structure->empty()) return;

    auto& device = devices[idx];
    device.phrases.clear();

    // Need beat grid to convert beats to time
    auto beatGrid = database->find_beat_grid_by_filename(trackPath);
    if (!beatGrid || beatGrid->empty()) return;

    // Approximate conversion using average BPM
    float avgBpm = beatGrid->average_bpm();
    if (avgBpm <= 0) return;

    float msPerBeat = 60000.0f / avgBpm;

    for (const auto& phrase : structure->phrases) {
        PhraseDisplay display;
        float startMs = phrase.beat * msPerBeat;
        float endMs = phrase.end_beat * msPerBeat;

        display.startPercent = startMs / device.trackDurationMs;
        display.endPercent = endMs / device.trackDurationMs;
        display.name = phrase.phrase_name(structure->mood);
        display.color = getPhraseColor(display.name);

        device.phrases.push_back(display);
    }
}

// ============================================================================
// Drawing - Header
// ============================================================================

void ofApp::drawHeader() {
    ofSetColor(255);
    ofDrawBitmapString("ofxBeatLink - Comprehensive Monitor", 20, 25);

    // Status
    ofSetColor(virtualCdjRunning ? ofColor(80, 255, 120) : ofColor(255, 80, 80));
    ofDrawBitmapString(virtualCdjRunning ? "VirtualCdj: ON" : "VirtualCdj: OFF", 300, 25);

    // Controls
    ofSetColor(80);
    std::string controls = "[W]aveform [C]ue [P]hrase [H]istory [M]ode [V]irtualCdj [Q]uit";
    ofDrawBitmapString(controls, ofGetWidth() - 520, 25);

    // Connected devices count
    int connectedCount = 0;
    for (const auto& d : devices) {
        if (d.connected) ++connectedCount;
    }
    ofSetColor(120);
    ofDrawBitmapString("Devices: " + ofToString(connectedCount) + "/4", 20, 50);
}

// ============================================================================
// Drawing - Device Panel
// ============================================================================

void ofApp::drawDevicePanel(int deviceNumber, float x, float y, float width, float height) {
    int idx = deviceNumber - 1;
    const auto& device = devices[idx];

    // Panel background with beat flash
    float flashIntensity = device.flashAlpha * 0.2f;
    bool isMaster = (deviceNumber == masterDeviceNumber);

    if (isMaster) {
        ofSetColor(static_cast<int>(30 + flashIntensity * 50),
                   static_cast<int>(35 + flashIntensity * 40),
                   static_cast<int>(25 + flashIntensity * 30));
    } else {
        ofSetColor(static_cast<int>(28 + flashIntensity * 40),
                   static_cast<int>(28 + flashIntensity * 30),
                   static_cast<int>(30 + flashIntensity * 20));
    }
    ofDrawRectRounded(x, y, width, height, 6);

    // Border
    ofNoFill();
    if (isMaster) {
        ofSetColor(255, 200, 80, static_cast<int>(150 + device.flashAlpha * 105));
        ofSetLineWidth(2);
    } else if (device.status.has_value() && device.status->isPlaying) {
        ofSetColor(80, 200, 120, static_cast<int>(100 + device.beatAlpha * 155));
        ofSetLineWidth(1.5f);
    } else {
        ofSetColor(60);
        ofSetLineWidth(1);
    }
    ofDrawRectRounded(x, y, width, height, 6);
    ofFill();
    ofSetLineWidth(1);

    // Content layout
    float contentX = x + 10;
    float contentY = y + 8;
    float contentWidth = width - 20;

    // Header row: Device name + status indicators
    drawDeviceHeader(device, contentX, contentY, contentWidth);
    contentY += 22;

    // Status indicators row
    drawStatusIndicators(device, contentX, contentY);

    // BPM display (right side)
    drawBpmDisplay(device, x + width - 130, contentY - 15);
    contentY += 25;

    // Waveform section
    if (showWaveforms) {
        float waveformHeight = compactMode ? 40 : 60;
        drawWaveform(device, contentX, contentY, contentWidth, waveformHeight);

        // Cue points overlay
        if (showCuePoints && !device.cuePoints.empty()) {
            drawCuePoints(device, contentX, contentY, contentWidth, waveformHeight);
        }

        // Phrases below waveform
        if (showPhrases && !device.phrases.empty()) {
            drawPhrases(device, contentX, contentY + waveformHeight + 2, contentWidth, 12);
            contentY += 14;
        }

        contentY += waveformHeight + 8;
    }

    // Beat indicators
    drawBeatIndicators(device, contentX, contentY);

    // Beat progress bar
    drawBeatProgress(device, contentX + 160, contentY + 5, contentWidth - 170);
    contentY += 35;

    // Timing info
    drawTimingInfo(device, contentX, contentY);

    // Beat history graph
    if (showBeatHistory && !compactMode) {
        drawBeatHistory(device, contentX, contentY + 25, contentWidth, 30);
    }
}

void ofApp::drawDeviceHeader(const DeviceState& state, float x, float y, float width) {
    // Device number
    ofSetColor(100, 180, 255);
    ofDrawBitmapString("#" + ofToString(state.info.deviceNumber), x, y + 12);

    // Device name
    ofSetColor(255);
    ofDrawBitmapString(state.info.deviceName, x + 25, y + 12);

    // Track title (if available)
    if (!state.trackTitle.empty()) {
        ofSetColor(180);
        std::string title = state.trackTitle;
        if (title.length() > 40) title = title.substr(0, 37) + "...";
        ofDrawBitmapString(title, x + 130, y + 12);
    }

    // IP address (right aligned)
    ofSetColor(60);
    ofDrawBitmapString(state.info.ipAddress, x + width - 110, y + 12);
}

void ofApp::drawStatusIndicators(const DeviceState& state, float x, float y) {
    float indicatorX = x;

    // Playing status
    if (state.status.has_value()) {
        if (state.status->isPlaying) {
            ofSetColor(80, 255, 120);
            ofDrawBitmapString("PLAY", indicatorX, y + 12);
        } else {
            ofSetColor(80);
            ofDrawBitmapString("STOP", indicatorX, y + 12);
        }
        indicatorX += 45;

        // Master
        if (state.status->isMaster) {
            ofSetColor(255, 200, 80);
            ofDrawBitmapString("MASTER", indicatorX, y + 12);
            indicatorX += 60;
        }

        // Sync
        if (state.status->isSynced) {
            ofSetColor(100, 180, 255);
            ofDrawBitmapString("SYNC", indicatorX, y + 12);
            indicatorX += 45;
        }

        // On-Air
        if (state.status->isOnAir) {
            ofSetColor(255, 80, 80);
            ofDrawBitmapString("ON-AIR", indicatorX, y + 12);
            indicatorX += 55;
        }

        // At Cue
        if (state.status->isAtCue) {
            ofSetColor(255, 150, 80);
            ofDrawBitmapString("CUE", indicatorX, y + 12);
        }
    } else if (state.lastBeat.has_value()) {
        // Fallback to beat-based status
        ofSetColor(100);
        ofDrawBitmapString("(No VirtualCdj status)", indicatorX, y + 12);
    }
}

void ofApp::drawBpmDisplay(const DeviceState& state, float x, float y) {
    if (!state.lastBeat.has_value() && !state.status.has_value()) return;

    double bpm = 0;
    double pitch = 0;

    if (state.lastBeat.has_value()) {
        bpm = state.lastBeat->bpm;
        pitch = state.lastBeat->pitchPercent;
    } else if (state.status.has_value()) {
        bpm = state.status->effectiveBpm;
        pitch = state.status->pitchPercent;
    }

    // BPM
    ofSetColor(80, 255, 150);
    ofDrawBitmapString(formatBpm(bpm), x, y + 12);

    // Pitch
    std::string pitchStr = (pitch >= 0 ? "+" : "") + ofToString(pitch, 2) + "%";
    ofSetColor(150);
    ofDrawBitmapString(pitchStr, x, y + 28);
}

void ofApp::drawWaveform(const DeviceState& state, float x, float y, float width, float height) {
    // Background
    ofSetColor(15);
    ofDrawRectangle(x, y, width, height);

    // Draw waveform data if available
    if (state.waveform.loaded && state.waveform.entryCount > 0) {
        float centerY = y + height / 2;

        if (state.waveform.style == cratedigger::WaveformStyle::ThreeBand &&
            !state.waveform.threeBandData.empty()) {
            // 3-band waveform (CDJ-3000 style)
            size_t entries = state.waveform.entryCount;
            float stepX = width / entries;

            for (size_t i = 0; i < entries && i * 3 + 2 < state.waveform.threeBandData.size(); ++i) {
                float wx = x + i * stepX;
                uint8_t low = state.waveform.threeBandData[i * 3] & 0x1F;
                uint8_t mid = state.waveform.threeBandData[i * 3 + 1] & 0x1F;
                uint8_t high = state.waveform.threeBandData[i * 3 + 2] & 0x1F;

                float scale = height / 2.0f / 31.0f;

                // Low (red)
                ofSetColor(255, 60, 60, 180);
                ofDrawRectangle(wx, centerY, stepX, low * scale);

                // Mid (green)
                ofSetColor(60, 255, 60, 180);
                ofDrawRectangle(wx, centerY - mid * scale, stepX, mid * scale);

                // High (blue)
                ofSetColor(60, 150, 255, 180);
                ofDrawRectangle(wx, centerY - mid * scale - high * scale, stepX, high * scale);
            }
        } else if (!state.waveform.preview.empty()) {
            // Blue monochrome waveform
            size_t entries = state.waveform.entryCount > 0 ? state.waveform.entryCount : state.waveform.preview.size();
            float stepX = width / entries;

            ofSetColor(80, 150, 255, 200);
            for (size_t i = 0; i < entries && i < state.waveform.preview.size(); ++i) {
                float wx = x + i * stepX;
                uint8_t h = state.waveform.preview[i] & 0x1F;
                float wh = h * (height / 2.0f) / 31.0f;
                ofDrawRectangle(wx, centerY - wh, stepX, wh * 2);
            }
        }

        // Playhead
        float playheadX = x + state.playheadPosition * width;
        ofSetColor(255, 255, 255, 200);
        ofSetLineWidth(2);
        ofDrawLine(playheadX, y, playheadX, y + height);
        ofSetLineWidth(1);
    } else {
        // No waveform - draw placeholder
        ofSetColor(40);
        ofDrawBitmapString("No waveform data", x + width / 2 - 60, y + height / 2 + 4);
    }

    // Border
    ofNoFill();
    ofSetColor(40);
    ofDrawRectangle(x, y, width, height);
    ofFill();
}

void ofApp::drawCuePoints(const DeviceState& state, float x, float y, float width, float height) {
    for (const auto& cue : state.cuePoints) {
        float cueX = x + cue.positionPercent * width;

        // Loop region
        if (cue.isLoop) {
            float loopEndX = x + cue.loopEndPercent * width;
            ofSetColor(getCuePointColor(cue.colorId).r,
                      getCuePointColor(cue.colorId).g,
                      getCuePointColor(cue.colorId).b, 50);
            ofDrawRectangle(cueX, y, loopEndX - cueX, height);
        }

        // Cue marker
        ofSetColor(getCuePointColor(cue.colorId));
        ofDrawLine(cueX, y, cueX, y + height);

        // Hot cue number
        if (cue.isHotCue) {
            ofDrawBitmapString(ofToString(cue.hotCueNumber), cueX + 2, y + 12);
        }
    }
}

void ofApp::drawPhrases(const DeviceState& state, float x, float y, float width, float height) {
    for (size_t i = 0; i < state.phrases.size(); ++i) {
        const auto& phrase = state.phrases[i];
        float startX = x + phrase.startPercent * width;
        float endX = x + phrase.endPercent * width;
        float phraseWidth = std::max(endX - startX, 2.0f);

        ofSetColor(phrase.color.r, phrase.color.g, phrase.color.b, 150);
        ofDrawRectangle(startX, y, phraseWidth, height);

        // Label (only if wide enough)
        if (phraseWidth > 30) {
            ofSetColor(255);
            std::string label = phrase.name;
            if (label.length() > 6) label = label.substr(0, 5);
            ofDrawBitmapString(label, startX + 2, y + 10);
        }
    }
}

void ofApp::drawBeatIndicators(const DeviceState& state, float x, float y) {
    constexpr float size = 30;
    constexpr float gap = 8;

    int currentBeat = state.lastBeat.has_value() ? state.lastBeat->beatWithinBar : 0;

    for (int i = 1; i <= 4; ++i) {
        float bx = x + (i - 1) * (size + gap);
        bool active = (i == currentBeat);

        // Background
        ofSetColor(35);
        ofDrawRectRounded(bx, y, size, size, 4);

        // Active indicator
        if (active) {
            int r, g, b;
            if (i == 1) {
                r = 255; g = 80; b = 80;  // Red for downbeat
            } else {
                r = 80; g = 200; b = 120;  // Green for other beats
            }
            ofSetColor(r, g, b, static_cast<int>(150 + state.beatAlpha * 105));
            ofDrawRectRounded(bx + 2, y + 2, size - 4, size - 4, 3);
        }

        // Number
        ofSetColor(active ? 255 : 70);
        ofDrawBitmapString(ofToString(i), bx + 11, y + 20);
    }
}

void ofApp::drawBeatProgress(const DeviceState& state, float x, float y, float width) {
    // Background
    ofSetColor(30);
    ofDrawRectRounded(x, y, width, 20, 3);

    // Progress bar
    if (state.lastBeat.has_value()) {
        float progressWidth = state.beatProgress * width;
        bool isDownbeat = state.lastBeat->beatWithinBar == 1;

        if (isDownbeat) {
            ofSetColor(255, 80, 80, 200);
        } else {
            ofSetColor(80, 180, 255, 200);
        }
        ofDrawRectRounded(x, y, progressWidth, 20, 3);
    }

    // Border
    ofNoFill();
    ofSetColor(50);
    ofDrawRectRounded(x, y, width, 20, 3);
    ofFill();
}

void ofApp::drawTimingInfo(const DeviceState& state, float x, float y) {
    if (!state.lastBeat.has_value()) return;

    const auto& beat = *state.lastBeat;

    ofSetColor(80);
    ofDrawBitmapString("Next Beat:", x, y + 12);
    ofSetColor(150);
    ofDrawBitmapString(ofToString(beat.nextBeatMs) + "ms", x + 85, y + 12);

    ofSetColor(80);
    ofDrawBitmapString("Next Bar:", x + 170, y + 12);
    ofSetColor(150);
    ofDrawBitmapString(ofToString(beat.nextBarMs) + "ms", x + 250, y + 12);

    // Track BPM
    ofSetColor(80);
    ofDrawBitmapString("Track:", x + 340, y + 12);
    ofSetColor(120);
    ofDrawBitmapString(formatBpm(beat.trackBpm), x + 390, y + 12);
}

void ofApp::drawBeatHistory(const DeviceState& state, float x, float y, float width, float height) {
    if (state.beatHistory.empty()) return;

    // Background
    ofSetColor(25);
    ofDrawRectangle(x, y, width, height);

    // Find BPM range
    double minBpm = 999, maxBpm = 0;
    for (const auto& entry : state.beatHistory) {
        minBpm = std::min(minBpm, entry.bpm);
        maxBpm = std::max(maxBpm, entry.bpm);
    }

    // Add some padding to range
    double range = maxBpm - minBpm;
    if (range < 1.0) {
        minBpm -= 0.5;
        maxBpm += 0.5;
        range = 1.0;
    }

    // Draw graph line
    ofSetColor(100, 180, 255);
    ofNoFill();
    ofBeginShape();
    size_t count = state.beatHistory.size();
    for (size_t i = 0; i < count; ++i) {
        float px = x + (static_cast<float>(i) / (DeviceState::MAX_BEAT_HISTORY - 1)) * width;
        float normalized = (state.beatHistory[i].bpm - minBpm) / range;
        float py = y + height - normalized * height;
        ofVertex(px, py);
    }
    ofEndShape(false);
    ofFill();

    // Current BPM line
    if (!state.beatHistory.empty()) {
        double currentBpm = state.beatHistory.back().bpm;
        float normalized = (currentBpm - minBpm) / range;
        float lineY = y + height - normalized * height;

        ofSetColor(255, 200, 80, 100);
        ofDrawLine(x, lineY, x + width, lineY);
    }

    // Border
    ofNoFill();
    ofSetColor(40);
    ofDrawRectangle(x, y, width, height);
    ofFill();
}

void ofApp::drawEmptySlot(int deviceNumber, float x, float y, float width, float height) {
    // Background
    ofSetColor(25);
    ofDrawRectRounded(x, y, width, height, 6);

    // Border
    ofNoFill();
    ofSetColor(40);
    ofDrawRectRounded(x, y, width, height, 6);
    ofFill();

    // Device number
    ofSetColor(60);
    ofDrawBitmapString("Device #" + ofToString(deviceNumber), x + 20, y + 30);
    ofSetColor(40);
    ofDrawBitmapString("Not connected", x + 20, y + 50);
    ofDrawBitmapString("Connect CDJ/XDJ to the same network", x + 20, y + 75);
}

void ofApp::drawMasterInfo() {
    float x = ofGetWidth() - 200;
    float y = 40;

    ofSetColor(100);
    ofDrawBitmapString("Master Tempo:", x, y);

    if (masterTempo > 0) {
        ofSetColor(255, 200, 80);
        ofDrawBitmapString(formatBpm(masterTempo), x + 110, y);

        if (masterDeviceNumber > 0) {
            ofSetColor(80);
            ofDrawBitmapString("(#" + ofToString(masterDeviceNumber) + ")", x + 170, y);
        }
    } else {
        ofSetColor(60);
        ofDrawBitmapString("---.--", x + 110, y);
    }
}

void ofApp::drawDatabaseStatus() {
    float x = 20;
    float y = ofGetHeight() - 20;

    if (databaseLoaded) {
        ofSetColor(80, 200, 120);
        ofDrawBitmapString("Database: Loaded", x, y);
    } else {
        ofSetColor(100);
        ofDrawBitmapString("Drag PIONEER folder here for waveforms & cue points", x, y);
    }
}

void ofApp::drawInstructions() {
    // Already handled in drawHeader
}

// ============================================================================
// Helpers
// ============================================================================

ofColor ofApp::getCuePointColor(uint8_t colorId) {
    switch (colorId) {
        case 0: return ofColor(255, 255, 255);
        case 1: return ofColor(255, 80, 80);
        case 2: return ofColor(255, 180, 80);
        case 3: return ofColor(255, 255, 80);
        case 4: return ofColor(80, 255, 80);
        case 5: return ofColor(80, 255, 255);
        case 6: return ofColor(80, 80, 255);
        case 7: return ofColor(180, 80, 255);
        case 8: return ofColor(255, 80, 180);
        default: return ofColor(150);
    }
}

ofColor ofApp::getPhraseColor(const std::string& phraseName) {
    if (phraseName == "Intro") return ofColor(100, 180, 255);
    if (phraseName == "Outro") return ofColor(100, 180, 255);
    if (phraseName == "Chorus") return ofColor(255, 100, 100);
    if (phraseName == "Up") return ofColor(255, 200, 80);
    if (phraseName == "Down") return ofColor(80, 200, 120);
    if (phraseName == "Bridge") return ofColor(180, 100, 255);
    if (phraseName.find("Verse") != std::string::npos) return ofColor(80, 255, 180);
    return ofColor(100, 100, 100);
}

std::string ofApp::formatTime(uint32_t ms) {
    int mins = ms / 60000;
    int secs = (ms / 1000) % 60;
    int millis = (ms % 1000) / 10;
    return ofToString(mins) + ":" + ofToString(secs, 0, 2, '0') + "." + ofToString(millis, 0, 2, '0');
}

std::string ofApp::formatBpm(double bpm) {
    return ofToString(bpm, 2);
}
