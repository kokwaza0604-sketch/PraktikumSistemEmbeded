#!/usr/bin/env python3
"""
============================================================================
Debug & Visualisasi: STM32_01_Task_Create_Basic
============================================================================

Parser serial output dan visualisasi data dari program FreeRTOS Task Basic.
Menampilkan grafik real-time:
  1. Toggle count per task (bar chart)
  2. Stack usage per task (bar chart)
  3. Heap usage timeline
  4. Task activity timeline

Format data yang di-parse:
  [DATA]TASK,<nama>,<prioritas>,<stack_free>,<toggle_count>,<tick>
  [DATA]HEAP,<free_bytes>,<min_free>,<tick>
  [DATA]SYSTEM,<task_count>,<uptime_sec>,<tick>

Usage:
  python debug_task_basic.py --port /dev/ttyUSB0
  python debug_task_basic.py --file output.log
  python debug_task_basic.py --demo

============================================================================
"""

import sys
import argparse
import time
import re
from collections import defaultdict
from datetime import datetime

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    from matplotlib.gridspec import GridSpec
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak ditemukan. Install: pip install matplotlib")

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial tidak ditemukan. Install: pip install pyserial")


class TaskBasicParser:
    """Parser untuk data serial dari program Task Create Basic"""

    def __init__(self):
        # Data storage
        self.task_data = defaultdict(lambda: {
            'priority': 0,
            'stack_free': [],
            'toggle_count': [],
            'ticks': []
        })
        self.heap_data = {
            'free': [],
            'min_free': [],
            'ticks': []
        }
        self.system_data = {
            'task_count': [],
            'uptime': [],
            'ticks': []
        }
        self.raw_lines = []
        self.start_time = time.time()

        # Regex patterns untuk parsing
        self.re_task = re.compile(
            r'\[DATA\]TASK,(\w+),(\d+),(\d+),(\d+),(\d+)'
        )
        self.re_heap = re.compile(
            r'\[DATA\]HEAP,(\d+),(\d+),(\d+)'
        )
        self.re_system = re.compile(
            r'\[DATA\]SYSTEM,(\d+),(\d+),(\d+)'
        )

    def parse_line(self, line):
        """Parse satu baris output serial"""
        line = line.strip()
        if not line:
            return

        self.raw_lines.append(line)

        # Parse TASK data
        m = self.re_task.match(line)
        if m:
            name = m.group(1)
            priority = int(m.group(2))
            stack_free = int(m.group(3))
            toggle_count = int(m.group(4))
            tick = int(m.group(5))

            self.task_data[name]['priority'] = priority
            self.task_data[name]['stack_free'].append(stack_free)
            self.task_data[name]['toggle_count'].append(toggle_count)
            self.task_data[name]['ticks'].append(tick)
            return

        # Parse HEAP data
        m = self.re_heap.match(line)
        if m:
            free_bytes = int(m.group(1))
            min_free = int(m.group(2))
            tick = int(m.group(3))

            self.heap_data['free'].append(free_bytes)
            self.heap_data['min_free'].append(min_free)
            self.heap_data['ticks'].append(tick)
            return

        # Parse SYSTEM data
        m = self.re_system.match(line)
        if m:
            task_count = int(m.group(1))
            uptime = int(m.group(2))
            tick = int(m.group(3))

            self.system_data['task_count'].append(task_count)
            self.system_data['uptime'].append(uptime)
            self.system_data['ticks'].append(tick)
            return

    def get_summary(self):
        """Dapatkan ringkasan data yang terkumpul"""
        summary = []
        summary.append("=" * 60)
        summary.append("  RINGKASAN DATA - Task Create Basic")
        summary.append("=" * 60)

        for name, data in self.task_data.items():
            summary.append(f"\n  Task: {name}")
            summary.append(f"    Prioritas     : {data['priority']}")
            if data['toggle_count']:
                summary.append(f"    Toggle Count  : {data['toggle_count'][-1]}")
            if data['stack_free']:
                summary.append(f"    Stack Free    : {data['stack_free'][-1]} words")
                summary.append(f"    Stack Min     : {min(data['stack_free'])} words")
            summary.append(f"    Data points   : {len(data['ticks'])}")

        if self.heap_data['free']:
            summary.append(f"\n  Heap:")
            summary.append(f"    Current Free  : {self.heap_data['free'][-1]} bytes")
            summary.append(f"    Min Free      : {min(self.heap_data['min_free'])} bytes")

        if self.system_data['uptime']:
            summary.append(f"\n  Sistem:")
            summary.append(f"    Uptime        : {self.system_data['uptime'][-1]} detik")
            summary.append(f"    Task Count    : {self.system_data['task_count'][-1]}")

        summary.append("=" * 60)
        return "\n".join(summary)


