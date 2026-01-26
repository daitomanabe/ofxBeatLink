#!/usr/bin/env python3
"""
DJ Link Mock Sender
Sends valid DeviceAnnouncement (port 50000), Beat (port 50001), and 
CDJ Status (port 50002) packets to test ofxBeatLink receiver.

Based on beat-link-cpp packet structures and prodj-link packets.py
"""

import socket
import struct
import time
import argparse
import sys
import random

# Magic header for all DJ Link packets (from Util.hpp)
# "Qspt1WmJOL" in ASCII
MAGIC_HEADER = bytes([0x51, 0x73, 0x70, 0x74, 0x31, 0x57, 0x6d, 0x4a, 0x4f, 0x4c])

# Packet types (from PacketTypes.hpp)
PACKET_TYPE_KEEP_ALIVE = 0x06  # Port 50000 - DEVICE_KEEP_ALIVE
PACKET_TYPE_BEAT = 0x28        # Port 50001 - BEAT
PACKET_TYPE_CDJ_STATUS = 0x0a  # Port 50002 - CDJ_STATUS

# Ports
PORT_ANNOUNCEMENT = 50000
PORT_BEAT = 50001
PORT_STATUS = 50002

# Pitch values (from Util.hpp)
NEUTRAL_PITCH = 1048576  # Normal speed (1.0x)

# Play states (from packets.py)
class PlayState:
    NO_TRACK = 0x00
    LOADING_TRACK = 0x02
    PLAYING = 0x03
    LOOPING = 0x04
    PAUSED = 0x05
    CUED = 0x06
    CUEING = 0x07
    SEEKING = 0x09
    END_OF_TRACK = 0x11

# State mask flags
class StateFlags:
    ON_AIR = 0x08
    SYNC = 0x10
    MASTER = 0x20
    PLAY = 0x40


def build_device_announcement(device_name: str, device_number: int, mac_address: bytes, ip_address: bytes) -> bytes:
    """
    Build a DeviceAnnouncement keep-alive packet (54 bytes).
    Based on DeviceAnnouncement.hpp PACKET_SIZE = 0x36 (54)
    
    Packet structure (from DeviceAnnouncement.hpp):
    - 0x00-0x09: Magic header (10 bytes)
    - 0x0a: Packet type (0x06 for keep-alive)
    - 0x0b: Sub-type (usually 0x02)
    - 0x0c-0x1f: Device name (20 bytes, null-padded) - extractString(data, 0x0c, 20)
    - 0x20: Unknown (usually 0x01)
    - 0x21: Unknown (usually 0x02)
    - 0x22-0x23: Unknown (usually 0x00, 0x36=packet length)
    - 0x24: Device number - data[0x24]
    - 0x25: Device type (0x01=CDJ, 0x02=Mixer, 0x03=rekordbox)
    - 0x26-0x2b: MAC address (6 bytes)
    - 0x2c-0x2f: IP address (4 bytes)
    - 0x30: Peer count
    - 0x31-0x35: Padding
    """
    packet = bytearray(54)
    
    # Magic header (offset 0x00)
    packet[0x00:0x0a] = MAGIC_HEADER
    
    # Packet type (offset 0x0a)
    packet[0x0a] = PACKET_TYPE_KEEP_ALIVE
    
    # Sub-type (offset 0x0b)
    packet[0x0b] = 0x02
    
    # Device name - 20 bytes at offset 0x0c (null-padded)
    name_bytes = device_name.encode('ascii')[:20].ljust(20, b'\x00')
    packet[0x0c:0x20] = name_bytes
    
    # Unknown fields
    packet[0x20] = 0x01
    packet[0x21] = 0x02
    packet[0x22] = 0x00
    packet[0x23] = 0x36  # Packet length
    
    # Device number (offset 0x24)
    packet[0x24] = device_number & 0xFF
    
    # Device type (offset 0x25): 0x01 = CDJ
    packet[0x25] = 0x01
    
    # MAC address (offset 0x26, 6 bytes)
    packet[0x26:0x2c] = mac_address[:6]
    
    # IP address (offset 0x2c, 4 bytes)
    packet[0x2c:0x30] = ip_address[:4]
    
    # Peer count (offset 0x30)
    packet[0x30] = 0x01
    
    return bytes(packet)


