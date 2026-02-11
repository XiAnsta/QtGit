import socket
import struct
import time
import math
import random

# Configuration
HOST = '127.0.0.1'
PORT = 1234

def create_packet():
    # Header: 0xAAAA (2 bytes)
    header = 0xAAAA
    
    # Payload Logic
    # Monitor Data: Voltage (Double), Current (Double), Temp (Double)
    voltage = 12.0 + random.uniform(-0.1, 0.1)
    current = 24.5 + random.uniform(-0.5, 0.5)
    temp = 35.0 + random.uniform(-0.2, 0.2)
    
    # Pack Monitor Data (3 doubles = 24 bytes)
    monitor_data = struct.pack('>ddd', voltage, current, temp)
    
    # Waveform Data?
    # Our C++ parser currently generates waveforms locally if it sees a valid packet header.
    # But if we were sending real waveform data, we'd append it here.
    # The C++ code checks for payload length.
    # Let's send just the monitor data for now, and let C++ generate the mock waveforms 
    # as implemented in DataParser::parseOnePacket (mock logic).
    
    payload = monitor_data
    
    # Length: Header(2) + Length(4) + Payload(N)
    total_length = 2 + 4 + len(payload)
    
    # Pack: Header (H), Length (I), Payload (s)
    # Big Endian (>)
    packet = struct.pack('>HI', header, total_length) + payload
    
    return packet

def run_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print(f"Mock Device Server listening on {HOST}:{PORT}")
        
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            try:
                while True:
                    # Receive commands (non-blocking or check?)
                    # For simplicity, just blast data at 10Hz
                    packet = create_packet()
                    conn.sendall(packet)
                    time.sleep(0.1) # 10Hz
            except ConnectionResetError:
                print("Connection closed by client.")
            except BrokenPipeError:
                print("Connection closed.")

if __name__ == "__main__":
    run_server()
