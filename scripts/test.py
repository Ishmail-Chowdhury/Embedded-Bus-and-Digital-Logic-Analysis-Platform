#!/usr/bin/env python3
"""Run native behavioral regressions; these do not replace physical timing tests."""
import os
import shlex
import sys
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
extra_flags = shlex.split(os.environ.get('CXXFLAGS', ''))
# Some macOS CLT installations omit the compiler-relative C++ headers.
# Use the selected SDK's headers locally; do not change xcode-select or system settings.
if sys.platform == 'darwin' and not extra_flags and 'CXX' not in os.environ:
    sdk = subprocess.check_output(['xcrun', '--show-sdk-path'], text=True).strip()
    headers = Path(sdk) / 'usr/include/c++/v1'
    if headers.is_dir():
        extra_flags += ['-isystem', str(headers)]
P = 'ExternalGPIOPeripheral/peripheral/'
SUITES = {
    'i2c': ('I2CAnalyzer', ['I2CAnalyzer/' + f for f in ['capture.cpp', 'EdgeDetector.cpp', 'BitDecoder.cpp', 'PacketDecoder.cpp', 'RingBuffer.cpp']]),
    'logic': ('logic-analyzer', ['logic-analyzer/capture_buffer.cpp', 'logic-analyzer/trigger.cpp', 'logic-analyzer/measurements.cpp']),
    'peripheral': (P, [P + f for f in ['gpio_controller.cpp', 'registers.cpp', 'interrupt_controller.cpp']]),
    'host': ('ExternalGPIOPeripheral/host', []),
    'expander_bus': (P, [P + 'expander_bus.cpp']),
}
with tempfile.TemporaryDirectory(prefix='embedded-tests-') as tmp:
    for name, (include, sources) in SUITES.items():
        exe = str(Path(tmp) / name)
        cmd = [os.environ.get('CXX', 'c++'), *extra_flags, '-std=c++11', '-Wall', '-Wextra', '-Werror',
               '-fsanitize=' + os.environ.get('SANITIZERS', 'undefined'), '-fno-omit-frame-pointer', '-g',
               '-Itests/stubs', '-I' + include, 'tests/stubs/Arduino.cpp',
               'tests/test_' + name + '.cpp', *sources, '-o', exe]
        subprocess.run(cmd, cwd=ROOT, check=True)
        subprocess.run([exe], cwd=ROOT, check=True)