def build_beat_packet(device_name: str, device_number: int, bpm: float, 
                      beat_within_bar: int, pitch_percent: float = 0.0) -> bytes:
    """
    Build a Beat packet (96 bytes).
    Based on Beat.hpp PACKET_SIZE = 0x60 (96)
    
    Packet structure (from Beat.hpp and DeviceUpdate.hpp):
    - 0x00-0x09: Magic header (10 bytes)
    - 0x0a: Packet type (0x28 for beat)
    - 0x0b-0x1e: Device name (20 bytes) - extractString(data, 0x0b, 20)
    - 0x1f: Unknown (usually 0x01)
    - 0x20: Unknown (usually 0x00)
    - 0x21: Device number - DEVICE_NUMBER_OFFSET = 0x21
    - 0x22-0x23: Unknown
    - 0x24-0x27: Next beat (ms, big-endian, 4 bytes)
    - 0x28-0x2b: Second beat (ms)
    - 0x2c-0x2f: Next bar (ms)
    - 0x30-0x33: Fourth beat (ms)
    - 0x34-0x37: Second bar (ms)
    - 0x38-0x3b: Eighth beat (ms)
    - 0x3c-0x54: Padding/unknown
    - 0x55-0x57: Pitch (3 bytes, big-endian) - bytesToNumber(data, 0x55, 3)
    - 0x58-0x59: Unknown
    - 0x5a-0x5b: BPM * 100 (2 bytes, big-endian) - bytesToNumber(data, 0x5a, 2)
    - 0x5c: Beat within bar (1-4) - packetBytes_[0x5c]
    - 0x5d-0x5f: Padding
    """
    packet = bytearray(96)  # 0x60 bytes
    
    # Magic header (offset 0x00)
    packet[0x00:0x0a] = MAGIC_HEADER
    
    # Packet type (offset 0x0a)
    packet[0x0a] = PACKET_TYPE_BEAT
    
    # Device name - 20 bytes at offset 0x0b (null-padded)
    name_bytes = device_name.encode('ascii')[:20].ljust(20, b'\x00')
    packet[0x0b:0x1f] = name_bytes
    
    # Unknown fields
    packet[0x1f] = 0x01
    packet[0x20] = 0x00
    
    # Device number (offset 0x21)
    packet[0x21] = device_number & 0xFF
    
    # Unknown and subtype
    packet[0x22] = 0x00
    packet[0x23] = 0x3c  # Subtype: stype_beat (from prodj-link packets.py)
    
    # Calculate beat timing (based on BPM)
    if bpm > 0:
        beat_interval_ms = int(60000 / bpm)  # ms per beat
    else:
        beat_interval_ms = 500  # Default 120 BPM
    
    # Next beat timing (offset 0x24, 4 bytes, big-endian)
    struct.pack_into('>I', packet, 0x24, beat_interval_ms)
    
    # Second beat (offset 0x28)
    struct.pack_into('>I', packet, 0x28, beat_interval_ms * 2)
    
    # Next bar - beats until next downbeat (offset 0x2c)
    beats_to_bar = (4 - beat_within_bar + 1) % 4
    if beats_to_bar == 0:
        beats_to_bar = 4
    struct.pack_into('>I', packet, 0x2c, beat_interval_ms * beats_to_bar)
    
    # Fourth beat (offset 0x30)
    struct.pack_into('>I', packet, 0x30, beat_interval_ms * 4)
    
    # Second bar (offset 0x34)
    struct.pack_into('>I', packet, 0x34, beat_interval_ms * (beats_to_bar + 4))
    
    # Eighth beat (offset 0x38)
    struct.pack_into('>I', packet, 0x38, beat_interval_ms * 8)
    
    # Pitch (offset 0x54-0x57, 4 bytes for prodj-link compatibility)
    # beat-link-cpp reads 3 bytes from 0x55, so we set 4 bytes starting at 0x54
    # with first byte being 0x00
    # pitch_percent: -100 to +100, NEUTRAL_PITCH = 1048576 = 0x100000 = normal (0%)
    # Formula: pitchToPercentage = (pitch - NEUTRAL_PITCH) / (NEUTRAL_PITCH / 100.0)
    # Reverse: pitch = NEUTRAL_PITCH + (pitch_percent * NEUTRAL_PITCH / 100)
    pitch_value = int(NEUTRAL_PITCH + (pitch_percent * NEUTRAL_PITCH / 100))
    pitch_value = max(0, min(pitch_value, 0xFFFFFF))  # Clamp to 3 bytes (24 bits)
    # Write as 4 bytes starting at 0x54 (first byte is 0x00 for values <= 0xFFFFFF)
    struct.pack_into('>I', packet, 0x54, pitch_value)
    
    # BPM * 100 (offset 0x5a, 2 bytes, big-endian)
    # e.g., 128.0 BPM -> 12800
    bpm_value = int(bpm * 100)
    bpm_value = max(0, min(bpm_value, 0xFFFF))  # Clamp to 2 bytes
    struct.pack_into('>H', packet, 0x5a, bpm_value)
    
    # Beat within bar (offset 0x5c, 1-4)
    packet[0x5c] = beat_within_bar & 0xFF
    
    return bytes(packet)


