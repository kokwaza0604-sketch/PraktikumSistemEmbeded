#!/usr/bin/env python3
"""
SPI Loopback Debug Script
==========================
Reads serial output from ESP32 SPI loopback test,
parses TX/RX hex data, calculates bit error rate,
and plots success/fail statistics using matplotlib.

Usage:
    python debug_spi_loopback.py [--port /dev/ttyUSB0] [--baud 115200]
"""

import serial
import re
import sys
import argparse
import time
from collections import defaultdict

try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARNING] matplotlib not installed. Install with: pip install matplotlib")
    print("          Plotting will be disabled.\n")


def parse_hex_line(line):
    """Parse a TX or RX hex line and return list of byte values."""
    match = re.search(r'(TX|RX):\s*((?:[0-9A-Fa-f]{2}\s*)+)', line)
    if match:
        label = match.group(1)
        hex_str = match.group(2).strip()
        bytes_list = [int(b, 16) for b in hex_str.split()]
        return label, bytes_list
    return None, None


def calculate_bit_errors(tx_bytes, rx_bytes):
    """Calculate bit error rate between TX and RX data."""
    if len(tx_bytes) != len(rx_bytes):
        return -1, -1, -1

    total_bits = len(tx_bytes) * 8
    error_bits = 0
    error_bytes = 0

    for tx, rx in zip(tx_bytes, rx_bytes):
        xor = tx ^ rx
        bit_errors = bin(xor).count('1')
        error_bits += bit_errors
        if tx != rx:
            error_bytes += 1

    ber = error_bits / total_bits if total_bits > 0 else 0
    return error_bits, error_bytes, ber


def plot_results(test_results):
    """Plot test results using matplotlib."""
    if not HAS_MATPLOTLIB:
        print("\n[INFO] Skipping plot (matplotlib not available)")
        return

    test_names = list(test_results.keys())
    statuses = [1 if test_results[t]['pass'] else 0 for t in test_names]
    ber_values = [test_results[t].get('ber', 0) * 100 for t in test_names]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # Bar chart: Pass/Fail
    colors = ['#2ecc71' if s == 1 else '#e74c3c' for s in statuses]
    bars = ax1.bar(range(len(test_names)), statuses, color=colors, edgecolor='black')
    ax1.set_xticks(range(len(test_names)))
    ax1.set_xticklabels(test_names, rotation=30, ha='right', fontsize=9)
    ax1.set_yticks([0, 1])
    ax1.set_yticklabels(['FAIL', 'PASS'])
    ax1.set_title('SPI Loopback Test Results')
    ax1.set_ylabel('Result')

    for bar, status in zip(bars, statuses):
        label = 'PASS' if status == 1 else 'FAIL'
        ax1.text(bar.get_x() + bar.get_width() / 2, bar.get_height() / 2,
                 label, ha='center', va='center', fontweight='bold', color='white')

    # Bar chart: Bit Error Rate
    ax2.bar(range(len(test_names)), ber_values, color='#3498db', edgecolor='black')
    ax2.set_xticks(range(len(test_names)))
    ax2.set_xticklabels(test_names, rotation=30, ha='right', fontsize=9)
    ax2.set_title('Bit Error Rate (BER)')
    ax2.set_ylabel('BER (%)')
    ax2.set_ylim(0, max(ber_values) * 1.2 if max(ber_values) > 0 else 1)

    plt.tight_layout()
    plt.savefig('spi_loopback_results.png', dpi=150)
    print("\n[INFO] Plot saved to spi_loopback_results.png")
    plt.show()


def main():
    parser = argparse.ArgumentParser(description='SPI Loopback Debug Monitor')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--timeout', type=int, default=30, help='Read timeout in seconds')
    parser.add_argument('--file', type=str, help='Read from log file instead of serial')
    args = parser.parse_args()

    test_results = {}
    current_test = None
    tx_data = None
    rx_data = None

    def process_line(line):
        nonlocal current_test, tx_data, rx_data

        line = line.strip()
        if not line:
            return

        print(f"[SERIAL] {line}")

        # Detect test name
        test_match = re.search(r'--- Test:\s*(.+?)\s*\(', line)
        if test_match:
            current_test = test_match.group(1).strip()
            tx_data = None
            rx_data = None
            print(f"\n>>> Test detected: {current_test}")
            return

        # Parse TX/RX data
        label, bytes_list = parse_hex_line(line)
        if label == 'TX':
            tx_data = bytes_list
            print(f"    TX bytes ({len(bytes_list)}): {bytes_list}")
        elif label == 'RX':
            rx_data = bytes_list
            print(f"    RX bytes ({len(bytes_list)}): {bytes_list}")

            # If we have both TX and RX, calculate BER
            if tx_data is not None and rx_data is not None:
                error_bits, error_bytes, ber = calculate_bit_errors(tx_data, rx_data)
                print(f"    Bit errors: {error_bits}, Byte errors: {error_bytes}, BER: {ber:.6f}")

        # Detect result
        result_match = re.search(r'Result:\s*(PASS|FAIL)', line)
        if result_match and current_test:
            passed = result_match.group(1) == 'PASS'
            ber = 0
            if tx_data and rx_data:
                _, _, ber = calculate_bit_errors(tx_data, rx_data)
            test_results[current_test] = {'pass': passed, 'ber': ber}
            print(f"    >>> {current_test}: {'PASS' if passed else 'FAIL'} (BER={ber:.6f})")

        # Detect summary
        if 'TEST SUMMARY' in line:
            print("\n" + "=" * 50)
            print("FINAL RESULTS:")
            for name, result in test_results.items():
                status = "PASS" if result['pass'] else "FAIL"
                print(f"  {name}: {status} (BER={result['ber']:.6f})")
            print("=" * 50)

    # Read from file or serial
    if args.file:
        print(f"[INFO] Reading from file: {args.file}")
        with open(args.file, 'r') as f:
            for line in f:
                process_line(line)
    else:
        print(f"[INFO] Opening serial port {args.port} at {args.baud} baud")
        try:
            ser = serial.Serial(args.port, args.baud, timeout=1)
            print("[INFO] Listening for SPI loopback data... (Ctrl+C to stop)\n")

            start_time = time.time()
            while True:
                if args.timeout and (time.time() - start_time) > args.timeout:
                    print(f"\n[INFO] Timeout ({args.timeout}s) reached")
                    break

                if ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='replace')
                        process_line(line)
                    except UnicodeDecodeError:
                        pass

                # Check if all tests are done
                if len(test_results) >= 5:
                    print("\n[INFO] All 5 tests completed")
                    break

        except serial.SerialException as e:
            print(f"[ERROR] Serial error: {e}")
            sys.exit(1)
        except KeyboardInterrupt:
            print("\n[INFO] Interrupted by user")
        finally:
            if 'ser' in locals():
                ser.close()

    # Plot results
    if test_results:
        plot_results(test_results)
    else:
        print("\n[WARNING] No test results captured")


if __name__ == '__main__':
    main()
