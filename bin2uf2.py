#!/usr/bin/env python3
"""
Simple BIN to UF2 converter for Adafruit Feather M0
Based on: https://github.com/microsoft/uf2

Feather M0 RFM9x uses SAMD21 with Arduino bootloader
Start address: 0x2000 (after 8KB bootloader)
"""

import sys
import struct

UF2_MAGIC_START0 = 0x0A324655  # "UF2\n"
UF2_MAGIC_START1 = 0x9E5D5157  # Randomly selected
UF2_MAGIC_END = 0x0AB16F30   # Randomly selected

# Feather M0 bootloader is 8KB, app starts at 0x2000
APP_START_ADDRESS = 0x2000

def convert_bin_to_uf2(bin_path, uf2_path):
    with open(bin_path, 'rb') as f:
        bin_data = f.read()
    
    print(f"Input file: {bin_path} ({len(bin_data)} bytes)")
    
    # Pad to multiple of 256 bytes
    padding = 256 - (len(bin_data) % 256)
    if padding < 256:
        bin_data += b'\xff' * padding
    
    uf2_data = bytearray()
    num_blocks = 0
    total_blocks = len(bin_data) // 256
    
    for addr in range(0, len(bin_data), 256):
        chunk = bin_data[addr:addr+256]
        if len(chunk) < 256:
            chunk += b'\xff' * (256 - len(chunk))
        
        # UF2 block structure (512 bytes total)
        block = struct.pack('<IIIIIII',
            UF2_MAGIC_START0,
            UF2_MAGIC_START1,
            0x00002000,  # flags (no family ID)
            APP_START_ADDRESS + addr,  # target address
            256,  # payload size
            num_blocks,  # block number
            total_blocks  # total blocks
        )
        block += chunk
        block += b'\x00' * (476 - len(chunk))  # padding to 476 bytes
        block += struct.pack('<I', UF2_MAGIC_END)
        
        uf2_data.extend(block)
        num_blocks += 1
    
    with open(uf2_path, 'wb') as f:
        f.write(uf2_data)
    
    print(f"Output file: {uf2_path} ({len(uf2_data)} bytes, {num_blocks} blocks)")
    print(f"Family ID: 0x239A92A1 (Feather M0)")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: bin2uf2.py input.bin output.uf2")
        sys.exit(1)
    
    convert_bin_to_uf2(sys.argv[1], sys.argv[2])
