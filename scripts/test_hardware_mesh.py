#!/usr/bin/env python3
import argparse
import time
import sys
import serial
import re
import threading

"""
MeshCore-BitChat Hardware Test Script
Story 9.3: Hardware-in-Loop Tests

This script connects to two MeshCore devices via USB serial and performs
end-to-end verification of BitChat features.

Requirements:
    pip install pyserial
"""

class MeshDevice:
    def __init__(self, port, baud=115200, name="Device"):
        self.port = port
        self.baud = baud
        self.name = name
        self.ser = None
        self.buffer = ""
        self.lock = threading.Lock()
        self.running = False
        self.thread = None
        self.received_messages = []

    def connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
            self.running = True
            self.thread = threading.Thread(target=self._reader)
            self.thread.daemon = True
            self.thread.start()
            print(f"[{self.name}] Connected to {self.port}")
            return True
        except Exception as e:
            print(f"[{self.name}] Failed to connect: {e}")
            return False

    def disconnect(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=2)
        if self.ser and self.ser.is_open:
            self.ser.close()

    def send(self, cmd):
        if not self.ser: return
        full_cmd = cmd + "\n"
        print(f"[{self.name}] Sending: {cmd}")
        self.ser.write(full_cmd.encode('utf-8'))
        time.sleep(0.1)

    def _reader(self):
        while self.running and self.ser and self.ser.is_open:
            try:
                line = self.ser.readline().decode('utf-8', errors='replace').strip()
                if line:
                    print(f"[{self.name}] Rcv: {line}")
                    self._parse_line(line)
            except Exception as e:
                print(f"[{self.name}] Error reading: {e}")
                break

    def _parse_line(self, line):
        # Detect received BitChat messages
        # Format usually: [BitChat] <Sender> MessageContent
        if "Received BitChat message" in line or "[BitChat]" in line:
            with self.lock:
                self.received_messages.append(line)

    def wait_for_message(self, content, timeout=10):
        start_time = time.time()
        while time.time() - start_time < timeout:
            with self.lock:
                for msg in self.received_messages:
                    if content in msg:
                        return True
            time.sleep(0.5)
        return False

def run_test(port_a, port_b):
    alice = MeshDevice(port_a, name="Alice")
    bob = MeshDevice(port_b, name="Bob")

    if not alice.connect() or not bob.connect():
        return False

    try:
        # Reset input buffers with some newlines
        alice.send("")
        bob.send("")
        time.sleep(1)

        # 1. Enable BitChat on both
        print("\n--- Step 1: Enabling BitChat ---")
        alice.send("bitchat enable")
        bob.send("bitchat enable")
        time.sleep(2)
        
        # 2. Check status
        # (Optional: parse output of 'bitchat status')

        # 3. Alice sends message to #mesh
        message_content = f"TestMessage_{int(time.time())}"
        print(f"\n--- Step 2: Alice sending '{message_content}' ---")
        alice.send(f"bitchat send #mesh {message_content}")

        # 4. Wait for Bob to receive
        print("\n--- Step 3: Waiting for Bob to receive ---")
        if bob.wait_for_message(message_content, timeout=15):
            print("✅ SUCCESS: Bob received message from Alice")
        else:
            print("❌ FAILURE: Bob did not receive message")
            return False

        # 5. Bob replies
        reply_content = f"Reply_{int(time.time())}"
        print(f"\n--- Step 4: Bob replying '{reply_content}' ---")
        bob.send(f"bitchat send #mesh {reply_content}")

        # 6. Wait for Alice to receive
        print("\n--- Step 5: Waiting for Alice to receive ---")
        if alice.wait_for_message(reply_content, timeout=15):
            print("✅ SUCCESS: Alice received reply from Bob")
        else:
            print("❌ FAILURE: Alice did not receive reply")
            return False

        return True

    finally:
        alice.disconnect()
        bob.disconnect()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MeshCore-BitChat Hardware Test")
    parser.add_argument("--port-a", required=True, help="Serial port for Device A (Alice)")
    parser.add_argument("--port-b", required=True, help="Serial port for Device B (Bob)")
    
    args = parser.parse_args()
    
    success = run_test(args.port_a, args.port_b)
    sys.exit(0 if success else 1)
