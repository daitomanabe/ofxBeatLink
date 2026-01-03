#include "ofxBeatLink.h"

ofxBeatLink::ofxBeatLink() {
}

ofxBeatLink::~ofxBeatLink() {
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
