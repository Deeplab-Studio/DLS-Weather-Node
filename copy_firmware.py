import shutil
import os
import json

Import("env")

def copy_firmware(source, target, env):
    print("Copying firmware files to docs/ folder for Webflasher...")
    
    # Get environment info
    env_name = env["PIOENV"]
    board = env["BOARD"]
    
    # Define paths
    build_dir = env.subst("$BUILD_DIR")
    # Structure: docs/firmware/<env_name>/
    firmware_dir = os.path.join(env.subst("$PROJECT_DIR"), "docs", "firmware", env_name)
    
    # Ensure firmware directory exists
    if not os.path.exists(firmware_dir):
        os.makedirs(firmware_dir)

    # Determine Chip Family for Manifest (Standard names required by ESP Web Tools)
    # Valid values: ESP32, ESP32-C3, ESP32-S3, ESP32-S2, ESP32-C6
    chip_family = "ESP32" # Default
    if "c3" in board or "c3" in env_name:
        chip_family = "ESP32-C3"
    elif "s3" in board or "s3" in env_name:
        chip_family = "ESP32-S3"
    elif "c6" in board or "c6" in env_name:
        chip_family = "ESP32-C6"
    
    # Offsets
    bootloader_offset = 0x1000 if chip_family == "ESP32" else 0x0
    partitions_offset = 0x8000
    app_offset = 0x10000
    boot_app0_offset = 0xe000 # Only for ESP32 usually
    is_esp32 = (chip_family == "ESP32")

    manifest = {
        "name": f"DLS Weather Node - {env_name}",
        "version": "1.0.2",
        "builds": [
            {
                "chipFamily": chip_family,
                "parts": []
            }
        ]
    }

    # Helper to copy and add to manifest
    def copy_opt(filename, offset):
        src = os.path.join(build_dir, filename)
        dst = os.path.join(firmware_dir, filename)
        if os.path.exists(src):
            shutil.copy(src, dst)
            manifest["builds"][0]["parts"].append({
                "path": filename,
                "offset": offset
            })
            print(f"Copied {filename} -> {dst}")
        else:
            # Fallback for boot_app0
            if filename == "boot_app0.bin":
                try:
                    platform_packages = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
                    src_pkg = os.path.join(platform_packages, "tools", "partitions", "boot_app0.bin")
                    if os.path.exists(src_pkg):
                         shutil.copy(src_pkg, dst)
                         manifest["builds"][0]["parts"].append({
                            "path": filename,
                            "offset": offset
                        })
                         print(f"Copied {filename} (from pkg) -> {dst}")
                except:
                    pass

    # Copy files
    copy_opt("bootloader.bin", bootloader_offset)
    copy_opt("partitions.bin", partitions_offset)
    if is_esp32:
        copy_opt("boot_app0.bin", boot_app0_offset)
    copy_opt("firmware.bin", app_offset)

    # Write Manifest
    manifest_path = os.path.join(firmware_dir, "manifest.json")
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=4)
    print(f"Generated manifest at {manifest_path}")

    # --- Generate UPDATE Manifest (Minimal - Only Firmware) ---
    manifest_update = {
        "name": f"DLS Weather Node (Update) - {env_name}",
        "version": manifest["version"],
        "builds": [
            {
                "chipFamily": manifest["builds"][0]["chipFamily"],
                "parts": [
                    {
                        "path": "firmware.bin",
                        "offset": app_offset
                    }
                ]
            }
        ]
    }
    update_path = os.path.join(firmware_dir, "manifest_update.json")
    with open(update_path, "w") as f:
        json.dump(manifest_update, f, indent=4)
    print(f"Generated update manifest at {update_path}")

env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware)
