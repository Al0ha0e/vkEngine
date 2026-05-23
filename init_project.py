import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent

DIRECTORIES = [
    "out",
    "src/generated",
]

ASSET_COMMANDS = [
    [
        sys.executable,
        "tools/gen_transmittance_lut.py",
        "builtin_assets/texture/",
        "builtin_assets/config/atmosphere_param.json",
    ],
    [
        sys.executable,
        "tools/gen_brdf_lut.py",
        "builtin_assets/texture/",
        "256",
        "256",
    ],
]


def run(command):
    print(" ".join(command))
    subprocess.run(command, cwd=ROOT, check=True)


def main():
    for directory in DIRECTORIES:
        path = ROOT / directory
        path.mkdir(parents=True, exist_ok=True)
        print(f"created {path.relative_to(ROOT)}")

    for command in ASSET_COMMANDS:
        run(command)

    print("project initialization complete")


if __name__ == "__main__":
    main()
