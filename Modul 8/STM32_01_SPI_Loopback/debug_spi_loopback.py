"""
============================================================
 Debug Script - STM32 SPI Loopback Test
 Modul 07 - SPI & Storage
============================================================
 Parse serial output, calculate BER, plot pass/fail chart
 Usage: python debug_spi_loopback.py [PORT] [BAUD]
============================================================
"""

import serial
import sys
import re
import time
from collections import defaultdict

try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed - charts disabled")

# ----- Configuration -----
PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
TIMEOUT = 1.0

# ----- Data Storage -----
test_results = []
tx_data_log = []
rx_data_log = []
ber_values = []
test_names = []

current_test = {
    'name': '',
    'tx': [],
    'rx': [],
    'result': '',
    'errors': 0,
    'bytes': 0
}


def parse_hex_line(line):
    """Parse hex dump line and extract bytes"""
    match = re.search(r'\[(\d+) bytes\]:\s*((?:[0-9A-Fa-f]{2}\s*)+)', line)
    if match:
        byte_count = int(match.group(1))
        hex_str = match.group(2).strip()
        bytes_list = [int(b, 16) for b in hex_str.split()]
        return bytes_list
    return []


def parse_result_line(line):
    """Parse PASS/FAIL result line"""
    pass_match = re.search(r'Result:\s*PASS\s*\((\d+) errors in (\d+) bytes\)', line)
    if pass_match:
        return 'PASS', int(pass_match.group(1)), int(pass_match.group(2))

    fail_match = re.search(r'Result:\s*FAIL\s*\((\d+) errors in (\d+) bytes\)', line)
    if fail_match:
        return 'FAIL', int(fail_match.group(1)), int(fail_match.group(2))

    return None, 0, 0


def calculate_ber(errors, total_bytes):
    """Calculate Bit Error Rate"""
    if total_bytes == 0:
        return 0.0
    total_bits = total_bytes * 8
    return (errors / total_bits) * 100.0


def print_summary(results):
    """Print test summary"""
    print("\n" + "=" * 60)
    print("  DEBUG ANALYSIS SUMMARY")
    print("=" * 60)

    total = len(results)
    passed = sum(1 for r in results if r['result'] == 'PASS')
    failed = total - passed
    total_errors = sum(r['errors'] for r in results)
    total_bytes = sum(r['bytes'] for r in results)

    print(f"  Total Tests  : {total}")
    print(f"  Passed       : {passed}")
    print(f"  Failed       : {failed}")
    print(f"  Total Bytes  : {total_bytes}")
    print(f"  Total Errors : {total_errors}")
    print(f"  Overall BER  : {calculate_ber(total_errors, total_bytes):.6f}%")
    print()

    for i, r in enumerate(results):
        status = "PASS" if r['result'] == 'PASS' else "FAIL"
        ber = calculate_ber(r['errors'], r['bytes'])
        print(f"  [{i+1}] {r['name']:<30s} {status}  BER={ber:.4f}%")

    print("=" * 60)


def plot_results(results):
    """Plot pass/fail bar chart and BER"""
    if not HAS_MATPLOTLIB or len(results) == 0:
        return

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

    # Bar chart: Pass/Fail
    names = [r['name'][:20] for r in results]
    colors = ['#2ecc71' if r['result'] == 'PASS' else '#e74c3c' for r in results]
    statuses = [1 if r['result'] == 'PASS' else 0 for r in results]

    ax1.bar(range(len(results)), statuses, color=colors, edgecolor='black')
    ax1.set_xticks(range(len(results)))
    ax1.set_xticklabels(names, rotation=45, ha='right', fontsize=8)
    ax1.set_ylabel('Result (1=Pass, 0=Fail)')
    ax1.set_title('SPI Loopback Test Results')
    ax1.set_ylim(-0.1, 1.5)
    ax1.axhline(y=1, color='green', linestyle='--', alpha=0.3)

    # BER chart
    bers = [calculate_ber(r['errors'], r['bytes']) for r in results]
    ax2.bar(range(len(results)), bers, color='#3498db', edgecolor='black')
    ax2.set_xticks(range(len(results)))
    ax2.set_xticklabels(names, rotation=45, ha='right', fontsize=8)
    ax2.set_ylabel('Bit Error Rate (%)')
    ax2.set_title('Bit Error Rate per Test')
    ax2.set_ylim(0, max(bers) * 1.2 if max(bers) > 0 else 1)

    plt.tight_layout()
    filename = 'spi_loopback_results.png'
    plt.savefig(filename, dpi=150)
    print(f"\n[INFO] Chart saved to {filename}")
    plt.close()


def main():
    print(f"[INFO] Connecting to {PORT} at {BAUD} baud...")
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=TIMEOUT)
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open {PORT}: {e}")
        print("[INFO] Running in offline analysis mode...")
        # Demo data for testing
        demo_results = [
            {'name': 'Ascending', 'result': 'PASS', 'errors': 0, 'bytes': 16},
            {'name': 'Alt 0xAA/0x55', 'result': 'PASS', 'errors': 0, 'bytes': 16},
            {'name': 'All 0xFF', 'result': 'PASS', 'errors': 0, 'bytes': 16},
            {'name': 'All 0x00', 'result': 'PASS', 'errors': 0, 'bytes': 16},
            {'name': 'ASCII', 'result': 'PASS', 'errors': 0, 'bytes': 16},
        ]
        print_summary(demo_results)
        plot_results(demo_results)
        return

    print("[INFO] Listening for SPI loopback test data...")
    print("[INFO] Press Ctrl+C to stop and show summary\n")

    results = []
    current_name = ""

    try:
        while True:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            if not line:
                continue

            print(f"  >> {line}")

            # Detect test start
            test_match = re.search(r'--- Test:\s*(.+?)\s*---', line)
            if test_match:
                current_name = test_match.group(1)

            # Detect TX data
            if 'TX [' in line:
                tx_bytes = parse_hex_line(line)
                tx_data_log.append(tx_bytes)

            # Detect RX data
            if 'RX [' in line:
                rx_bytes = parse_hex_line(line)
                rx_data_log.append(rx_bytes)

            # Detect result
            result, errors, byte_count = parse_result_line(line)
            if result:
                results.append({
                    'name': current_name,
                    'result': result,
                    'errors': errors,
                    'bytes': byte_count
                })

    except KeyboardInterrupt:
        print("\n\n[INFO] Stopped by user.")
    finally:
        ser.close()
        if results:
            print_summary(results)
            plot_results(results)
        else:
            print("[INFO] No test results captured.")


if __name__ == '__main__':
    main()