class TaskBasicVisualizer:
    """Visualisasi real-time data Task Basic"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = None
        self.axes = None

    def setup_plot(self):
        """Buat layout figure matplotlib"""
        self.fig = plt.figure(figsize=(14, 10))
        self.fig.suptitle('STM32 FreeRTOS - Task Create Basic Monitor',
                          fontsize=14, fontweight='bold')

        gs = GridSpec(2, 2, figure=self.fig, hspace=0.35, wspace=0.3)

        self.ax_toggle = self.fig.add_subplot(gs[0, 0])
        self.ax_stack = self.fig.add_subplot(gs[0, 1])
        self.ax_heap = self.fig.add_subplot(gs[1, 0])
        self.ax_activity = self.fig.add_subplot(gs[1, 1])

        self.colors = {
            'Task_LED1': '#2196F3',
            'Task_LED2': '#4CAF50',
            'Task_Monitor': '#FF9800',
        }

    def update_plot(self, frame=None):
        """Update semua subplot"""
        # --- Toggle Count Bar Chart ---
        self.ax_toggle.clear()
        self.ax_toggle.set_title('Toggle Count per Task', fontsize=11)

        names = []
        counts = []
        colors = []
        for name, data in self.parser.task_data.items():
            if data['toggle_count']:
                names.append(name.replace('Task_', ''))
                counts.append(data['toggle_count'][-1])
                colors.append(self.colors.get(name, '#9E9E9E'))

        if names:
            bars = self.ax_toggle.bar(names, counts, color=colors, edgecolor='black')
            for bar, count in zip(bars, counts):
                self.ax_toggle.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                                    f'{count}', ha='center', va='bottom', fontsize=9)
            self.ax_toggle.set_ylabel('Jumlah Toggle')
            self.ax_toggle.grid(axis='y', alpha=0.3)

        # --- Stack Usage Bar Chart ---
        self.ax_stack.clear()
        self.ax_stack.set_title('Stack Free (words)', fontsize=11)

        names = []
        stacks = []
        colors = []
        for name, data in self.parser.task_data.items():
            if data['stack_free']:
                names.append(name.replace('Task_', ''))
                stacks.append(data['stack_free'][-1])
                colors.append(self.colors.get(name, '#9E9E9E'))

        if names:
            bars = self.ax_stack.bar(names, stacks, color=colors, edgecolor='black')
            for bar, s in zip(bars, stacks):
                self.ax_stack.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                                   f'{s}', ha='center', va='bottom', fontsize=9)
            self.ax_stack.set_ylabel('Words Free')
            self.ax_stack.axhline(y=20, color='red', linestyle='--', alpha=0.5,
                                  label='Batas bahaya')
            self.ax_stack.legend(fontsize=8)
            self.ax_stack.grid(axis='y', alpha=0.3)

        # --- Heap Timeline ---
        self.ax_heap.clear()
        self.ax_heap.set_title('Heap Usage Timeline', fontsize=11)

        if self.parser.heap_data['ticks']:
            ticks_sec = [t / 1000.0 for t in self.parser.heap_data['ticks']]
            self.ax_heap.plot(ticks_sec, self.parser.heap_data['free'],
                              'b-o', markersize=3, label='Free', linewidth=1.5)
            self.ax_heap.plot(ticks_sec, self.parser.heap_data['min_free'],
                              'r--', markersize=2, label='Min Free', linewidth=1)
            self.ax_heap.axhline(y=1024, color='red', linestyle=':', alpha=0.5,
                                  label='1KB warning')
            self.ax_heap.set_xlabel('Waktu (detik)')
            self.ax_heap.set_ylabel('Bytes')
            self.ax_heap.legend(fontsize=8)
            self.ax_heap.grid(True, alpha=0.3)

        # --- Task Activity Timeline ---
        self.ax_activity.clear()
        self.ax_activity.set_title('Task Toggle Activity', fontsize=11)

        for name, data in self.parser.task_data.items():
            if data['ticks'] and data['toggle_count']:
                ticks_sec = [t / 1000.0 for t in data['ticks']]
                color = self.colors.get(name, '#9E9E9E')
                self.ax_activity.plot(ticks_sec, data['toggle_count'],
                                      '-o', markersize=2, color=color,
                                      label=name.replace('Task_', ''),
                                      linewidth=1.5)

        self.ax_activity.set_xlabel('Waktu (detik)')
        self.ax_activity.set_ylabel('Toggle Count')
        self.ax_activity.legend(fontsize=8)
        self.ax_activity.grid(True, alpha=0.3)

        self.fig.canvas.draw_idle()

    def show(self):
        """Tampilkan plot"""
        self.setup_plot()
        self.update_plot()
        plt.tight_layout()
        plt.show()


def generate_demo_data(parser):
    """Generate data demo untuk testing tanpa hardware"""
    import random
    random.seed(42)

    print("  Generating demo data...")

    for i in range(50):
        tick = i * 2000

        # Task LED1 data (500ms blink)
        toggle1 = (i + 1) * 4
        stack1 = 200 - random.randint(0, 10)
        parser.parse_line(
            f"[DATA]TASK,Task_LED1,2,{stack1},{toggle1},{tick}"
        )

        # Task LED2 data (200ms blink)
        toggle2 = (i + 1) * 10
        stack2 = 195 - random.randint(0, 8)
        parser.parse_line(
            f"[DATA]TASK,Task_LED2,1,{stack2},{toggle2},{tick}"
        )

        # Monitor task
        stack_mon = 350 - random.randint(0, 15)
        parser.parse_line(
            f"[DATA]TASK,Task_Monitor,3,{stack_mon},0,{tick}"
        )

        # Heap data
        heap_free = 5500 - random.randint(0, 50)
        heap_min = min(5400, heap_free)
        parser.parse_line(f"[DATA]HEAP,{heap_free},{heap_min},{tick}")

        # System data
        uptime = i * 2
        parser.parse_line(f"[DATA]SYSTEM,5,{uptime},{tick}")

    print(f"  Generated {len(parser.raw_lines)} data points")


def read_serial(port, baudrate, parser, visualizer=None):
    """Baca data dari serial port"""
    if not HAS_SERIAL:
        print("[ERROR] pyserial diperlukan. Install: pip install pyserial")
        return

    print(f"  Menghubungkan ke {port} @ {baudrate} baud...")

    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"  Terhubung! Tekan Ctrl+C untuk berhenti.\n")

        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if line:
                    print(f"  >> {line}")
                    parser.parse_line(line)

                    if visualizer and len(parser.raw_lines) % 10 == 0:
                        visualizer.update_plot()

    except KeyboardInterrupt:
        print("\n  Dihentikan oleh user.")
    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
    finally:
        if 'ser' in locals():
            ser.close()
            print("  Serial port ditutup.")


def read_file(filepath, parser):
    """Baca data dari file log"""
    print(f"  Membaca file: {filepath}")

    try:
        with open(filepath, 'r') as f:
            line_count = 0
            for line in f:
                parser.parse_line(line)
                line_count += 1

        print(f"  Selesai membaca {line_count} baris")
        data_count = sum(len(d['ticks']) for d in parser.task_data.values())
        print(f"  Data points: {data_count}")

    except FileNotFoundError:
        print(f"[ERROR] File tidak ditemukan: {filepath}")
        sys.exit(1)


def main():
    """Entry point utama"""
    arg_parser = argparse.ArgumentParser(
        description='Debug & Visualisasi STM32 FreeRTOS Task Basic',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh penggunaan:
  %(prog)s --port /dev/ttyUSB0           # Serial monitor
  %(prog)s --port COM3 --baud 115200     # Windows serial
  %(prog)s --file output.log             # Parse file log
  %(prog)s --demo                        # Mode demo (tanpa hardware)
        """
    )

    arg_parser.add_argument('--port', type=str, help='Serial port (misal /dev/ttyUSB0)')
    arg_parser.add_argument('--baud', type=int, default=115200, help='Baudrate (default: 115200)')
    arg_parser.add_argument('--file', type=str, help='File log untuk di-parse')
    arg_parser.add_argument('--demo', action='store_true', help='Mode demo dengan data simulasi')
    arg_parser.add_argument('--no-plot', action='store_true', help='Tanpa visualisasi grafik')

    args = arg_parser.parse_args()

    # Header
    print()
    print("=" * 60)
    print("  STM32 FreeRTOS - Task Create Basic - Debug Tool")
    print(f"  Waktu: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 60)
    print()

    parser = TaskBasicParser()
    visualizer = None

    if not args.no_plot and HAS_MATPLOTLIB:
        visualizer = TaskBasicVisualizer(parser)

    if args.demo:
        generate_demo_data(parser)
        print()
        print(parser.get_summary())

        if visualizer:
            visualizer.show()

    elif args.file:
        read_file(args.file, parser)
        print()
        print(parser.get_summary())

        if visualizer:
            visualizer.show()

    elif args.port:
        if visualizer:
            visualizer.setup_plot()

        read_serial(args.port, args.baud, parser, visualizer)
        print()
        print(parser.get_summary())

        if visualizer:
            visualizer.show()

    else:
        print("  Gunakan --port, --file, atau --demo")
        print("  Jalankan --help untuk bantuan lengkap")
        print()
        print("  Menjalankan mode demo...")
        print()
        generate_demo_data(parser)
        print()
        print(parser.get_summary())

        if visualizer:
            visualizer.show()


if __name__ == '__main__':
    main()
