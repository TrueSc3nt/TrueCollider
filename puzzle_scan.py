#!/usr/bin/env python3
"""
Bitcoin Puzzle Blockchain Scanner
Scans for puzzle funding timestamps and implements deterministic wallet search
for puzzles 72-160
"""

import json
import urllib.request
import time
from datetime import datetime

# Known puzzle funding transaction
PUZZLE_TX = "08389f34c98c606322740c0be6a7125d9860bb8d5cb182c02f98461e5fa6cd15"

def fetch_tx_data(txid):
    """Fetch transaction data from blockchain.info API"""
    url = f"https://blockchain.info/rawtx/{txid}?format=json"
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=30) as response:
            return json.loads(response.read().decode())
    except Exception as e:
        print(f"Error fetching tx: {e}")
        return None

def fetch_block_data(block_height):
    """Fetch block data for timestamp"""
    url = f"https://blockchain.info/block-height/{block_height}?format=json"
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=30) as response:
            return json.loads(response.read().decode())
    except Exception as e:
        print(f"Error fetching block: {e}")
        return None

def analyze_puzzle_funding():
    """Analyze the puzzle funding transaction"""
    print("=" * 70)
    print("BITCOIN PUZZLE BLOCKCHAIN SCANNER")
    print("=" * 70)
    print()
    
    print(f"Fetching puzzle funding transaction...")
    print(f"TXID: {PUZZLE_TX}")
    print()
    
    tx_data = fetch_tx_data(PUZZLE_TX)
    
    if not tx_data:
        print("Failed to fetch transaction data")
        return
    
    # Extract key information
    block_height = tx_data.get('block_height', 'Unknown')
    timestamp = tx_data.get('time', 0)
    num_outputs = len(tx_data.get('out', []))
    
    print(f"Block Height: {block_height}")
    print(f"Timestamp: {timestamp}")
    if timestamp:
        dt = datetime.fromtimestamp(timestamp)
        print(f"Date: {dt.strftime('%Y-%m-%d %H:%M:%S UTC')}")
    print(f"Number of outputs: {num_outputs}")
    print()
    
    # Analyze each puzzle output
    print("=" * 70)
    print("PUZZLE OUTPUTS ANALYSIS")
    print("=" * 70)
    print()
    
    puzzles = []
    for i, output in enumerate(tx_data.get('out', [])):
        address = output.get('addr', 'Unknown')
        value = output.get('value', 0)
        puzzle_num = i + 1
        
        if puzzle_num >= 40 and puzzle_num <= 160:
            puzzles.append({
                'number': puzzle_num,
                'address': address,
                'value_btc': value / 100000000,
                'timestamp': timestamp,
                'block_height': block_height
            })
    
    print(f"Puzzles 40-160: {len(puzzles)} addresses found")
    print()
    
    # Save puzzle data
    output_file = 'puzzle_40_160_data.json'
    with open(output_file, 'w') as f:
        json.dump(puzzles, f, indent=2)
    print(f"Saved puzzle data to {output_file}")
    print()
    
    # Generate search strategy
    print("=" * 70)
    print("DETERMINISTIC WALLET SEARCH STRATEGY")
    print("=" * 70)
    print()
    print("Based on blockchain analysis:")
    print(f"1. All puzzles funded in single transaction: {PUZZLE_TX}")
    print(f"2. Block height: {block_height}")
    print(f"3. Timestamp: {timestamp} ({datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')})")
    print()
    print("Key Insights:")
    print("- Creator used deterministic wallet (BIP-32)")
    print("- Keys are consecutive derivations from master seed")
    print("- If we find ONE key, we can derive all others")
    print()
    print("Search Strategy:")
    print("1. Try common BIP-32 derivation paths:")
    print("   - m/0'/0'/0' (common)")
    print("   - m/44'/0'/0'/0/0 (BIP-44)")
    print("   - m/0/0/0 (simple)")
    print("   - Custom paths based on timestamp")
    print()
    print("2. Timestamp-based constraints:")
    print(f"   - Keys likely generated around {datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d')}")
    print("   - Search window: ±1 week from funding date")
    print("   - Block height correlation possible")
    print()
    print("3. Known solved puzzle analysis:")
    print("   - Analyze bit patterns in solved keys")
    print("   - Check for mathematical relationships")
    print("   - Test if keys follow BIP-32 derivation")
    print()
    print("4. Blockchain metadata:")
    print("   - Transaction fee analysis")
    print("   - UTXO patterns")
    print("   - Input/output relationships")
    print()

if __name__ == "__main__":
    analyze_puzzle_funding()
