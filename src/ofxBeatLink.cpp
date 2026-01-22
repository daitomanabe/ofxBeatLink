#include "ofxBeatLink.h"

ofxBeatLink::ofxBeatLink() = default;

ofxBeatLink::~ofxBeatLink() {
#if OFX_BEATLINK_HAS_VIRTUALCDJ
    stopVirtualCdj();
#endif
    stop();
}

bool ofxBeatLink::start() {
    if (running_) {
        return true;
    }

    auto& deviceFinder = beatlink::DeviceFinder::getInstance();
    auto& beatFinder = beatlink::BeatFinder::getInstance();

    // Register callbacks
    deviceFinder.addDeviceFoundListener([this](const beatlink::DeviceAnnouncement& device) {
        onDeviceFound(device);
    });

    deviceFinder.addDeviceLostListener([this](const beatlink::DeviceAnnouncement& device) {
        onDeviceLost(device);
    });

    beatFinder.addBeatListener([this](const beatlink::Beat& beat) {
        onBeat(beat);
    });

    // Start listeners
    bool deviceStarted = deviceFinder.start();
    if (!deviceStarted) {
        ofLogError("ofxBeatLink") << "Failed to start DeviceFinder (port 50000 may be in use)";
        return false;
    }

    bool beatStarted = beatFinder.start();
    if (!beatStarted) {
        ofLogError("ofxBeatLink") << "Failed to start BeatFinder (port 50001 may be in use)";
        deviceFinder.stop();
        return false;
    }

    running_ = true;
    ofLogNotice("ofxBeatLink") << "Started listening for DJ Link devices";
    return true;
}

void ofxBeatLink::stop() {
    if (!running_) {
        return;
    }

    auto& deviceFinder = beatlink::DeviceFinder::getInstance();
    auto& beatFinder = beatlink::BeatFinder::getInstance();

    beatFinder.stop();
    beatFinder.clearListeners();

    deviceFinder.stop();
    deviceFinder.clearListeners();

    running_ = false;
    ofLogNotice("ofxBeatLink") << "Stopped listening for DJ Link devices";
}

#if OFX_BEATLINK_HAS_VIRTUALCDJ
// ========== VirtualCdj Methods ==========

bool ofxBeatLink::startVirtualCdj(int deviceNumber) {
    if (virtualCdjRunning_) {
        return true;
    }

    auto& virtualCdj = beatlink::VirtualCdj::getInstance();

    // Set device name before starting
    virtualCdj.setDeviceName(virtualCdjName_);

    // Register listeners using callback wrapper classes
    deviceUpdateListener_ = std::make_shared<beatlink::DeviceUpdateCallbacks>(
        [this](const beatlink::DeviceUpdate& update) {
            // Handle CdjStatus updates directly in callback
            const auto* cdjStatus = dynamic_cast<const beatlink::CdjStatus*>(&update);
            if (cdjStatus) {
                auto ofStatus = convertCdjStatus(*cdjStatus);
                {
                    std::lock_guard<std::mutex> lock(latestStatusesMutex_);
                    latestStatuses_[ofStatus.deviceNumber] = ofStatus;
                }
                {
                    std::lock_guard<std::mutex> lock(deviceUpdateQueueMutex_);
                    deviceUpdateQueue_.push(ofStatus);
                }
            }
        }
    );
    virtualCdj.addUpdateListener(deviceUpdateListener_);

    masterListener_ = std::make_shared<beatlink::MasterListenerCallbacks>(
        [this](const beatlink::DeviceUpdate* update) {
            if (update) {
                const auto* cdjStatus = dynamic_cast<const beatlink::CdjStatus*>(update);
                if (cdjStatus) {
                    auto ofStatus = convertCdjStatus(*cdjStatus);
                    std::lock_guard<std::mutex> lock(masterChangedQueueMutex_);
                    masterChangedQueue_.push(ofStatus);
                }
            }
        },
        [](double) {},  // tempo changed - not used
        [](const beatlink::Beat&) {}  // new beat - not used
    );
    virtualCdj.addMasterListener(masterListener_);

    // Start VirtualCdj
    bool started = false;
    if (deviceNumber == 0) {
        started = virtualCdj.start();
    } else {
        started = virtualCdj.start(static_cast<uint8_t>(deviceNumber));
    }

    if (!started) {
        ofLogError("ofxBeatLink") << "Failed to start VirtualCdj";
        virtualCdj.removeUpdateListener(deviceUpdateListener_);
        virtualCdj.removeMasterListener(masterListener_);
        deviceUpdateListener_.reset();
        masterListener_.reset();
        return false;
    }

    virtualCdjRunning_ = true;
    ofLogNotice("ofxBeatLink") << "VirtualCdj started as device #" << static_cast<int>(virtualCdj.getDeviceNumber());
    return true;
}

