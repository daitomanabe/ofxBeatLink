#pragma once

#include "ofMain.h"
#include <beatlink/BeatLink.hpp>
#include <mutex>
#include <queue>
#include <optional>

// Define OFX_BEATLINK_NO_VIRTUALCDJ to build without VirtualCdj support
// VirtualCdj requires additional dependencies (sqlite3, utf8proc, etc.)
#ifndef OFX_BEATLINK_HAS_VIRTUALCDJ
#define OFX_BEATLINK_HAS_VIRTUALCDJ 0
#endif

/**
 * Beat event data structure for openFrameworks
 */
struct ofxBeatLinkBeat {
    int deviceNumber = 0;
    std::string deviceName;
    double bpm = 0.0;           // Effective BPM (with pitch adjustment)
    double trackBpm = 0.0;      // Original track BPM
    int beatWithinBar = 1;      // 1-4 (1 = downbeat)
    double pitchPercent = 0.0;  // Pitch adjustment percentage
    int64_t nextBeatMs = 0;     // Milliseconds until next beat
    int64_t nextBarMs = 0;      // Milliseconds until next bar
};

/**
 * Device announcement data structure for openFrameworks
 */
struct ofxBeatLinkDevice {
    int deviceNumber = 0;
    std::string deviceName;
    std::string ipAddress;
    std::string macAddress;
    bool isOpusQuad = false;
    bool isXdjAz = false;
};

/**
 * CDJ status data structure for openFrameworks
 * More detailed status than beat info, updated via VirtualCdj
 */
struct ofxBeatLinkCdjStatus {
    int deviceNumber = 0;
    std::string deviceName;
    std::string ipAddress;

    // Playback state
    bool isPlaying = false;
    bool isTrackLoaded = false;
    bool isAtCue = false;
    bool isPlayingForwards = false;
    bool isPlayingBackwards = false;

    // Status flags
    bool isMaster = false;
    bool isSynced = false;
    bool isOnAir = false;
    bool isBpmOnlySynced = false;

    // Tempo info
    double bpm = 0.0;              // Track BPM (raw)
    double effectiveBpm = 0.0;     // With pitch adjustment
    double pitchPercent = 0.0;
    int beatWithinBar = 1;         // 1-4
    int beatNumber = -1;           // Position in track

    // Track source
    int trackSourcePlayer = 0;
    int rekordboxId = 0;
    int trackNumber = 0;

    // Timestamp
    uint64_t timestamp = 0;
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

    // ========== Basic Mode (Passive Listener) ==========

    /**
     * Start listening for DJ Link devices and beats (passive mode).
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

#if OFX_BEATLINK_HAS_VIRTUALCDJ
    // ========== VirtualCdj Mode (Active Participant) ==========

    /**
     * Start VirtualCdj to act as a CDJ on the network.
     * This enables receiving detailed device status updates (CdjStatus).
     * @param deviceNumber Device number to claim (0 = auto-assign, 1-4 = specific)
     * @return true if started successfully
     */
    bool startVirtualCdj(int deviceNumber = 0);

    /**
     * Stop VirtualCdj.
     */
    void stopVirtualCdj();

    /**
     * Check if VirtualCdj is running.
     */
    bool isVirtualCdjRunning() const;

    /**
     * Get the device number assigned to our VirtualCdj.
     */
    int getVirtualCdjDeviceNumber() const;

    /**
     * Set the device name for our VirtualCdj.
     * Must be called before startVirtualCdj().
     */
    void setVirtualCdjDeviceName(const std::string& name);

    /**
     * Get the tempo master device status.
     */
    std::optional<ofxBeatLinkCdjStatus> getTempoMaster();

    /**
     * Get the master tempo (BPM).
     */
    double getMasterTempo() const;

    /**
     * Get all current device statuses.
     */
    std::vector<ofxBeatLinkCdjStatus> getCurrentStatuses();

    /**
     * Get status for a specific device.
     */
    std::optional<ofxBeatLinkCdjStatus> getStatusFor(int deviceNumber);
#endif // OFX_BEATLINK_HAS_VIRTUALCDJ

    // ========== Events ==========

    // Basic events
    ofEvent<ofxBeatLinkBeat> beatEvent;
    ofEvent<ofxBeatLinkDevice> deviceFoundEvent;
    ofEvent<ofxBeatLinkDevice> deviceLostEvent;

#if OFX_BEATLINK_HAS_VIRTUALCDJ
    // VirtualCdj events (require startVirtualCdj)
    ofEvent<ofxBeatLinkCdjStatus> deviceUpdateEvent;  // Detailed status updates
    ofEvent<ofxBeatLinkCdjStatus> masterChangedEvent; // Tempo master changed
#endif

private:
    void onBeat(const beatlink::Beat& beat);
    void onDeviceFound(const beatlink::DeviceAnnouncement& device);
    void onDeviceLost(const beatlink::DeviceAnnouncement& device);

    ofxBeatLinkBeat convertBeat(const beatlink::Beat& beat);
    ofxBeatLinkDevice convertDevice(const beatlink::DeviceAnnouncement& device);
#if OFX_BEATLINK_HAS_VIRTUALCDJ
    ofxBeatLinkCdjStatus convertCdjStatus(const beatlink::CdjStatus& status);
#endif

    std::atomic<bool> running_{false};
#if OFX_BEATLINK_HAS_VIRTUALCDJ
    std::atomic<bool> virtualCdjRunning_{false};
    std::string virtualCdjName_ = "ofxBeatLink";
#endif

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

#if OFX_BEATLINK_HAS_VIRTUALCDJ
    std::mutex deviceUpdateQueueMutex_;
    std::queue<ofxBeatLinkCdjStatus> deviceUpdateQueue_;

    std::mutex masterChangedQueueMutex_;
    std::queue<ofxBeatLinkCdjStatus> masterChangedQueue_;

    // Latest status per device (from VirtualCdj)
    std::mutex latestStatusesMutex_;
    std::map<int, ofxBeatLinkCdjStatus> latestStatuses_;

    // Listener pointers for cleanup
    beatlink::DeviceUpdateListenerPtr deviceUpdateListener_;
    beatlink::MasterListenerPtr masterListener_;
#endif
};