def build_cdj_status_packet(device_name: str, device_number: int, bpm: float,
                            beat_within_bar: int, beat_count: int,
                            pitch_percent: float = 0.0,
                            play_state: int = PlayState.PLAYING,
                            is_master: bool = False,
                            is_synced: bool = False,
                            is_on_air: bool = True,
                            track_id: int = 1,
                            track_number: int = 1,
                            loaded_slot: int = 3,  # USB
                            cue_distance: int = 0x1ff) -> bytes:
    """
    Build a CDJ Status packet (approx 284 bytes for XDJ-1000 style).
    Based on CdjStatus.hpp and packets.py StatusPacket
    
    Packet structure (from packets.py and CdjStatus.hpp):
    - 0x00-0x09: Magic header (10 bytes)
    - 0x0a: Packet type (0x0a for CDJ status)
    - 0x0b-0x1e: Device name (20 bytes)
    - 0x1f: Unknown (0x01)
    - 0x20: Revision (0x04 for XDJ-1000)
    - 0x21: Device number
    - 0x22-0x23: Remaining bytes length (0x00f8 = 248 for XDJ-1000)
    - 0x24: Device number (again)
    - 0x25: Unknown (0x00)
    - ... CDJ status content ...
    """
    # XDJ-1000 style packet: 0x26 (38) header + 0xf8 (248) content = 286 bytes
    packet_size = 0x26 + 0xf8  # 286 bytes
    packet = bytearray(packet_size)
    
    # Magic header (offset 0x00)
    packet[0x00:0x0a] = MAGIC_HEADER
    
    # Packet type (offset 0x0a)
    packet[0x0a] = PACKET_TYPE_CDJ_STATUS
    
    # Device name - 20 bytes at offset 0x0b (null-padded)
    name_bytes = device_name.encode('ascii')[:20].ljust(20, b'\x00')
    packet[0x0b:0x1f] = name_bytes
    
    # Unknown and revision
    packet[0x1f] = 0x01
    packet[0x20] = 0x04  # Revision for XDJ-1000
    
    # Device number (offset 0x21)
    packet[0x21] = device_number & 0xFF
    
    # Remaining bytes (0x00f8 for XDJ-1000)
    struct.pack_into('>H', packet, 0x22, 0xf8)
    
    # Device number again and unknown
    packet[0x24] = device_number & 0xFF
    packet[0x25] = 0x00
    
    # === CDJ Status Content (starts at offset 0x26) ===
    content_offset = 0x26
    
    # Activity (offset 0x26-0x27): 0=idle, 1=playing
    activity = 1 if play_state in [PlayState.PLAYING, PlayState.LOOPING, PlayState.CUEING] else 0
    struct.pack_into('>H', packet, content_offset + 0x00, activity)
    
    # Loaded player number (offset 0x28)
    packet[content_offset + 0x02] = device_number  # Loaded from self
    
    # Loaded slot (offset 0x29): 0=empty, 1=cd, 2=sd, 3=usb, 4=rekordbox
    packet[content_offset + 0x03] = loaded_slot
    
    # Track analyze type (offset 0x2a): 0=unknown, 1=rekordbox, 2=file, 5=cd
    packet[content_offset + 0x04] = 1  # rekordbox analyzed
    
    # Padding (offset 0x2b)
    packet[content_offset + 0x05] = 0x00
    
    # Track ID (offset 0x2c-0x2f)
    struct.pack_into('>I', packet, content_offset + 0x06, track_id)
    
    # Track number (offset 0x30-0x33)
    struct.pack_into('>I', packet, content_offset + 0x0a, track_number)
    
    # Unknown fields (offset 0x34-0x4f) - mostly zeros
    # u5, u6, u7, u8 fields
    
    # USB/SD activity indicators (offset 0x6a-0x6b)
    packet[content_offset + 0x44] = 0x00  # 0x100 marker high byte
    packet[content_offset + 0x45] = 0x00  # 0x100 marker low byte  
    packet[content_offset + 0x46] = 0x06  # USB active (0x04=inactive, 0x06=active)
    packet[content_offset + 0x47] = 0x04  # SD inactive
    
    # USB state (offset 0x6c-0x6f): 0=loaded, 4=not_loaded
    struct.pack_into('>I', packet, content_offset + 0x48, 0)  # USB loaded
    
    # SD state (offset 0x70-0x73)
    struct.pack_into('>I', packet, content_offset + 0x4c, 4)  # SD not loaded
    
    # Link available (offset 0x74-0x77)
    struct.pack_into('>I', packet, content_offset + 0x50, 1)
    
    # Play state (offset 0x78-0x7b)
    struct.pack_into('>I', packet, content_offset + 0x54, play_state)
    
    # Firmware (offset 0x7c-0x7f) - 4 ASCII chars
    packet[content_offset + 0x58:content_offset + 0x5c] = b'1.00'
    
    # Padding (offset 0x80-0x83)
    
    # Tempo master count (offset 0x84-0x87)
    struct.pack_into('>I', packet, content_offset + 0x60, 0)
    
    # State mask (offset 0x88-0x89)
    state_mask = 0x84  # Base value (always set bits)
    if is_on_air:
        state_mask |= StateFlags.ON_AIR
    if is_synced:
        state_mask |= StateFlags.SYNC
    if is_master:
        state_mask |= StateFlags.MASTER
    if play_state in [PlayState.PLAYING, PlayState.LOOPING, PlayState.CUEING]:
        state_mask |= StateFlags.PLAY
    struct.pack_into('>H', packet, content_offset + 0x62, state_mask)
    
    # u9 counter (offset 0x8a)
    packet[content_offset + 0x64] = 0xff
    
    # play_state2 (offset 0x8b): 0xfa=playing, 0xfe=stopped
    packet[content_offset + 0x65] = 0xfa if play_state == PlayState.PLAYING else 0xfe
    
    # Physical pitch (offset 0x8c-0x8f) - slider position
    pitch_value = int(NEUTRAL_PITCH + (pitch_percent * NEUTRAL_PITCH / 100))
    pitch_value = max(0, min(pitch_value, 0x200000))
    struct.pack_into('>I', packet, content_offset + 0x66, pitch_value)
    
    # BPM state (offset 0x90-0x91): 0x8000=rekordbox
    struct.pack_into('>H', packet, content_offset + 0x6a, 0x8000)
    
    # BPM (offset 0x92-0x93)
    bpm_value = int(bpm * 100)
    struct.pack_into('>H', packet, content_offset + 0x6c, bpm_value)
    
    # u13 (offset 0x94-0x97)
    struct.pack_into('>I', packet, content_offset + 0x6e, 0x7fffffff)
    
    # Actual pitch (offset 0x98-0x9b) - accounting for master tempo etc
    struct.pack_into('>I', packet, content_offset + 0x72, pitch_value)
    
    # play_state3 (offset 0x9c-0x9d): 0=empty, 1=paused, 9=playing
    play_state3 = 9 if play_state == PlayState.PLAYING else 1
    struct.pack_into('>H', packet, content_offset + 0x76, play_state3)
    
    # u10 (offset 0x9e)
    packet[content_offset + 0x78] = 0x01
    
    # Unknown (offset 0x9f)
    packet[content_offset + 0x79] = 0xff
    
    # Beat count (offset 0xa0-0xa3)
    struct.pack_into('>I', packet, content_offset + 0x7a, beat_count)
    
    # Cue distance (offset 0xa4-0xa5): 0x1ff=no cue, lower=closer
    struct.pack_into('>H', packet, content_offset + 0x7e, cue_distance)
    
    # Beat within bar (offset 0xa6)
    packet[content_offset + 0x80] = beat_within_bar
    
    # Padding (offset 0xa7-0xb5)
    
    # u11 (offset 0xb6-0xb7)
    struct.pack_into('>H', packet, content_offset + 0x90, 0x1000)
    
    # More padding and repeated pitch values
    # physical_pitch2 (offset 0xc0-0xc3)
    struct.pack_into('>I', packet, content_offset + 0x9a, pitch_value)
    
    # actual_pitch2 (offset 0xc4-0xc7)
    struct.pack_into('>I', packet, content_offset + 0x9e, pitch_value)
    
    # packet_count (offset 0xc8-0xcb) - permanently increasing
    struct.pack_into('>I', packet, content_offset + 0xa2, int(time.time() * 10) & 0xFFFFFFFF)
    
    # is_nexus (offset 0xcc): 0x0f=nexus
    packet[content_offset + 0xa6] = 0x0f
    
    return bytes(packet)