void ofxBeatLink::stopVirtualCdj() {
    if (!virtualCdjRunning_) {
        return;
    }

    auto& virtualCdj = beatlink::VirtualCdj::getInstance();

    if (deviceUpdateListener_) {
        virtualCdj.removeUpdateListener(deviceUpdateListener_);
        deviceUpdateListener_.reset();
    }
    if (masterListener_) {
        virtualCdj.removeMasterListener(masterListener_);
        masterListener_.reset();
    }

    virtualCdj.stop();
    virtualCdjRunning_ = false;

    ofLogNotice("ofxBeatLink") << "VirtualCdj stopped";
}

bool ofxBeatLink::isVirtualCdjRunning() const {
    return virtualCdjRunning_;
}

int ofxBeatLink::getVirtualCdjDeviceNumber() const {
    if (!virtualCdjRunning_) {
        return 0;
    }
    return beatlink::VirtualCdj::getInstance().getDeviceNumber();
}

void ofxBeatLink::setVirtualCdjDeviceName(const std::string& name) {
    virtualCdjName_ = name;
}

std::optional<ofxBeatLinkCdjStatus> ofxBeatLink::getTempoMaster() {
    if (!virtualCdjRunning_) {
        return std::nullopt;
    }

    auto master = beatlink::VirtualCdj::getInstance().getTempoMaster();
    if (!master) {
        return std::nullopt;
    }

    auto* cdjStatus = dynamic_cast<beatlink::CdjStatus*>(master.get());
    if (!cdjStatus) {
        return std::nullopt;
    }

    return convertCdjStatus(*cdjStatus);
}

double ofxBeatLink::getMasterTempo() const {
    if (!virtualCdjRunning_) {
        return 0.0;
    }
    return beatlink::VirtualCdj::getInstance().getMasterTempo();
}

std::vector<ofxBeatLinkCdjStatus> ofxBeatLink::getCurrentStatuses() {
    std::lock_guard<std::mutex> lock(latestStatusesMutex_);
    std::vector<ofxBeatLinkCdjStatus> result;
    result.reserve(latestStatuses_.size());
    for (const auto& [deviceNum, status] : latestStatuses_) {
        result.push_back(status);
    }
    return result;
}

std::optional<ofxBeatLinkCdjStatus> ofxBeatLink::getStatusFor(int deviceNumber) {
    std::lock_guard<std::mutex> lock(latestStatusesMutex_);
    if (auto it = latestStatuses_.find(deviceNumber); it != latestStatuses_.end()) {
        return it->second;
    }
    return std::nullopt;
}
#endif // OFX_BEATLINK_HAS_VIRTUALCDJ

bool ofxBeatLink::isRunning() const {
    return running_;
}

void ofxBeatLink::update() {
    // Process beat events
    {
        std::lock_guard<std::mutex> lock(beatQueueMutex_);
        while (!beatQueue_.empty()) {
            auto beat = beatQueue_.front();
            beatQueue_.pop();
            ofNotifyEvent(beatEvent, beat);
        }
    }

    // Process device found events
    {
        std::lock_guard<std::mutex> lock(deviceFoundQueueMutex_);
        while (!deviceFoundQueue_.empty()) {
            auto device = deviceFoundQueue_.front();
            deviceFoundQueue_.pop();
            ofNotifyEvent(deviceFoundEvent, device);
        }
    }

    // Process device lost events
    {
        std::lock_guard<std::mutex> lock(deviceLostQueueMutex_);
        while (!deviceLostQueue_.empty()) {
            auto device = deviceLostQueue_.front();
            deviceLostQueue_.pop();
            ofNotifyEvent(deviceLostEvent, device);
        }
    }

#if OFX_BEATLINK_HAS_VIRTUALCDJ
    // Process device update events (VirtualCdj mode)
    {
        std::lock_guard<std::mutex> lock(deviceUpdateQueueMutex_);
        while (!deviceUpdateQueue_.empty()) {
            auto status = deviceUpdateQueue_.front();
            deviceUpdateQueue_.pop();
            ofNotifyEvent(deviceUpdateEvent, status);
        }
    }

    // Process master changed events (VirtualCdj mode)
    {
        std::lock_guard<std::mutex> lock(masterChangedQueueMutex_);
        while (!masterChangedQueue_.empty()) {
            auto status = masterChangedQueue_.front();
            masterChangedQueue_.pop();
            ofNotifyEvent(masterChangedEvent, status);
        }
    }
#endif
}

