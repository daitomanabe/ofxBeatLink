#!/usr/bin/env python3
"""
Start 4 mock CDJ devices in a single process using threads.
Each device has different BPM, pitch, name, and state.
"""

import threading
import time
import sys
import signal

# Import from mock_sender
from mock_sender import MockDJLinkSender, PlayState


def create_deck_configs():
    """Create configurations for 4 different decks."""
    return [
        {
            "name": "CDJ-2000NXS2",
            "number": 1,
            "bpm": 128.0,
            "pitch": 0.0,
            "is_master": True,
            "is_synced": True,
            "is_on_air": True,
            "play_state": PlayState.PLAYING,
            "track_id": 101,
        },
        {
            "name": "CDJ-3000",
            "number": 2,
            "bpm": 130.0,
            "pitch": 3.2,
            "is_master": False,
            "is_synced": True,
            "is_on_air": True,
            "play_state": PlayState.PLAYING,
            "track_id": 205,
        },
        {
            "name": "XDJ-1000MK2",
            "number": 3,
            "bpm": 126.0,
            "pitch": -1.5,
            "is_master": False,
            "is_synced": True,
            "is_on_air": False,
            "play_state": PlayState.LOOPING,
            "track_id": 333,
        },
        {
            "name": "DDJ-1000",
            "number": 4,
            "bpm": 140.0,
            "pitch": 6.0,
            "is_master": False,
            "is_synced": False,
            "is_on_air": True,
            "play_state": PlayState.CUED,
            "track_id": 42,
        },
    ]


class MultiDeckSender:
    """Manages multiple mock CDJ senders."""
    
    def __init__(self, target_ip: str = '127.0.0.1'):
        self.target_ip = target_ip
        self.senders = []
        self.threads = []
        self.running = False
        
    def setup(self, configs):
        """Create senders from configurations."""
        for cfg in configs:
            sender = MockDJLinkSender(
                target_ip=self.target_ip,
                device_name=cfg["name"],
                device_number=cfg["number"],
                bpm=cfg["bpm"],
                debug=False
            )
            sender.pitch_percent = cfg["pitch"]
            sender.is_master = cfg["is_master"]
            sender.is_synced = cfg["is_synced"]
            sender.is_on_air = cfg["is_on_air"]
            sender.play_state = cfg["play_state"]
            sender.track_id = cfg["track_id"]
            self.senders.append(sender)
    
    def _run_sender(self, sender, index):
        """Run a single sender (called from thread)."""
        beat_interval = 60.0 / sender.bpm if sender.bpm > 0 else 0.5
        announce_interval = 1.5
        status_interval = 0.2
        
        last_announce_time = 0
        last_beat_time = index * 0.1  # Offset start times slightly
        last_status_time = index * 0.05
        
        while self.running:
            now = time.time()
            
            if now - last_announce_time >= announce_interval:
                sender.send_announcement()
                last_announce_time = now
            
            if now - last_status_time >= status_interval:
                sender.send_status()
                last_status_time = now
            
            if now - last_beat_time >= beat_interval:
                if sender.play_state in [PlayState.PLAYING, PlayState.LOOPING, PlayState.CUEING]:
                    sender.send_beat()
                last_beat_time = now
            
            time.sleep(0.01)
    
    def start(self):
        """Start all senders."""
        self.running = True
        for i, sender in enumerate(self.senders):
            t = threading.Thread(target=self._run_sender, args=(sender, i), daemon=True)
            t.start()
            self.threads.append(t)
    
    def stop(self):
        """Stop all senders."""
        self.running = False
        for t in self.threads:
            t.join(timeout=1.0)
        for sender in self.senders:
            sender.sock_announce.close()
            sender.sock_beat.close()
            sender.sock_status.close()
    
    def print_status(self):
        """Print current status of all decks."""
        print("\n" + "=" * 70)
        print(f"  4-Deck Mock CDJ Sender - Target: {self.target_ip}")
        print("=" * 70)
        for sender in self.senders:
            state_name = sender.get_play_state_name()
            flags = []
            if sender.is_master:
                flags.append("MASTER")
            if sender.is_synced:
                flags.append("SYNC")
            if sender.is_on_air:
                flags.append("ON-AIR")
            flags_str = " [" + ",".join(flags) + "]" if flags else ""
            
            print(f"  #{sender.device_number} {sender.device_name:15} | "
                  f"BPM: {sender.bpm:6.1f} | Pitch: {sender.pitch_percent:+5.1f}% | "
                  f"{state_name:8}{flags_str}")
        print("=" * 70)
        print("Press Ctrl+C to stop all decks.\n")


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Start 4 mock CDJ devices')
    parser.add_argument('--target', '-t', default='127.0.0.1',
                        help='Target IP address (default: 127.0.0.1)')
    parser.add_argument('--broadcast', '-B', action='store_true',
                        help='Send to broadcast address')
    args = parser.parse_args()
    
    target = '255.255.255.255' if args.broadcast else args.target
    
    multi = MultiDeckSender(target_ip=target)
    multi.setup(create_deck_configs())
    
    # Handle Ctrl+C gracefully
    def signal_handler(sig, frame):
        print("\nStopping all decks...")
        multi.stop()
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    multi.print_status()
    multi.start()
    
    # Keep main thread alive and show periodic updates
    beat_counter = 0
    try:
        while True:
            time.sleep(1.0)
            beat_counter += 1
            # Print summary every 5 seconds
            if beat_counter % 5 == 0:
                beats = [f"#{s.device_number}:{s.beat_within_bar}/4" for s in multi.senders 
                         if s.play_state in [PlayState.PLAYING, PlayState.LOOPING]]
                if beats:
                    print(f"[BEATS] {' | '.join(beats)}")
    except KeyboardInterrupt:
        pass
    
    multi.stop()
    print("Done.")


if __name__ == '__main__':
    main()
