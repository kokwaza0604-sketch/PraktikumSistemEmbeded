#!/usr/bin/env python3
"""
Debug & Analisis - ESP32_01_I2C_Scanner
Modul 06 - I2C & Sensor

Membaca output serial dari I2C Scanner, mencatat perangkat
yang ditemukan ke CSV, dan menampilkan grafik statistik.
"""

import argparse
import csv
import re
import sys
import os
from datetime import datetime
from collections import Counter

import serial
import matplotlib.pyplot as plt


def parse_args():
    """Parsing argumen baris perintah."""
    parser = argparse.ArgumentParser(
        description="Analisis output I2C Scanner ESP32"
    )
    parser.add_argument(
        "--port", "-p",
        default="/dev/ttyUSB0",
        help="Port serial (default: /dev/ttyUSB0)"
    )
    parser.add_argument(
        "--baud", "-b",
        type=int,
        default=115200,
        help="Baud rate (default: 115200)"
    )
    parser.add_argument(
        "--output", "-o",
        default="i2c_scan_log.csv",
        help="File output CSV (default: i2c_scan_log.csv)"
    )
    parser.add_argument(
        "--duration", "-d",
        type=int,
        default=60,
        help="Durasi pemantauan dalam detik (default: 60)"
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Buka koneksi serial
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        print(f"[INFO] Terhubung ke {args.port} @ {args.baud} baud")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        sys.exit(1)

    # Siapkan file CSV
    csv_file = open(args.output, "w", newline="")
    writer = csv.writer(csv_file)
    writer.writerow(["timestamp", "alamat", "nama_perangkat"])

    # Pola regex untuk parsing output
    device_pattern = re.compile(r"DEVICE_FOUND:\s*addr=(0x[0-9A-Fa-f]+)\s*name=(.+)")
    scan_pattern = re.compile(r"SCAN_COMPLETE:\s*total=(\d+)")

    # Penghitung statistik
    device_counter = Counter()
    scan_results = []  # jumlah perangkat per scan
    timestamps = []

    print("[INFO] Memulai pemantauan... (Ctrl+C untuk berhenti)")
    start_time = datetime.now()

    try:
        while True:
            # Cek durasi
            elapsed = (datetime.now() - start_time).total_seconds()
            if elapsed > args.duration:
                print(f"[INFO] Durasi {args.duration}s tercapai, berhenti.")
                break

            line = ser.readline().decode("utf-8", errors="replace").strip()
            if not line:
                continue

            # Cek apakah baris berisi info perangkat
            m_dev = device_pattern.search(line)
            if m_dev:
                addr = m_dev.group(1)
                name = m_dev.group(2).strip()
                ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                writer.writerow([ts, addr, name])
                device_counter[addr] += 1
                print(f"  [PERANGKAT] {addr} - {name}")

            # Cek apakah scan selesai
            m_scan = scan_pattern.search(line)
            if m_scan:
                total = int(m_scan.group(1))
                scan_results.append(total)
                timestamps.append(datetime.now())
                print(f"  [SCAN] Total perangkat: {total}")

    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")
    finally:
        csv_file.close()
        ser.close()

    # Tampilkan ringkasan
    print("\n" + "=" * 40)
    print("RINGKASAN PEMINDAIAN I2C")
    print("=" * 40)
    print(f"Total scan dilakukan : {len(scan_results)}")
    print(f"Perangkat unik       : {len(device_counter)}")
    for addr, count in device_counter.most_common():
        print(f"  {addr} : terdeteksi {count}x")
    print(f"Data disimpan ke     : {args.output}")

    # Buat grafik jika ada data
    if scan_results:
        fig, axes = plt.subplots(1, 2, figsize=(12, 5))

        # Grafik 1: Jumlah perangkat per scan
        axes[0].plot(range(1, len(scan_results) + 1), scan_results,
                     marker="o", color="steelblue")
        axes[0].set_xlabel("Scan ke-")
        axes[0].set_ylabel("Jumlah Perangkat")
        axes[0].set_title("Jumlah Perangkat per Scan")
        axes[0].grid(True, alpha=0.3)

        # Grafik 2: Frekuensi deteksi per alamat
        if device_counter:
            addrs = list(device_counter.keys())
            counts = list(device_counter.values())
            axes[1].barh(addrs, counts, color="coral")
            axes[1].set_xlabel("Jumlah Deteksi")
            axes[1].set_title("Frekuensi Deteksi per Alamat")
            axes[1].grid(True, axis="x", alpha=0.3)

        plt.tight_layout()
        plot_file = args.output.replace(".csv", "_plot.png")
        plt.savefig(plot_file, dpi=150)
        print(f"Grafik disimpan ke   : {plot_file}")
        plt.show()


if __name__ == "__main__":
    main()
