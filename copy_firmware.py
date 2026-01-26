import shutil
import os

Import("env")

def copy_firmware(source, target, env):
    print("Copying firmware files to docs/ folder for Webflasher...")
    
    # Define paths
    build_dir = env.subst("$BUILD_DIR")
    docs_dir = os.path.join(env.subst("$PROJECT_DIR"), "docs")
    
    # Ensure docs directory exists
    if not os.path.exists(docs_dir):
        os.makedirs(docs_dir)

    # File mappings (Source in .pio -> Target in docs)
    files = {
        "firmware.bin": "firmware.bin",
        "partitions.bin": "partitions.bin",
        "bootloader.bin": "bootloader.bin",
        "boot_app0.bin": "boot_app0.bin" # This might need to be found in packages
    }

    # Helper to copy
    def my_copy(src, dst):
        if os.path.exists(src):
            shutil.copy(src, dst)
            print(f"Copied {src} -> {dst}")
        else:
            print(f"Warning: Source file {src} not found!")

    # Copy firmware and partitions
    my_copy(os.path.join(build_dir, "firmware.bin"), os.path.join(docs_dir, "firmware.bin"))
    my_copy(os.path.join(build_dir, "partitions.bin"), os.path.join(docs_dir, "partitions.bin"))
    my_copy(os.path.join(build_dir, "bootloader.bin"), os.path.join(docs_dir, "bootloader.bin"))
    
    # boot_app0.bin is tricky, it's usually in the arduino framework package.
    # Often for simple purposes, users might overlook it, but it's needed for OTA layout.
    # PlatformIO puts it in ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin
    # But finding it dynamically is better or we just skip it if it's not generated in build dir.
    # Standard Arduino/ESP32 build might not copy it to build dir. 
    # Let's try to locate it via partition table info or just assume standard path if possible.
    # For now, let's verify if 'boot_app0.bin' is in the build dir (sometimes it is).
    # If not, we might need a more complex lookup.
    
    # Try finding it in the build directory first (some versions copy it)
    if os.path.exists(os.path.join(build_dir, "boot_app0.bin")):
        my_copy(os.path.join(build_dir, "boot_app0.bin"), os.path.join(docs_dir, "boot_app0.bin"))
    else:
        # Fallback: Try to find in packages
        # This is a bit hacky but works for many standard setups
        try:
            platform_packages = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
            boot_app0_path = os.path.join(platform_packages, "tools", "partitions", "boot_app0.bin")
            if os.path.exists(boot_app0_path):
                 my_copy(boot_app0_path, os.path.join(docs_dir, "boot_app0.bin"))
        except:
            print("Could not locate boot_app0.bin in packages.")

env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware)
