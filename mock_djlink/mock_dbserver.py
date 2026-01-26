#!/usr/bin/env python3
"""
Mock DJ Link DB Server
Provides dummy waveform, artwork, and metadata via TCP.

Protocol flow:
1. Client connects to TCP:12523, sends "RemoteDBServer" query
2. Server replies with actual DB port number
3. Client connects to DB port, sends DBMessage requests
4. Server replies with waveform/artwork/metadata
"""

import socket
import struct
import threading
import time
import math
import argparse
import random
from io import BytesIO

# DBServer query port
DB_QUERY_PORT = 12523
DB_SERVICE_PORT = 12524  # Actual service port we'll use

# Magic values for DBMessage protocol
DB_MESSAGE_MAGIC = 0x872349ae

# Field types
FIELD_INT8 = 0x0f
FIELD_INT16 = 0x10
FIELD_INT32 = 0x11
FIELD_BINARY = 0x14
FIELD_STRING = 0x26

# Request types
REQ_SETUP = 0x0000
REQ_ROOT_MENU = 0x1000
REQ_METADATA = 0x2002
REQ_ARTWORK = 0x2003
REQ_PREVIEW_WAVEFORM = 0x2004
REQ_BEATGRID = 0x2204
REQ_WAVEFORM = 0x2904

# Response types
RESP_SUCCESS = 0x4000
RESP_MENU_HEADER = 0x4001
RESP_ARTWORK = 0x4002
RESP_MENU_ITEM = 0x4101
RESP_MENU_FOOTER = 0x4201
RESP_PREVIEW_WAVEFORM = 0x4402
RESP_BEATGRID = 0x4602
RESP_WAVEFORM = 0x4a02


def encode_field(field_type: int, value) -> bytes:
    """Encode a single DBMessage field."""
    if field_type == FIELD_INT8:
        return bytes([FIELD_INT8, value & 0xFF])
    elif field_type == FIELD_INT16:
        return bytes([FIELD_INT16]) + struct.pack('>H', value)
    elif field_type == FIELD_INT32:
        return bytes([FIELD_INT32]) + struct.pack('>I', value)
    elif field_type == FIELD_BINARY:
        data = value if isinstance(value, bytes) else bytes(value)
        return bytes([FIELD_BINARY]) + struct.pack('>I', len(data)) + data
    elif field_type == FIELD_STRING:
        if isinstance(value, str):
            encoded = value.encode('utf-16-be') + b'\x00\x00'
        else:
            encoded = value + b'\x00\x00'
        length = len(encoded) // 2 + 1
        return bytes([FIELD_STRING]) + struct.pack('>I', length) + encoded
    return b''


def decode_field(data: bytes, offset: int) -> tuple:
    """Decode a single DBMessage field. Returns (value, new_offset)."""
    if offset >= len(data):
        return None, offset
    
    field_type = data[offset]
    offset += 1
    
    if field_type == FIELD_INT8:
        return data[offset], offset + 1
    elif field_type == FIELD_INT16:
        return struct.unpack('>H', data[offset:offset+2])[0], offset + 2
    elif field_type == FIELD_INT32:
        return struct.unpack('>I', data[offset:offset+4])[0], offset + 4
    elif field_type == FIELD_BINARY:
        length = struct.unpack('>I', data[offset:offset+4])[0]
        return data[offset+4:offset+4+length], offset + 4 + length
    elif field_type == FIELD_STRING:
        length = struct.unpack('>I', data[offset:offset+4])[0]
        str_data = data[offset+4:offset+4+(length-1)*2]
        return str_data.decode('utf-16-be', errors='ignore'), offset + 4 + length * 2
    return None, offset


def build_db_message(transaction_id: int, msg_type: int, args: list) -> bytes:
    """Build a DBMessage packet."""
    buf = BytesIO()
    
    # Magic (field)
    buf.write(encode_field(FIELD_INT32, DB_MESSAGE_MAGIC))
    
    # Transaction ID (field)
    buf.write(encode_field(FIELD_INT32, transaction_id))
    
    # Message type (field)
    buf.write(encode_field(FIELD_INT16, msg_type))
    
    # Argument count (field)
    buf.write(encode_field(FIELD_INT8, len(args)))
    
    # Argument types (binary field, 12 bytes)
    arg_types = bytes([a[0] for a in args[:12]]).ljust(12, b'\x00')
    buf.write(encode_field(FIELD_BINARY, arg_types))
    
    # Arguments
    for field_type, value in args:
        buf.write(encode_field(field_type, value))
    
    return buf.getvalue()