def dump_packet(packet: bytes, name: str):
    """Dump packet contents in hex format for debugging."""
    print(f"\n=== {name} ({len(packet)} bytes) ===")
    for i in range(0, len(packet), 16):
        hex_part = ' '.join(f'{b:02x}' for b in packet[i:i+16])
        ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in packet[i:i+16])
        print(f'{i:04x}: {hex_part:<48} {ascii_part}')


class MockDJLinkSender:
    def __init__(self, target_ip: str = '127.0.0.1', device_name: str = 'CDJ-MOCK',
                 device_number: int = 1, bpm: float = 128.0, debug: bool = False):
        self.target_ip = target_ip
        self.device_name = device_name
        self.device_number = device_number
        self.bpm = bpm
        self.beat_within_bar = 1
        self.beat_count = 0
        self.pitch_percent = 0.0
        self.debug = debug
        
        # CDJ Status fields
        self.play_state = PlayState.PLAYING
        self.is_master = False
        self.is_synced = True
        self.is_on_air = True
        self.track_id = random.randint(1, 9999)
        self.track_number = 1
        self.cue_distance = 0x1ff  # No cue
        
        # Mock MAC and IP
        self.mac_address = bytes([0x00, 0x11, 0x22, 0x33, 0x44, self.device_number])
        self.ip_bytes = bytes([int(x) for x in target_ip.split('.')])
        
        # Create UDP sockets
        self.sock_announce = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock_announce.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        
        self.sock_beat = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock_beat.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        
        self.sock_status = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock_status.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    
    def send_announcement(self):
        """Send device announcement packet."""
        packet = build_device_announcement(
            self.device_name, 
            self.device_number,
            self.mac_address,
            self.ip_bytes
        )
        if self.debug:
            dump_packet(packet, "DeviceAnnouncement")
        self.sock_announce.sendto(packet, (self.target_ip, PORT_ANNOUNCEMENT))
    
    def send_beat(self):
        """Send beat packet."""
        packet = build_beat_packet(
            self.device_name,
            self.device_number,
            self.bpm,
            self.beat_within_bar,
            self.pitch_percent
        )
        if self.debug:
            dump_packet(packet, f"Beat (beat={self.beat_within_bar})")
        self.sock_beat.sendto(packet, (self.target_ip, PORT_BEAT))
        
        # Advance beat counters
        self.beat_count += 1
        self.beat_within_bar = (self.beat_within_bar % 4) + 1
    
    def send_status(self):
        """Send CDJ status packet."""
        packet = build_cdj_status_packet(
            self.device_name,
            self.device_number,
            self.bpm,
            self.beat_within_bar,
            self.beat_count,
            self.pitch_percent,
            self.play_state,
            self.is_master,
            self.is_synced,
            self.is_on_air,
            self.track_id,
            self.track_number,
            cue_distance=self.cue_distance
        )
        if self.debug:
            dump_packet(packet, f"CDJ Status")
        self.sock_status.sendto(packet, (self.target_ip, PORT_STATUS))
    
    def get_play_state_name(self) -> str:
        """Get human-readable play state name."""
        names = {
            PlayState.NO_TRACK: "NO_TRACK",
            PlayState.LOADING_TRACK: "LOADING",
            PlayState.PLAYING: "PLAYING",
            PlayState.LOOPING: "LOOPING",
            PlayState.PAUSED: "PAUSED",
            PlayState.CUED: "CUED",
            PlayState.CUEING: "CUEING",
            PlayState.SEEKING: "SEEKING",
            PlayState.END_OF_TRACK: "END",
        }
        return names.get(self.play_state, f"0x{self.play_state:02x}")
    
    def run(self, announce_interval: float = 1.5, status_interval: float = 0.2, 
            use_bpm_timing: bool = True):
        """
        Run the mock sender.
        
        Args:
            announce_interval: Seconds between device announcements
            status_interval: Seconds between CDJ status packets
            use_bpm_timing: If True, send beats at BPM rate. If False, use fixed interval
        """
        print(f"Mock DJ Link Sender")
        print(f"  Target: {self.target_ip}")
        print(f"  Device: {self.device_name} #{self.device_number}")
        print(f"  BPM: {self.bpm}")
        print(f"  Pitch: {self.pitch_percent:+.2f}%")
        print(f"  Track ID: {self.track_id}")
        print(f"  Play State: {self.get_play_state_name()}")
        print(f"  Master: {self.is_master} | Sync: {self.is_synced} | OnAir: {self.is_on_air}")
        print(f"\nPackets: Announcement(50000), Beat(50001), Status(50002)")
        print(f"Press Ctrl+C to stop.\n")
        
        if use_bpm_timing and self.bpm > 0:
            beat_interval = 60.0 / self.bpm
        else:
            beat_interval = 0.5  # Default 120 BPM
        
        last_announce_time = 0
        last_beat_time = 0
        last_status_time = 0
        
        try:
            while True:
                now = time.time()
                
                # Send announcement periodically
                if now - last_announce_time >= announce_interval:
                    self.send_announcement()
                    last_announce_time = now
                    if not self.debug:
                        print(f"[ANNOUNCE] {self.device_name} #{self.device_number}")
                
                # Send status frequently (like real CDJs)
                if now - last_status_time >= status_interval:
                    self.send_status()
                    last_status_time = now
                
                # Send beat at BPM rate (only when playing)
                if now - last_beat_time >= beat_interval:
                    if self.play_state in [PlayState.PLAYING, PlayState.LOOPING, PlayState.CUEING]:
                        self.send_beat()
                        effective_bpm = self.bpm * (1 + self.pitch_percent / 100)
                        flags = []
                        if self.is_master:
                            flags.append("MASTER")
                        if self.is_synced:
                            flags.append("SYNC")
                        if self.is_on_air:
                            flags.append("ON-AIR")
                        flags_str = " [" + ",".join(flags) + "]" if flags else ""
                        if not self.debug:
                            print(f"[BEAT] BPM: {self.bpm:.1f} | Beat: {self.beat_within_bar}/4 | "
                                  f"Pitch: {self.pitch_percent:+.2f}% | Track: {self.track_id}{flags_str}")
                    last_beat_time = now
                
                # Small sleep to avoid busy loop
                time.sleep(0.01)
                
        except KeyboardInterrupt:
            print("\nStopping...")
        finally:
            self.sock_announce.close()
            self.sock_beat.close()
            self.sock_status.close()


