"""DLS Weather Node - webflasher icin firmware + manifest uretimi.

PlatformIO post-action olarak calisir: .pio/build/<env>/ altindaki binary'leri
docs/firmware/<env>/ altina kopyalar ve ESP Web Tools manifestlerini yazar.
Surum numarasi tek kaynaktan (include/version.h) okunur.
"""

import json
import os
import re
import shutil

Import("env")

# ESP32 ailesi icin sabit offsetler
OFF_BOOTLOADER_ESP32 = 0x1000
OFF_BOOTLOADER_OTHER = 0x0
OFF_PARTITIONS = 0x8000
OFF_APP = 0x10000
OFF_OTA_DATA = 0xE000

MCU_TO_FAMILY = {
    "esp32": "ESP32",
    "esp32c3": "ESP32-C3",
    "esp32s3": "ESP32-S3",
    "esp32s2": "ESP32-S2",
    "esp32c6": "ESP32-C6",
}


def read_version(project_dir):
    """include/version.h icindeki DLS_FW_VERSION degerini okur."""
    path = os.path.join(project_dir, "include", "version.h")
    try:
        with open(path, encoding="utf-8") as f:
            text = f.read()
    except OSError:
        return "0.0.0"
    m = re.search(r'DLS_FW_VERSION\s+"([^"]+)"', text)
    return m.group(1) if m else "0.0.0"


def partition_csv_path(env):
    """Kullanilan partition tablosunun CSV yolunu bulur (yoksa None)."""
    name = env.BoardConfig().get("build.partitions", "default.csv")
    fw_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    for candidate in (
        os.path.join(fw_dir, "tools", "partitions", name),
        os.path.join(fw_dir, "tools", "partitions", name + ".csv"),
    ):
        if os.path.exists(candidate):
            return candidate
    return None


def has_otadata(env):
    """Partition tablosunda otadata bolumu var mi? (boot_app0 gerekli mi)"""
    path = partition_csv_path(env)
    if not path:
        return False
    with open(path, encoding="utf-8") as f:
        content = f.read()
    return "otadata" in content


def copy_firmware(source, target, env):
    env_name = env["PIOENV"]
    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")
    out_dir = os.path.join(project_dir, "docs", "firmware", env_name)
    os.makedirs(out_dir, exist_ok=True)

    mcu = str(env.BoardConfig().get("build.mcu", "esp32")).lower()
    family = MCU_TO_FAMILY.get(mcu)
    if family is None:
        raise SystemExit(f"[copy_firmware] Bilinmeyen MCU: {mcu} (env: {env_name})")

    bootloader_off = OFF_BOOTLOADER_ESP32 if family == "ESP32" else OFF_BOOTLOADER_OTHER
    parts = []

    def add_part(filename, offset, required=False, fallback_dir=None):
        src = os.path.join(build_dir, filename)
        if not os.path.exists(src) and fallback_dir:
            alt = os.path.join(fallback_dir, filename)
            src = alt if os.path.exists(alt) else src
        if not os.path.exists(src):
            if required:
                raise SystemExit(f"[copy_firmware] Zorunlu dosya yok: {src}")
            print(f"[copy_firmware] atlandi (yok): {filename}")
            return
        shutil.copy(src, os.path.join(out_dir, filename))
        parts.append({"path": filename, "offset": offset})
        print(f"[copy_firmware] {filename} -> {out_dir}")

    fw_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    pkg_parts = os.path.join(fw_dir, "tools", "partitions")

    add_part("bootloader.bin", bootloader_off, required=True)
    add_part("partitions.bin", OFF_PARTITIONS, required=True)
    if has_otadata(env):
        add_part("boot_app0.bin", OFF_OTA_DATA, required=False, fallback_dir=pkg_parts)
    add_part("firmware.bin", OFF_APP, required=True)

    version = read_version(project_dir)

    manifest = {
        "name": f"DLS Weather Node - {env_name}",
        "version": version,
        "builds": [{"chipFamily": family, "parts": parts}],
    }
    manifest_path = os.path.join(out_dir, "manifest.json")
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=4)

    update_manifest = {
        "name": f"DLS Weather Node (Update) - {env_name}",
        "version": version,
        "builds": [
            {
                "chipFamily": family,
                "parts": [{"path": "firmware.bin", "offset": OFF_APP}],
            }
        ],
    }
    update_path = os.path.join(out_dir, "manifest_update.json")
    with open(update_path, "w", encoding="utf-8") as f:
        json.dump(update_manifest, f, indent=4)

    print(f"[copy_firmware] {version} / {family} -> {manifest_path}")


env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware)
