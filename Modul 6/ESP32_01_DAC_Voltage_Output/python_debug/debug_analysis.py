#!/usr/bin/env python3
"""
Debug & Analysis Script - DAC Voltage Output
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Script ini membaca data serial dari ESP32, mem-parsing nilai tegangan DAC,
menampilkan grafik real-time, dan menyimpan data ke file CSV.
"""

import serial
import time
import csv
import argparse
import re
from collections import deque

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib/numpy tidak terinstal. Plotting dinonaktifkan.")
    print("Install dengan: pip install matplotlib numpy")


def parse_args():
    parser = argparse.ArgumentParser(
        description='DAC Voltage Output - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='dac_voltage_output.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<step>,<voltage>,<dac_value>"""
    match = re.search(r'DATA:(\d+),([\d.]+),(\d+)', line)
    if match:
        return {
            'step': int(match.group(1)),
            'voltage': float(match.group(2)),
            'dac_value': int(match.group(3))
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port selama durasi tertentu."""
    data = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print(f"[INFO] Menyimpan ke: {output_file}")
    print("-" * 60)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'step', 'voltage', 'dac_value'])

        while (time.time() - start_time) < duration:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  {line}")
                        parsed = parse_line(line)
                        if parsed:
                            elapsed = time.time() - start_time
                            timestamps.append(elapsed)
                            data.append(parsed)
                            writer.writerow([
                                f"{elapsed:.3f}",
                                parsed['step'],
                                parsed['voltage'],
                                parsed['dac_value']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi data tegangan DAC."""
    if not HAS_MATPLOTLIB or not data:
        print("[INFO] Tidak ada data untuk divisualisasikan")
        return

    voltages = [d['voltage'] for d in data]
    dac_values = [d['dac_value'] for d in data]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('DAC Voltage Output - Analisis', fontsize=14)

    # Plot tegangan vs waktu
    ax1.step(timestamps, voltages, 'b-', where='post', linewidth=2)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Tegangan (V)')
    ax1.set_title('Tegangan Output DAC vs Waktu')
    ax1.set_ylim(-0.2, 3.5)
    ax1.grid(True, alpha=0.3)
    ax1.axhline(y=3.3, color='r', linestyle='--', alpha=0.5, label='VCC (3.3V)')
    ax1.legend()

    # Plot nilai DAC vs waktu
    ax2.step(timestamps, dac_values, 'g-', where='post', linewidth=2)
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Nilai DAC (0-255)')
    ax2.set_title('Nilai DAC vs Waktu')
    ax2.set_ylim(-10, 265)
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('dac_voltage_output.png', dpi=150)
    print("[INFO] Grafik disimpan: dac_voltage_output.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  DAC Voltage Output - Debug & Analysis Tool")
    print("  Modul 05 - DAC & PWM")
    print("=" * 60)
    print(f"  Port     : {args.port}")
    print(f"  Baud     : {args.baud}")
    print(f"  Durasi   : {args.duration} detik")
    print(f"  Output   : {args.output}")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)  # Tunggu ESP32 reset
        ser.reset_input_buffer()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            print(f"\n[STATISTIK]")
            voltages = [d['voltage'] for d in data]
            print(f"  Tegangan min   : {min(voltages):.3f} V")
            print(f"  Tegangan max   : {max(voltages):.3f} V")
            print(f"  Level unik     : {len(set(voltages))}")
            plot_data(timestamps, data)
        else:
            print("[WARN] Tidak ada data yang berhasil di-parse")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
