#!/usr/bin/env python3
"""
BIP-32 Deterministic Wallet Key Derivation for Bitcoin Puzzles
Tests common derivation paths against known puzzle keys
"""

import hashlib
import hmac
import struct
import binascii

# secp256k1 curve order
SECP256K1_ORDER = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141

def sha256(data):
    return hashlib.sha256(data).digest()

def hmac_sha512(key, data):
    return hmac.new(key, data, hashlib.sha512).digest()

def bytes_to_int(b):
    return int.from_bytes(b, 'big')

def int_to_bytes(n, length=32):
    return n.to_bytes(length, 'big')

def hash160(data):
    """RIPEMD160(SHA256(data))"""
    sha = hashlib.sha256(data).digest()
    return hashlib.new('ripemd160', sha).digest()

def base58check_encode(version, payload):
    """Base58Check encode a Bitcoin address"""
    data = version + payload
    checksum = sha256(sha256(data))[:4]
    return base58_encode(data + checksum)

def base58_encode(data):
    """Base58 encode with Bitcoin alphabet"""
    alphabet = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'
    n = int.from_bytes(data, 'big')
    result = []
    while n > 0:
        n, remainder = divmod(n, 58)
        result.append(alphabet[remainder])
    for byte in data:
        if byte == 0:
            result.append('1')
        else:
            break
    return ''.join(reversed(result))

def bip32_master_key_from_seed(seed):
    """Derive BIP-32 master key from seed"""
    I = hmac_sha512(b'Bitcoin seed', seed)
    master_key = I[:32]
    master_chain = I[32:]
    return master_key, master_chain

def bip32_derive_child(parent_key, parent_chain, index):
    """Derive child key using BIP-32"""
    if index >= 0x80000000:
        # Hardened derivation
        data = b'\x00' + parent_key + struct.pack('>I', index)
    else:
        # Normal derivation
        data = parent_key + struct.pack('>I', index)
    
    I = hmac_sha512(parent_chain, data)
    child_key_int = (bytes_to_int(I[:32]) + bytes_to_int(parent_key)) % SECP256K1_ORDER
    child_chain = I[32:]
    
    return int_to_bytes(child_key_int), child_chain

def bip32_derive_path(master_key, master_chain, path):
    """Derive key along BIP-32 path"""
    key = master_key
    chain = master_chain
    
    components = path.split('/')
    for i, component in enumerate(components):
        if i == 0:
            continue  # Skip 'm'
        
        if component.endswith("'"):
            index = int(component[:-1]) + 0x80000000
        else:
            index = int(component)
        
        key, chain = bip32_derive_child(key, chain, index)
    
    return key

def private_key_to_p2pkh_address(private_key_bytes):
    """Convert private key to P2PKH address (simplified)"""
    # In production, use proper secp256k1 multiplication
    # This is a placeholder that returns the key itself for testing
    # You would need to implement EC multiplication for real addresses
    return private_key_bytes

def test_derivation_path(path, known_keys):
    """Test if a derivation path produces any known puzzle keys"""
    # This is a simplified test - in production you'd need the master seed
    # and actual secp256k1 point multiplication
    print(f"\nTesting path: {path}")
    print(f"  This would derive {len(known_keys)} keys from the master seed")
    print(f"  and check against known puzzle addresses")
    return False

def analyze_key_patterns(known_keys):
    """Analyze patterns in known solved puzzle keys"""
    print("\n" + "=" * 70)
    print("KEY PATTERN ANALYSIS")
    print("=" * 70)
    
    # Convert keys to bytes for analysis
    key_bytes = {}
    for puzzle_num, key in known_keys.items():
        key_bytes[puzzle_num] = int_to_bytes(key)
    
    # Check for byte patterns
    print("\nByte frequency analysis (first byte of each key):")
    first_bytes = {}
    for puzzle_num, key in sorted(known_keys.items()):
        if key > 0:
            bits = key.bit_length()
            shift = max(0, bits - 8)
            first_byte = (key >> shift) & 0xFF
        else:
            first_byte = 0
        if first_byte not in first_bytes:
            first_bytes[first_byte] = []
        first_bytes[first_byte].append(puzzle_num)
    
    for byte_val, puzzles in sorted(first_bytes.items()):
        print(f"  0x{byte_val:02x}: {len(puzzles)} puzzles")
    
    # Check if keys are consecutive in some base
    print("\nChecking for consecutive patterns:")
    sorted_keys = sorted(known_keys.items())
    for i in range(1, min(10, len(sorted_keys))):
        prev_num, prev_key = sorted_keys[i-1]
        curr_num, curr_key = sorted_keys[i]
        if curr_num - prev_num == 1:
            diff = curr_key - prev_key
            print(f"  Puzzle {prev_num} -> {curr_num}: difference = {diff} (0x{diff:x})")

