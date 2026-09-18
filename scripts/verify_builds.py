#!/usr/bin/env python3
"""Compile all Uno sketches/configurations and check SRAM and capture CPU budgets."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--arduino-cli', default=shutil.which('arduino-cli'))
parser.add_argument('--libraries', help='Additional Arduino library directory (optional)')
parser.add_argument('--avr-objdump', default=shutil.which('avr-objdump'))
args = parser.parse_args()
if not args.arduino_cli:
    mac_cli = Path('/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli')
    if mac_cli.exists():
        args.arduino_cli = str(mac_cli)
if not args.arduino_cli:
    parser.error('Pass --arduino-cli or install arduino-cli on PATH')
if not args.avr_objdump:
    for data in [Path.home() / 'Library/Arduino15', Path.home() / '.arduino15']:
        candidates = sorted(data.glob('packages/arduino/tools/avr-gcc/*/bin/avr-objdump'))
        if candidates:
            args.avr_objdump = str(candidates[-1])
if not args.avr_objdump:
    parser.error('Pass --avr-objdump (from the Arduino AVR toolchain)')

builds = [
    ('i2c-32', 'I2CAnalyzer', ''),
    ('logic-64', 'logic-analyzer', ''),
    ('peripheral', 'ExternalGPIOPeripheral/peripheral', ''),
    ('host-10k', 'ExternalGPIOPeripheral/host', ''),
    ('i2c-64', 'I2CAnalyzer', '-DOLED_HEIGHT=64'),
    ('logic-32', 'logic-analyzer', '-DOLED_HEIGHT=32'),
    ('host-100k', 'ExternalGPIOPeripheral/host', '-DHOST_I2C_CLOCK_HZ=100000UL'),
]
with tempfile.TemporaryDirectory(prefix='embedded-builds-') as tmp:
    for name, sketch, flags in builds:
        build = Path(tmp) / name
        cmd = [args.arduino_cli, 'compile', '--fqbn', 'arduino:avr:uno', '--warnings', 'all',
               '--build-path', str(build)]
        if args.libraries:
            cmd += ['--libraries', args.libraries]
        if flags:
            cmd += ['--build-property', 'compiler.cpp.extra_flags=' + flags]
        print('Building ' + name, flush=True)
        result = subprocess.run(cmd + [sketch], cwd=ROOT, capture_output=True, text=True)
        if result.stderr:
            print(result.stderr, end='', file=sys.stderr)
        print(result.stdout, end='', flush=True)
        result.check_returncode()
        ram = re.search(r'Global variables use (\d+) bytes', result.stdout)
        if not ram or int(ram[1]) > 1536:
            raise SystemExit('FAIL: cannot establish at least 512 bytes remaining SRAM')
        if sketch == 'logic-analyzer':
            elf = build / 'logic-analyzer.ino.elf'
            disassembly = build / 'capture.disasm'
            with disassembly.open('w') as out:
                subprocess.run([args.avr_objdump, '-d', '-C', str(elf)], stdout=out, check=True)
            subprocess.run([sys.executable, str(ROOT / 'scripts/check_timing.py'), str(disassembly)], check=True)
print('PASS all builds, SRAM budgets, and logic capture CPU budgets')
