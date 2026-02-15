#!/usr/bin/env python3
"""
Install a Python wheel and generate type stubs for IDE support.

Usage:
    python install_panda3d_pylib.py path/to/wheel.whl
"""

import sys
import subprocess
import site
from pathlib import Path


def main():
    if len(sys.argv) != 2:
        print("Usage: python install_panda3d_pylib.py path/to/wheel.whl")
        sys.exit(1)

    wheel_path = Path(sys.argv[1])

    if not wheel_path.exists():
        print(f"Error: Wheel file not found: {wheel_path}")
        sys.exit(1)

    if not wheel_path.suffix == ".whl":
        print(f"Error: File must be a .whl file: {wheel_path}")
        sys.exit(1)

    # Determine package name from wheel filename
    # Format: packagename-version-python-abi-platform.whl
    package_name = wheel_path.stem.split("-")[0]

    print(f"📦 Installing wheel: {wheel_path.name}")
    print(f"   Package: {package_name}")

    # Install/reinstall the wheel
    try:
        subprocess.run(
            [sys.executable, "-m", "pip", "install", "--force-reinstall", str(wheel_path)],
            check=True
        )
        print("✅ Wheel installed successfully")
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to install wheel: {e}")
        sys.exit(1)

    # Install mypy if not already available (for stubgen)
    print("\n🔧 Ensuring mypy is installed (for stubgen)...")
    try:
        subprocess.run(
            [sys.executable, "-m", "pip", "install", "mypy"],
            check=True,
            capture_output=True
        )
    except subprocess.CalledProcessError:
        print("⚠️  Warning: Could not install mypy")

    # Get site-packages directory
    site_packages = Path(site.getsitepackages()[0])
    print(f"\n📝 Generating type stubs in {site_packages}...")

    # For panda3d, generate stubs for main modules
    modules_to_stub = []

    if package_name == "panda3d":
        modules_to_stub = [
            "panda3d.core",
            "panda3d.direct",
            "panda3d.physics",
            "panda3d.fx",
            "panda3d.egg",
        ]
    else:
        # For other packages, try the package name
        modules_to_stub = [package_name]

    for module in modules_to_stub:
        try:
            print(f"   Generating stubs for {module}...")
            result = subprocess.run(
                ["stubgen", "--include-docstrings", "--inspect-mode", "-p", module, "-o", str(site_packages)],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                print(f"   ✅ {module}")
            else:
                print(f"   ⚠️  {module} failed:")
                if result.stderr:
                    print(f"      {result.stderr.strip()}")
                if result.stdout:
                    print(f"      {result.stdout.strip()}")
        except Exception as e:
            print(f"   ⚠️  {module} - {e}")

    print("\n🎉 Done! Restart VSCode or reload the Python extension to see documentation.")
    print("   Tip: Cmd+Shift+P → 'Developer: Reload Window'")


if __name__ == "__main__":
    main()