def parse_db_message(data: bytes) -> dict:
    """Parse a DBMessage packet."""
    result = {}
    offset = 0
    
    # Magic
    magic, offset = decode_field(data, offset)
    if magic != DB_MESSAGE_MAGIC:
        return None
    
    # Transaction ID
    result['transaction_id'], offset = decode_field(data, offset)
    
    # Message type
    result['type'], offset = decode_field(data, offset)
    
    # Argument count
    arg_count, offset = decode_field(data, offset)
    
    # Argument types (binary)
    arg_types, offset = decode_field(data, offset)
    
    # Arguments
    result['args'] = []
    for i in range(arg_count):
        value, offset = decode_field(data, offset)
        result['args'].append(value)
    
    return result


def generate_sine_waveform(length: int = 400, bpm: float = 128.0) -> bytes:
    """Generate a dummy preview waveform (sine wave pattern)."""
    data = bytearray(length)
    beat_samples = int(length * 60 / (bpm * 4))  # Samples per beat (assuming 4 beats visible)
    
    for i in range(length):
        # Create varying height based on beat pattern
        beat_phase = (i % beat_samples) / beat_samples
        
        # Height: 0-31 (5 bits)
        base_height = int(15 + 10 * math.sin(2 * math.pi * i / 50))
        beat_accent = int(8 * (1 - beat_phase) ** 2) if beat_phase < 0.2 else 0
        height = min(31, max(0, base_height + beat_accent))
        
        # Whiteness/color: 0-7 (3 bits) 
        whiteness = (i // 20) % 8
        
        data[i] = (whiteness << 5) | height
    
    return bytes(data)


def generate_detail_waveform(length: int = 150 * 180, bpm: float = 128.0) -> bytes:
    """Generate a dummy detail waveform (150 samples/sec * 180 sec = 3 min track)."""
    # Header (20 bytes) + data
    header = struct.pack('>I', 1) + struct.pack('>I', length) + b'\x00' * 12
    
    data = bytearray(length)
    samples_per_beat = int(150 * 60 / bpm)
    
    for i in range(length):
        beat_phase = (i % samples_per_beat) / samples_per_beat
        
        # Simulate waveform with beat emphasis
        base = int(12 + 8 * math.sin(2 * math.pi * i / 30))
        accent = int(10 * (1 - beat_phase) ** 3) if beat_phase < 0.15 else 0
        height = min(31, max(0, base + accent + random.randint(-2, 2)))
        whiteness = min(7, int(3 + 2 * math.sin(2 * math.pi * i / 100)))
        
        data[i] = (whiteness << 5) | height
    
    return header + bytes(data)


def generate_beatgrid(bpm: float = 128.0, duration_sec: float = 180.0) -> bytes:
    """Generate a dummy beatgrid."""
    ms_per_beat = 60000 / bpm
    beat_count = int(duration_sec * bpm / 60)
    
    # Header
    buf = BytesIO()
    buf.write(b'\x00' * 4)  # Padding
    buf.write(struct.pack('<I', beat_count))  # Beat count (little-endian)
    buf.write(struct.pack('<I', beat_count * 16))  # Payload size
    buf.write(struct.pack('<I', 1))  # u1
    buf.write(struct.pack('<H', 0))  # u2
    buf.write(struct.pack('<H', 0))  # u3
    
    # Beats
    for i in range(beat_count):
        beat_in_bar = (i % 4) + 1  # 1-4
        time_ms = int(i * ms_per_beat)
        bpm_100 = int(bpm * 100)
        
        buf.write(struct.pack('<H', beat_in_bar))
        buf.write(struct.pack('<H', bpm_100))
        buf.write(struct.pack('<I', time_ms))
        buf.write(b'\xff' * 8)  # Padding
    
    return buf.getvalue()


def generate_artwork(size: int = 80) -> bytes:
    """Generate a dummy artwork (simple colored square)."""
    # Create a simple BMP-like image data (not actual BMP, just raw RGB)
    # Real artwork would be JPEG, but we'll return a colored pattern
    
    # Simple gradient pattern
    data = bytearray()
    for y in range(size):
        for x in range(size):
            r = int(255 * x / size)
            g = int(255 * y / size)
            b = 128
            data.extend([r, g, b])
    
    return bytes(data)


class MockDBServer:
    """Mock DB Server that provides waveform/artwork data."""
    
    def __init__(self, query_port: int = DB_QUERY_PORT, service_port: int = DB_SERVICE_PORT):
        self.query_port = query_port
        self.service_port = service_port
        self.running = False
        self.tracks = {}  # track_id -> track_info
        
        # Pre-generate some dummy tracks
        for i in range(1, 100):
            self.tracks[i] = {
                'id': i,
                'title': f'Track {i}',
                'artist': f'Artist {(i-1) // 10 + 1}',
                'album': f'Album {(i-1) // 5 + 1}',
                'bpm': 120 + (i % 20) * 2,
                'duration': 180 + (i % 60),
                'artwork_id': i,
            }
    
    def start(self):
        """Start the DB server."""
        self.running = True
        
        # Start query listener
        self.query_thread = threading.Thread(target=self._query_listener, daemon=True)
        self.query_thread.start()
        
        # Start service listener
        self.service_thread = threading.Thread(target=self._service_listener, daemon=True)
        self.service_thread.start()
        
        print(f"Mock DB Server started")
        print(f"  Query port: {self.query_port}")
        print(f"  Service port: {self.service_port}")
    
    def stop(self):
        """Stop the DB server."""
        self.running = False
    
    def _query_listener(self):
        """Listen for DB server queries on port 12523."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.settimeout(1.0)
        
        try:
            sock.bind(('0.0.0.0', self.query_port))
            sock.listen(5)
            print(f"[QUERY] Listening on port {self.query_port}")
            
            while self.running:
                try:
                    conn, addr = sock.accept()
                    print(f"[QUERY] Connection from {addr}")
                    
                    # Receive query
                    data = conn.recv(1024)
                    if data and b'RemoteDBServer' in data:
                        # Reply with service port
                        conn.send(struct.pack('>H', self.service_port))
                        print(f"[QUERY] Sent service port {self.service_port}")
                    
                    conn.close()
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        print(f"[QUERY] Error: {e}")
        finally:
            sock.close()
    
    def _service_listener(self):
        """Listen for DB requests on the service port."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.settimeout(1.0)
        
        try:
            sock.bind(('0.0.0.0', self.service_port))
            sock.listen(5)
            print(f"[SERVICE] Listening on port {self.service_port}")
            
            while self.running:
                try:
                    conn, addr = sock.accept()
                    print(f"[SERVICE] Connection from {addr}")
                    
                    # Handle client in separate thread
                    t = threading.Thread(target=self._handle_client, args=(conn, addr), daemon=True)
                    t.start()
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        print(f"[SERVICE] Error: {e}")
        finally:
            sock.close()
    
    def _handle_client(self, conn: socket.socket, addr):
        """Handle a single DB client connection."""
        conn.settimeout(5.0)
        
        try:
            while self.running:
                # Receive message
                data = conn.recv(4096)
                if not data:
                    break
                
                msg = parse_db_message(data)
                if not msg:
                    continue
                
                print(f"[SERVICE] Request type=0x{msg['type']:04x}, args={msg['args']}")
                
                response = self._handle_request(msg)
                if response:
                    conn.send(response)
        except socket.timeout:
            pass
        except Exception as e:
            print(f"[SERVICE] Client error: {e}")
        finally:
            conn.close()
    
    def _handle_request(self, msg: dict) -> bytes:
        """Handle a DB request and return response."""
        req_type = msg['type']
        trans_id = msg['transaction_id']
        args = msg['args']
        
        if req_type == REQ_SETUP:
            # Setup request - just acknowledge
            return build_db_message(trans_id, RESP_SUCCESS, [
                (FIELD_INT32, 0),
            ])
        
        elif req_type == REQ_PREVIEW_WAVEFORM:
            # Preview waveform request
            track_id = args[2] if len(args) > 2 else 1
            track = self.tracks.get(track_id, self.tracks[1])
            waveform = generate_sine_waveform(400, track['bpm'])
            
            print(f"[SERVICE] Sending preview waveform for track {track_id} ({len(waveform)} bytes)")
            return build_db_message(trans_id, RESP_PREVIEW_WAVEFORM, [
                (FIELD_INT32, track_id),
                (FIELD_INT32, len(waveform)),
                (FIELD_BINARY, waveform),
            ])
        
        elif req_type == REQ_WAVEFORM:
            # Detail waveform request  
            track_id = args[2] if len(args) > 2 else 1
            track = self.tracks.get(track_id, self.tracks[1])
            waveform = generate_detail_waveform(150 * track['duration'], track['bpm'])
            
            print(f"[SERVICE] Sending detail waveform for track {track_id} ({len(waveform)} bytes)")
            return build_db_message(trans_id, RESP_WAVEFORM, [
                (FIELD_INT32, track_id),
                (FIELD_INT32, len(waveform)),
                (FIELD_BINARY, waveform),
            ])
        
        elif req_type == REQ_BEATGRID:
            # Beatgrid request
            track_id = args[2] if len(args) > 2 else 1
            track = self.tracks.get(track_id, self.tracks[1])
            beatgrid = generate_beatgrid(track['bpm'], track['duration'])
            
            print(f"[SERVICE] Sending beatgrid for track {track_id} ({len(beatgrid)} bytes)")
            return build_db_message(trans_id, RESP_BEATGRID, [
                (FIELD_INT32, track_id),
                (FIELD_INT32, len(beatgrid)),
                (FIELD_BINARY, beatgrid),
            ])
        
        elif req_type == REQ_ARTWORK:
            # Artwork request
            artwork_id = args[2] if len(args) > 2 else 1
            artwork = generate_artwork(80)
            
            print(f"[SERVICE] Sending artwork {artwork_id} ({len(artwork)} bytes)")
            return build_db_message(trans_id, RESP_ARTWORK, [
                (FIELD_INT32, artwork_id),
                (FIELD_INT32, len(artwork)),
                (FIELD_BINARY, artwork),
            ])
        
        elif req_type == REQ_METADATA:
            # Metadata request
            track_id = args[2] if len(args) > 2 else 1
            track = self.tracks.get(track_id, self.tracks[1])
            
            print(f"[SERVICE] Sending metadata for track {track_id}")
            return build_db_message(trans_id, RESP_MENU_ITEM, [
                (FIELD_INT32, track_id),
                (FIELD_STRING, track['title']),
                (FIELD_STRING, track['artist']),
                (FIELD_STRING, track['album']),
                (FIELD_INT32, int(track['bpm'] * 100)),
                (FIELD_INT32, track['duration']),
                (FIELD_INT32, track['artwork_id']),
            ])
        
        # Default success response
        return build_db_message(trans_id, RESP_SUCCESS, [(FIELD_INT32, 0)])


def main():
    parser = argparse.ArgumentParser(description='Mock DJ Link DB Server')
    parser.add_argument('--query-port', type=int, default=DB_QUERY_PORT,
                        help=f'Query port (default: {DB_QUERY_PORT})')
    parser.add_argument('--service-port', type=int, default=DB_SERVICE_PORT,
                        help=f'Service port (default: {DB_SERVICE_PORT})')
    args = parser.parse_args()
    
    server = MockDBServer(args.query_port, args.service_port)
    server.start()
    
    print("\nMock DB Server running. Press Ctrl+C to stop.\n")
    print("This server provides:")
    print("  - Preview waveform (400 samples)")
    print("  - Detail waveform (150 samples/sec)")
    print("  - Beatgrid")
    print("  - Artwork (80x80 gradient)")
    print("  - Track metadata")
    print()
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopping...")
        server.stop()


if __name__ == '__main__':
    main()
