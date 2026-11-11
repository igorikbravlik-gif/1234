# Собирает esp32fw.bin: bootloader + partitions + boot_app0 + firmware (адрес 0x0, для M5Burner / esptool)
Import("env")
import os, subprocess

def merge(source, target, env):
    build = env.subst("$BUILD_DIR")
    proj = env.subst("$PROJECT_DIR")
    out = os.path.join(proj, "esp32fw.bin")
    pio = env.PioPlatform()
    fw_dir = pio.get_package_dir("framework-arduinoespressif32")
    esptool = os.path.join(pio.get_package_dir("tool-esptoolpy"), "esptool.py")
    parts = [
        ("0x1000", os.path.join(build, "bootloader.bin")),
        ("0x8000", os.path.join(build, "partitions.bin")),
        ("0xe000", os.path.join(fw_dir, "tools", "partitions", "boot_app0.bin")),
        ("0x10000", os.path.join(build, "firmware.bin")),
    ]
    for _, p in parts:
        if not os.path.isfile(p):
            print("merge_bin: missing", p)
            return
    cmd = [env.subst("$PYTHONEXE"), esptool, "--chip", "esp32", "merge_bin", "-o", out,
           "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "8MB"]
    for addr, p in parts:
        cmd += [addr, p]
    subprocess.check_call(cmd)
    print("merge_bin: created", out)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge)
