"""
==========================================================
 Modul 06 - STM32_01_I2C_Scanner
 Debug & Analisis: Parser serial, logging CSV, plot
==========================================================
"""

import serial
import csv
import re
import sys
import time
from datetime import datetime

try:
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak tersedia, fitur plot dinonaktifkan")


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='i2c_scan_log.csv'):
    """Membaca data serial dari STM32 dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    perangkat_per_scan = []
    scan_timestamps = []
    scan_counts = []

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)  # Tunggu koneksi stabil
        print("[INFO] Port serial terbuka. Menunggu data...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'scan_number', 'jumlah_perangkat', 'alamat_ditemukan'])

            scan_num = 0
            alamat_list = []

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue

                print(f"[SERIAL] {line}")

                # Deteksi nomor scan
                match_scan = re.search(r'Scan #(\d+)', line)
                if match_scan:
                    scan_num = int(match_scan.group(1))
                    alamat_list = []

                # Deteksi alamat perangkat yang ditemukan (format hex di tabel)
                match_addr = re.findall(r'\b([0-9A-Fa-f]{2})\b', line)
                if match_addr and 'Total' not in line and 'Scan' not in line:
                    for addr in match_addr:
                        val = int(addr, 16)
                        if 0x01 <= val <= 0x7F:
                            alamat_list.append(f"0x{addr.upper()}")

                # Deteksi total perangkat
                match_total = re.search(r'Total perangkat ditemukan:\s*(\d+)', line)
                if match_total:
                    jumlah = int(match_total.group(1))
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    addr_str = ';'.join(set(alamat_list)) if alamat_list else 'none'
                    writer.writerow([ts, scan_num, jumlah, addr_str])
                    f.flush()

                    perangkat_per_scan.append(jumlah)
                    scan_timestamps.append(ts)
                    scan_counts.append(scan_num)

                    print(f"[LOG] Scan #{scan_num}: {jumlah} perangkat - {addr_str}")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("[INFO] Port serial ditutup")

    return scan_counts, perangkat_per_scan


def plot_hasil(csv_file='i2c_scan_log.csv'):
    """Membuat plot dari data CSV"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan untuk plot")
        return

    scan_nums = []
    jumlah_list = []

    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                scan_nums.append(int(row['scan_number']))
                jumlah_list.append(int(row['jumlah_perangkat']))
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    if not scan_nums:
        print("[WARN] Tidak ada data untuk diplot")
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.bar(scan_nums, jumlah_list, color='steelblue', edgecolor='navy')
    ax.set_xlabel('Nomor Scan')
    ax.set_ylabel('Jumlah Perangkat Ditemukan')
    ax.set_title('Hasil Pemindaian I2C Bus - STM32')
    ax.set_ylim(0, max(jumlah_list) + 2)
    ax.grid(axis='y', alpha=0.3)

    for i, v in enumerate(jumlah_list):
        ax.text(scan_nums[i], v + 0.1, str(v), ha='center', fontweight='bold')

    plt.tight_layout()
    plt.savefig('i2c_scan_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke i2c_scan_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'i2c_scan_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
