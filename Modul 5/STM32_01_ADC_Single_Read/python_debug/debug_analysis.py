"""
=============================================================================
 Program     : STM32_01_ADC_Single_Read - Debug & Analisis
 Modul       : 04 - ADC (Analog to Digital Converter)
 Deskripsi   : Serial monitor untuk membaca data ADC single read,
               menyimpan ke CSV, dan menampilkan grafik real-time.
=============================================================================
"""

import serial
import csv
import time
import re
import os
import sys
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime
from collections import deque

# ======================== KONFIGURASI ========================
SERIAL_PORT = '/dev/ttyUSB0'  # Sesuaikan dengan port yang digunakan
BAUD_RATE = 115200
CSV_FILENAME = 'adc_single_read_log.csv'
MAX_DATA_POINTS = 200  # Jumlah titik data maksimum di grafik

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
adc_values = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """
    Parsing baris data dari serial monitor
    Format: [counter] ADC_RAW:value
    """
    match = re.search(r'\[(\d+)\]\s*ADC_RAW:(\d+)', line)
    if match:
        counter = int(match.group(1))
        adc_raw = int(match.group(2))
        return {'counter': counter, 'adc_raw': adc_raw}
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke file CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'adc_raw']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            'counter': data['counter'],
            'adc_raw': data['adc_raw']
        })

def update_plot(frame):
    """Update grafik secara real-time"""
    ax.clear()
    if len(timestamps) > 0:
        ax.plot(list(timestamps), list(adc_values), 'b-o', markersize=3, label='ADC Raw')
        ax.set_ylim(-100, 4200)
        ax.axhline(y=4095, color='r', linestyle='--', alpha=0.5, label='Maks (4095)')
        ax.axhline(y=0, color='g', linestyle='--', alpha=0.5, label='Min (0)')
    ax.set_title('STM32 ADC Single Read - Real-time Monitor')
    ax.set_xlabel('Waktu (detik)')
    ax.set_ylabel('Nilai ADC (0-4095)')
    ax.legend(loc='upper right')
    ax.grid(True, alpha=0.3)

def main():
    """Fungsi utama program"""
    global ax

    print("=" * 60)
    print("  STM32 ADC Single Read - Serial Monitor & Analyzer")
    print("=" * 60)
    print(f"  Port    : {SERIAL_PORT}")
    print(f"  Baud    : {BAUD_RATE}")
    print(f"  Log CSV : {CSV_FILENAME}")
    print("=" * 60)

    # Buat figure untuk plotting
    fig, ax = plt.subplots(figsize=(10, 6))
    plt.tight_layout()

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)  # Tunggu reset MCU
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=500, cache_frame_data=False)

        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  >> {line}")
                        data = parse_line(line)
                        if data:
                            elapsed = time.time() - start_time
                            timestamps.append(elapsed)
                            adc_values.append(data['adc_raw'])
                            save_to_csv(data, CSV_FILENAME)
                except Exception as e:
                    print(f"[ERROR] Parsing: {e}")

            plt.pause(0.01)

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
        print("[INFO] Pastikan kabel USB terhubung dan port benar")
    except KeyboardInterrupt:
        print("\n\n[INFO] Program dihentikan oleh pengguna")
        print(f"[INFO] Data tersimpan di: {CSV_FILENAME}")
        print(f"[INFO] Total data: {len(adc_values)} sampel")
    finally:
        try:
            ser.close()
            print("[INFO] Port serial ditutup")
        except:
            pass

if __name__ == '__main__':
    main()