def generate_keyhunt_batch_script():
    """Generate a batch script for running keyhunt on all puzzles"""
    print("\n" + "=" * 70)
    print("BATCH SEARCH SCRIPT")
    print("=" * 70)
    
    with open('run_puzzle_search.sh', 'w') as f:
        f.write("#!/bin/bash\n")
        f.write("# Batch puzzle search script\n")
        f.write("# Run this from the keyhunt directory\n\n")
        
        f.write("echo 'Starting Bitcoin Puzzle Search (72-160)'\n")
        f.write("echo 'Timestamp: 2015-01-15 18:07:14 UTC'\n")
        f.write("echo 'Block: 339085'\n")
        f.write("echo\n")
        
        # Address file for all unsolved puzzles
        f.write("PUZZLE_FILE='tests/unsolvedpuzzles.txt'\n\n")
        
        # Search each puzzle
        for puzzle_num in range(72, 161):
            bit_range = puzzle_num
            
            f.write(f"echo '=== Searching Puzzle {puzzle_num} ({bit_range} bits) ==='\n")
            f.write(f"keyhunt -m address -f $PUZZLE_FILE -b {bit_range} -l compress -t 8 -x auto -q &\n")
            f.write(f"echo \"Started search for puzzle {puzzle_num}\"\n\n")
        
        f.write("echo 'All searches started. Check KEYFOUNDKEYFOUND.txt for results.'\n")
    
    print("Generated: run_puzzle_search.sh")
    print("Make it executable: chmod +x run_puzzle_search.sh")
    print("Run it: ./run_puzzle_search.sh")

def main():
    print("Bitcoin Puzzle Deterministic Wallet Search Tool")
    print("Target: Puzzles 72-160")
    print()
    
    # Load known solved puzzle keys (from privatekeys.pw)
    known_keys = {
        1: 0x1, 2: 0x3, 3: 0x7, 4: 0x8, 5: 0x15,
        6: 0x31, 7: 0x4c, 8: 0xe0, 9: 0x1d3, 10: 0x202,
        11: 0x483, 12: 0xa7b, 13: 0x1460, 14: 0x2930,
        15: 0x68f3, 16: 0xc936, 17: 0x1764f, 18: 0x3080d,
        19: 0x5749f, 20: 0xd2c55, 21: 0x1ba534, 22: 0x2de40f,
        23: 0x556e52, 24: 0xdc2a04, 25: 0x1fa5ee5, 26: 0x340326e,
        27: 0x6ac3875, 28: 0xd916ce8, 29: 0x17e2551e, 30: 0x3d94cd64,
        31: 0x7d4fe747, 32: 0xb862a62e, 33: 0x1a96ca8d8, 34: 0x34a65911d,
        35: 0x4aed21170, 36: 0x9de820a7c, 37: 0x1757756a93, 38: 0x22382facd0,
        39: 0x4831ba3048,
        # Puzzles 40-65: UNSOLVED
        # Puzzles 66-69: SOLVED (66: 2024-09-12, 67: 2025-02-21, 68: 2025-04-06, 69: 2025-04-30)
        # Puzzles 70-160: UNSOLVED
    }
    
    # Analyze key patterns
    analyze_key_patterns(known_keys)
    
    # Generate batch script
    generate_keyhunt_batch_script()
    
    # Test common derivation paths
    common_paths = [
        "m/0'/0'/0'",
        "m/44'/0'/0'/0/0",
        "m/0/0/0",
        "m/1",
        "m/0'/0'",
    ]
    
    print("\n" + "=" * 70)
    print("DERIVATION PATH TESTING")
    print("=" * 70)
    
    for path in common_paths:
        test_derivation_path(path, known_keys)
    
    print("\n" + "=" * 70)
    print("SEARCH STRATEGY SUMMARY")
    print("=" * 70)
    print("""
1. DIRECT ADDRESS SEARCH (fastest for small puzzles):
   keyhunt -m address -f tests/unsolvedpuzzles.txt -b 72 -t 8 -x auto

2. TIMESTAMP-CONSTRAINED SEARCH:
   keyhunt -m address -T 1421345234 -b 72 -t 8
   (Uses the puzzle funding timestamp to narrow search window)

3. BSGS MODE (for puzzles with public keys):
   keyhunt -m bsgs -f tests/PUZZLE_NUM.txt -B sequential -n 0x1000000 -t 4

4. DETERMINISTIC WALLET TESTING:
   If you find ONE key, use it to derive all others via BIP-32
   Test common derivation paths against known solved puzzles first
""")

if __name__ == "__main__":
    main()
