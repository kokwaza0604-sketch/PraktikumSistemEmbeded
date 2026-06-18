"""
Custom PlatformIO build script for STM32CubeF4 + FreeRTOS
Adds FreeRTOS include paths, compiles FreeRTOS kernel, and sets FPU flags.
"""
Import("env")
import os

board = env.BoardConfig()
mcu = board.get("build.mcu", "stm32f411ceu6")

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-stm32cubef4")

if not FRAMEWORK_DIR:
    print("Warning: framework-stm32cubef4 not found, skipping FreeRTOS setup")
else:
    # FPU flags for Cortex-M4F
    fpu_flags = ["-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"]
    env.Append(ASFLAGS=fpu_flags, CCFLAGS=fpu_flags, LINKFLAGS=fpu_flags)
    for flag_list_name in ("ASFLAGS", "CCFLAGS", "LINKFLAGS"):
        flag_list = env.get(flag_list_name, [])
        env.Replace(**{flag_list_name: [f for f in flag_list if f != "-mfloat-abi=soft"]})

    framework_includes = []

    def add_include_path(base_path, relative_paths):
        for rel_path in relative_paths:
            full_path = os.path.join(base_path, rel_path)
            if os.path.exists(full_path):
                framework_includes.append(full_path)

    add_include_path(FRAMEWORK_DIR, [
        "Drivers/STM32F4xx_HAL_Driver/Inc",
        "Drivers/CMSIS/Device/ST/STM32F4xx/Include",
        "Drivers/CMSIS/Include",
    ])

    freertos_base = os.path.join(
        FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source"
    )
    add_include_path(freertos_base, [
        "include",
        "portable/GCC/ARM_CM4F",
    ])

    env.Append(CPPPATH=framework_includes)

    freertos_sources = [
        "tasks.c", "queue.c", "list.c", "timers.c",
        "portable/GCC/ARM_CM4F/port.c",
        "portable/MemMang/heap_4.c",
    ]

    freertos_objs = []
    for src in freertos_sources:
        src_path = os.path.join(freertos_base, src)
        if os.path.exists(src_path):
            obj_name = src.replace("/", "_").replace(".c", ".o")
            obj_path = os.path.join("$BUILD_DIR", "FreeRTOS_obj", obj_name)
            obj = env.Object(obj_path, src_path)
            freertos_objs.append(obj)

    env.Append(PIOBUILDFILES=freertos_objs)