def main():
    parser = argparse.ArgumentParser(
        description='Mock DJ Link packet sender',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python mock_sender.py                        # Basic usage
  python mock_sender.py --bpm 140 --pitch 6    # Custom BPM and pitch
  python mock_sender.py --master --track 42    # As tempo master with track ID
  python mock_sender.py --paused               # Simulate paused state
  python mock_sender.py --debug                # Show packet hex dumps
""")
    
    # Basic options
    parser.add_argument('--target', '-t', default='127.0.0.1',
                        help='Target IP address (default: 127.0.0.1)')
    parser.add_argument('--name', '-n', default='CDJ-MOCK',
                        help='Device name (default: CDJ-MOCK)')
    parser.add_argument('--number', '-d', type=int, default=1,
                        help='Device number 1-4 (default: 1)')
    parser.add_argument('--bpm', '-b', type=float, default=128.0,
                        help='BPM (default: 128.0)')
    parser.add_argument('--pitch', '-p', type=float, default=0.0,
                        help='Pitch percentage -100 to +100 (default: 0.0)')
    
    # CDJ Status options
    parser.add_argument('--track', type=int, default=None,
                        help='Track ID (default: random)')
    parser.add_argument('--track-num', type=int, default=1,
                        help='Track number in playlist (default: 1)')
    parser.add_argument('--master', '-M', action='store_true',
                        help='Set as tempo master')
    parser.add_argument('--no-sync', action='store_true',
                        help='Disable sync mode')
    parser.add_argument('--off-air', action='store_true',
                        help='Set channel as off-air')
    
    # Play state options
    parser.add_argument('--paused', action='store_true',
                        help='Start in paused state')
    parser.add_argument('--cued', action='store_true',
                        help='Start in cued state')
    parser.add_argument('--looping', action='store_true',
                        help='Start in looping state')
    
    # Network options
    parser.add_argument('--broadcast', '-B', action='store_true',
                        help='Send to broadcast address (255.255.255.255)')
    
    # Debug
    parser.add_argument('--debug', '-D', action='store_true',
                        help='Show packet hex dump')
    
    args = parser.parse_args()
    
    target = '255.255.255.255' if args.broadcast else args.target
    
    sender = MockDJLinkSender(
        target_ip=target,
        device_name=args.name,
        device_number=args.number,
        bpm=args.bpm,
        debug=args.debug
    )
    
    # Apply options
    sender.pitch_percent = args.pitch
    sender.is_master = args.master
    sender.is_synced = not args.no_sync
    sender.is_on_air = not args.off_air
    
    if args.track is not None:
        sender.track_id = args.track
    sender.track_number = args.track_num
    
    # Set play state
    if args.paused:
        sender.play_state = PlayState.PAUSED
    elif args.cued:
        sender.play_state = PlayState.CUED
    elif args.looping:
        sender.play_state = PlayState.LOOPING
    
    sender.run()


if __name__ == '__main__':
    main()
