Import("env")
import os
import shutil

# Debug output
print("Extra script running for board: " + env["BOARD"])

# Get the current board and platform package path
board = env["BOARD"]
platform = env.PioPlatform()

if "f103" in board.lower():
    pkg_dir = platform.get_package_dir("framework-stm32cubef1")
    port_rel = ["Middlewares", "Third_Party", "FreeRTOS", "Source", "portable", "GCC", "ARM_CM3"]
else:
    pkg_dir = platform.get_package_dir("framework-stm32cubef4")
    port_rel = ["Middlewares", "Third_Party", "FreeRTOS", "Source", "portable", "GCC", "ARM_CM4F"]

port_dir = os.path.join(pkg_dir, *port_rel)
port_c = os.path.join(port_dir, "port.c")
port_include = port_dir
print("Pkg dir:", pkg_dir)
print("Port dir:", port_dir, "exists:", os.path.isdir(port_dir))

# Add include paths for FreeRTOS headers and portmacro.h
freertos_src = os.path.join(pkg_dir, "Middlewares", "Third_Party", "FreeRTOS", "Source")
freertos_include = os.path.join(freertos_src, "include")
print("FreeRTOS src:", freertos_src, "exists:", os.path.isdir(freertos_src))

# Create a local library folder and copy only the kernel sources we need
project_dir = env.subst('${PROJECT_DIR}')
local_lib = os.path.join(project_dir, 'lib', 'FreeRTOS')
local_src = os.path.join(local_lib, 'Source')
local_port = os.path.join(local_src, 'portable', 'GCC', 'ARM_CM3')
os.makedirs(local_port, exist_ok=True)

files_to_copy = [
    'croutine.c', 'event_groups.c', 'list.c', 'queue.c',
    'stream_buffer.c', 'tasks.c', 'timers.c'
]

for f in files_to_copy:
    src_f = os.path.join(freertos_src, f)
    dst_f = os.path.join(local_src, f)
    if os.path.exists(src_f) and not os.path.exists(dst_f):
        os.makedirs(os.path.dirname(dst_f), exist_ok=True)
        shutil.copyfile(src_f, dst_f)
        print('Copied', src_f, '->', dst_f)

# Copy include and port files
local_include = os.path.join(local_lib, 'include')
if not os.path.exists(local_include):
    shutil.copytree(freertos_include, local_include)
    print('Copied FreeRTOS include to', local_include)

src_port_c = os.path.join(port_dir, 'port.c')
src_port_h = os.path.join(port_dir, 'portmacro.h')
dst_port_c = os.path.join(local_port, 'port.c')
dst_port_h = os.path.join(local_port, 'portmacro.h')
if os.path.exists(src_port_c) and not os.path.exists(dst_port_c):
    shutil.copyfile(src_port_c, dst_port_c)
    print('Copied', src_port_c, '->', dst_port_c)
if os.path.exists(src_port_h) and not os.path.exists(dst_port_h):
    shutil.copyfile(src_port_h, dst_port_h)
    print('Copied', src_port_h, '->', dst_port_h)

# Add include paths for the local library
env.Append(CPPPATH=[local_include, local_port, os.path.join(project_dir, 'include')])
print('Using local FreeRTOS lib at', local_lib)
