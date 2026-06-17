import serial
import time

# Sesuaikan COM port dengan FTDI kamu
ser = serial.Serial('COM3', 115200, timeout=1) 

print("--- STM32 Monitoring Dashboard ---")

try:
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            print(f"[STM32 LOG]: {line}")
            
except KeyboardInterrupt:
    print("Monitoring Stopped")
    ser.close()