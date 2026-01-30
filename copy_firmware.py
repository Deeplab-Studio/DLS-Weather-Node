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

    # Determine Offsets based on Board Type
    # ESP32: Bootloader @ 0x1000
    # C3/S3/C6: Bootloader @ 0x0
    is_esp32 = "esp32" in board and not any(x in board for x in ["c3", "s3", "c6", "h2"])
    
    bootloader_offset = 0x1000 if is_esp32 else 0x0
    partitions_offset = 0x8000
    app_offset = 0x10000
    boot_app0_offset = 0xe000 # Only for ESP32 usually

    manifest = {
        "name": f"DLS Weather Node - {env_name}",
        "version": "1.0.2",
        "builds": [
            {
                "chipFamily": "ESP32" if is_esp32 else board.upper().replace("DEVKIT", "").replace("-", ""), # Approximate
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

env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware)
