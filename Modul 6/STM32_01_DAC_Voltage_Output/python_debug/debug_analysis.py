"""
==========================================================
 Debug Analysis Script - DAC Voltage Output
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data tegangan DAC/PWM dari serial,
            menyimpan ke CSV, dan menampilkan grafik.
==========================================================
"""

import serial
import csv
import time
import sys
import re
import matplotlib.pyplot as plt
from datetime import datetime

# Konfigurasi Serial
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
TIMEOUT = 2

def baca_serial(port=SERIAL_PORT, baud=BAUD_RATE, durasi=30):
    """Membaca data dari serial port selama durasi tertentu."""
    data_list = []
    try:
        ser = serial.Serial(port, baud, timeout=TIMEOUT)
        print(f"[INFO] Terhubung ke {port} @ {baud} baud")
        print(f"[INFO] Membaca data selama {durasi} detik...\n")

        waktu_mulai = time.time()
        while (time.time() - waktu_mulai) < durasi:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                continue

            print(f"  >> {line}")

            # Parsing: [DAC] Step  0: Nilai=   0, Tegangan=0.00 V
            # atau:    [PWM] Step  0: Duty=   0/4095, Tegangan~0.00 V
            match = re.search(r'Step\s+(\d+):.*?(?:Nilai|Duty)=\s*(\d+).*?(?:Tegangan[=~])\s*([\d.]+)', line)
            if match:
                step = int(match.group(1))
                nilai = int(match.group(2))
                tegangan = float(match.group(3))
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 2),
                    'step': step,
                    'nilai_dac': nilai,
                    'tegangan': tegangan
                })

        ser.close()
        print(f"\n[INFO] Selesai. Total data: {len(data_list)} sampel")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
        if 'ser' in locals():
            ser.close()

    return data_list

def simpan_csv(data_list, nama_file='dac_voltage_output.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'step', 'nilai_dac', 'tegangan'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik tegangan output."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    waktu = [d['waktu'] for d in data_list]
    tegangan = [d['tegangan'] for d in data_list]
    nilai_dac = [d['nilai_dac'] for d in data_list]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

    # Grafik tegangan vs waktu
    ax1.plot(waktu, tegangan, 'b-o', markersize=4, label='Tegangan Output')
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Tegangan (V)')
    ax1.set_title('DAC Voltage Output - Tegangan vs Waktu')
    ax1.set_ylim(-0.1, 3.5)
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Grafik nilai DAC vs tegangan
    ax2.plot(nilai_dac, tegangan, 'r-s', markersize=4, label='DAC vs Tegangan')
    ax2.set_xlabel('Nilai DAC (0-4095)')
    ax2.set_ylabel('Tegangan (V)')
    ax2.set_title('Linearitas DAC - Nilai vs Tegangan')
    ax2.set_ylim(-0.1, 3.5)
    ax2.grid(True, alpha=0.3)
    ax2.legend()

    plt.tight_layout()
    plt.savefig('dac_voltage_output.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke dac_voltage_output.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - DAC Voltage Output")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    # Gunakan argumen command line jika ada
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 30

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
