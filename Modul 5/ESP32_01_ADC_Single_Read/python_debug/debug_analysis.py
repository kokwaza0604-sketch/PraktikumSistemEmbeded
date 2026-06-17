#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Single Read
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini membaca data serial dari ESP32, mem-parsing nilai ADC mentah,
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
    print("[WARN] matplotlib/numpy not installed. Plotting disabled.")
    print("Install with: pip install matplotlib numpy")


def parse_args():
    parser = argparse.ArgumentParser(
        description='ADC Single Read - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='adc_single_read.csv',
                        help='Nama file output CSV (default: adc_single_read.csv)')
    return parser.parse_args()


def parse_line(line):
    """Parse output dari ESP32: [0001] ADC Raw: 2048 | Persentase: 50.01%"""
    match = re.search(
        r'\[(\d+)\]\s+ADC Raw:\s*(\d+)\s*\|\s*Persentase:\s*([\d.]+)%',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'raw': int(match.group(2)),
            'percentage': float(match.group(3))
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
        writer.writerow(['timestamp', 'counter', 'raw_value', 'percentage'])

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
                                parsed['counter'],
                                parsed['raw'],
                                parsed['percentage']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 60)
    print(f"[INFO] Selesai. Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik dari data yang dikumpulkan."""
    if not HAS_MATPLOTLIB or not data:
        return

    raw_values = [d['raw'] for d in data]

    fig, axes = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('ADC Single Read - Analisis Data', fontsize=14, fontweight='bold')

    # Plot 1: Nilai ADC vs Waktu
    axes[0].plot(timestamps, raw_values, 'b-o', markersize=3, label='ADC Raw')
    axes[0].set_xlabel('Waktu (detik)')
    axes[0].set_ylabel('Nilai ADC (0-4095)')
    axes[0].set_title('Nilai ADC Mentah vs Waktu')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)
    axes[0].set_ylim(-100, 4200)

    # Plot 2: Histogram distribusi nilai
    axes[1].hist(raw_values, bins=30, color='steelblue', edgecolor='black', alpha=0.7)
    axes[1].set_xlabel('Nilai ADC')
    axes[1].set_ylabel('Frekuensi')
    axes[1].set_title(f'Distribusi Nilai ADC (N={len(raw_values)})')
    axes[1].grid(True, alpha=0.3)

    # Tambahkan statistik
    if raw_values:
        stats_text = (f'Min: {min(raw_values)}\n'
                      f'Max: {max(raw_values)}\n'
                      f'Mean: {np.mean(raw_values):.1f}\n'
                      f'Std: {np.std(raw_values):.1f}')
        axes[1].text(0.98, 0.98, stats_text, transform=axes[1].transAxes,
                     fontsize=10, verticalalignment='top', horizontalalignment='right',
                     bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Single Read - Debug & Analysis Tool")
    print("  Modul 04 - ADC | Praktikum Sistem Embedded")
    print("=" * 60)
    print(f"  Port    : {args.port}")
    print(f"  Baud    : {args.baud}")
    print(f"  Durasi  : {args.duration} detik")
    print(f"  Output  : {args.output}")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)  # Tunggu ESP32 reset
        ser.flushInput()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            raw_values = [d['raw'] for d in data]
            print(f"\n[STATISTIK]")
            print(f"  Jumlah sampel : {len(data)}")
            print(f"  Nilai minimum : {min(raw_values)}")
            print(f"  Nilai maksimum: {max(raw_values)}")
            if HAS_MATPLOTLIB:
                print(f"  Rata-rata     : {np.mean(raw_values):.2f}")
                print(f"  Std deviasi   : {np.std(raw_values):.2f}")

            plot_data(timestamps, data, args.output)
        else:
            print("[WARN] Tidak ada data yang berhasil di-parse.")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