std::vector<ofxBeatLinkDevice> ofxBeatLink::getCurrentDevices() {
    std::vector<ofxBeatLinkDevice> result;

    auto& deviceFinder = beatlink::DeviceFinder::getInstance();
    auto devices = deviceFinder.getCurrentDevices();

    for (const auto& device : devices) {
        result.push_back(convertDevice(device));
    }

    return result;
}

ofxBeatLinkBeat ofxBeatLink::getLatestBeat(int deviceNumber) {
    std::lock_guard<std::mutex> lock(latestBeatsMutex_);
    auto it = latestBeats_.find(deviceNumber);
    if (it != latestBeats_.end()) {
        return it->second;
    }
    return ofxBeatLinkBeat();
}

ofxBeatLinkBeat ofxBeatLink::getLatestBeat() {
    std::lock_guard<std::mutex> lock(latestBeatsMutex_);
    return lastBeat_;
}

void ofxBeatLink::onBeat(const beatlink::Beat& beat) {
    auto ofBeat = convertBeat(beat);

    // Store latest beat
    {
        std::lock_guard<std::mutex> lock(latestBeatsMutex_);
        latestBeats_[ofBeat.deviceNumber] = ofBeat;
        lastBeat_ = ofBeat;
    }

    // Queue for main thread delivery
    {
        std::lock_guard<std::mutex> lock(beatQueueMutex_);
        beatQueue_.push(ofBeat);
    }
}

void ofxBeatLink::onDeviceFound(const beatlink::DeviceAnnouncement& device) {
    auto ofDevice = convertDevice(device);

    std::lock_guard<std::mutex> lock(deviceFoundQueueMutex_);
    deviceFoundQueue_.push(ofDevice);
}

void ofxBeatLink::onDeviceLost(const beatlink::DeviceAnnouncement& device) {
    auto ofDevice = convertDevice(device);

    // Clear latest beat for this device
    {
        std::lock_guard<std::mutex> lock(latestBeatsMutex_);
        latestBeats_.erase(ofDevice.deviceNumber);
    }

    std::lock_guard<std::mutex> lock(deviceLostQueueMutex_);
    deviceLostQueue_.push(ofDevice);
}

ofxBeatLinkBeat ofxBeatLink::convertBeat(const beatlink::Beat& beat) {
    ofxBeatLinkBeat result;
    result.deviceNumber = beat.getDeviceNumber();
    result.deviceName = beat.getDeviceName();
    result.bpm = beat.getEffectiveTempo();
    result.trackBpm = beat.getBpm() / 100.0;
    result.beatWithinBar = beat.getBeatWithinBar();
    result.pitchPercent = beatlink::Util::pitchToPercentage(beat.getPitch());
    result.nextBeatMs = beat.getNextBeat();
    result.nextBarMs = beat.getNextBar();
    return result;
}

ofxBeatLinkDevice ofxBeatLink::convertDevice(const beatlink::DeviceAnnouncement& device) {
    ofxBeatLinkDevice result;
    result.deviceNumber = device.getDeviceNumber();
    result.deviceName = device.getDeviceName();
    result.ipAddress = device.getAddress().to_string();
    result.macAddress = device.getHardwareAddressString();
    result.isOpusQuad = device.isOpusQuad();
    result.isXdjAz = device.isXdjAz();
    return result;
}

#if OFX_BEATLINK_HAS_VIRTUALCDJ
ofxBeatLinkCdjStatus ofxBeatLink::convertCdjStatus(const beatlink::CdjStatus& status) {
    ofxBeatLinkCdjStatus result;
    result.deviceNumber = status.getDeviceNumber();
    result.deviceName = status.getDeviceName();
    result.ipAddress = status.getAddress().to_string();

    // Playback state
    result.isPlaying = status.isPlaying();
    result.isTrackLoaded = status.isTrackLoaded();
    result.isAtCue = status.isAtCue();
    result.isPlayingForwards = status.isPlayingForwards();
    result.isPlayingBackwards = status.isPlayingBackwards();

    // Status flags
    result.isMaster = status.isTempoMaster();
    result.isSynced = status.isSynced();
    result.isOnAir = status.isOnAir();
    result.isBpmOnlySynced = status.isBpmOnlySynced();

    // Tempo info
    result.bpm = status.getBpm() / 100.0;
    result.effectiveBpm = status.getEffectiveTempo();
    result.pitchPercent = beatlink::Util::pitchToPercentage(status.getPitch());
    result.beatWithinBar = status.getBeatWithinBar();
    result.beatNumber = status.getBeatNumber();

    // Track source
    result.trackSourcePlayer = status.getTrackSourcePlayer();
    result.rekordboxId = status.getRekordboxId();
    result.trackNumber = status.getTrackNumber();

    result.timestamp = ofGetElapsedTimeMillis();
    return result;
}
#endif

