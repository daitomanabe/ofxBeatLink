#pragma once

#include "ofMain.h"
#include <beatlink/BeatLink.hpp>
#include <mutex>
#include <queue>

/**
 * Beat event data structure for openFrameworks
 */
struct ofxBeatLinkBeat {
    int deviceNumber;
    std::string deviceName;
    double bpm;           // Effective BPM (with pitch adjustment)
    double trackBpm;      // Original track BPM
    int beatWithinBar;    // 1-4 (1 = downbeat)
    double pitchPercent;  // Pitch adjustment percentage
    int64_t nextBeatMs;   // Milliseconds until next beat
    int64_t nextBarMs;    // Milliseconds until next bar

    ofxBeatLinkBeat() : deviceNumber(0), bpm(0), trackBpm(0),
                        beatWithinBar(1), pitchPercent(0),
                        nextBeatMs(0), nextBarMs(0) {}
};

/**
 * Device announcement data structure for openFrameworks
 */
struct ofxBeatLinkDevice {
    int deviceNumber;
    std::string deviceName;
    std::string ipAddress;
    std::string macAddress;
    bool isOpusQuad;
    bool isXdjAz;

    ofxBeatLinkDevice() : deviceNumber(0), isOpusQuad(false), isXdjAz(false) {}
};

/**
 * openFrameworks addon for Pioneer DJ Link protocol
 *
 * Wraps beat-link-cpp library for use with openFrameworks event system.
 * Events are delivered on the main thread via update().
 */
class ofxBeatLink {
public:
    ofxBeatLink();
    ~ofxBeatLink();

    /**
     * Start listening for DJ Link devices and beats.
     * @return true if started successfully
     */
    bool start();

    /**
     * Stop listening.
     */
    void stop();

    /**
     * Check if running.
     */
    bool isRunning() const;

    /**
     * Call this in ofApp::update() to process events on main thread.
     * This dispatches queued events to listeners.
     */
    void update();

    /**
     * Get currently detected devices.
     */
    std::vector<ofxBeatLinkDevice> getCurrentDevices();

    /**
     * Get the latest beat info for a specific device.
     * @param deviceNumber The device number (1-4 typically)
     * @return The latest beat info, or empty if not available
     */
    ofxBeatLinkBeat getLatestBeat(int deviceNumber);

    /**
     * Get the latest beat info from any device.
     */
    ofxBeatLinkBeat getLatestBeat();

    // openFrameworks-style events
    ofEvent<ofxBeatLinkBeat> beatEvent;
    ofEvent<ofxBeatLinkDevice> deviceFoundEvent;
    ofEvent<ofxBeatLinkDevice> deviceLostEvent;

private:
    void onBeat(const beatlink::Beat& beat);
    void onDeviceFound(const beatlink::DeviceAnnouncement& device);
    void onDeviceLost(const beatlink::DeviceAnnouncement& device);

    ofxBeatLinkBeat convertBeat(const beatlink::Beat& beat);
    ofxBeatLinkDevice convertDevice(const beatlink::DeviceAnnouncement& device);

    std::atomic<bool> running_{false};

    // Thread-safe event queues
    std::mutex beatQueueMutex_;
    std::queue<ofxBeatLinkBeat> beatQueue_;

    std::mutex deviceFoundQueueMutex_;
    std::queue<ofxBeatLinkDevice> deviceFoundQueue_;

    std::mutex deviceLostQueueMutex_;
    std::queue<ofxBeatLinkDevice> deviceLostQueue_;

    // Latest beat per device
    std::mutex latestBeatsMutex_;
    std::map<int, ofxBeatLinkBeat> latestBeats_;
    ofxBeatLinkBeat lastBeat_;
};
