#!/usr/bin/env python3
"""Verify the linked, fixed-cadence I2C sampler (objdump -d -z -C).

This checks instruction timing, not START latency or electrical qualification.
"""
from pathlib import Path
import re
import sys

text = Path(sys.argv[1]).read_text()
start = re.search(r'^([0-9a-f]+) <fast_i2c_sample_loop>:$', text, re.M)
end = re.search(r'^([0-9a-f]+) <fast_i2c_sample_end>:$', text, re.M)
if not start or not end or end.start() <= start.end():
    raise SystemExit('FAIL: missing fast sampler boundaries')
instructions = []
for line in text[start.end():end.start()].splitlines():
    m = re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s*(\w+)\s*(.*)', line)
    if m:
        addr, raw, op, args = m.groups()
        instructions.append((int(addr, 16), len(raw.split()), op, args))
expected = ['in', 'andi'] + ['nop'] * 14 + ['in', 'swap', 'andi', 'or', 'st', 'sbiw'] + ['nop'] * 6 + ['brne']
if [i[2] for i in instructions] != expected:
    raise SystemExit('FAIL: changed sampling instructions; use objdump -z and inspect the loop')
for i, (addr, size, _, _) in enumerate(instructions):
    if size != 2 or addr != int(start[1], 16) + i * 2:
        raise SystemExit('FAIL: unexpected instruction size or gap')
if instructions[-1][0] + 2 != int(end[1], 16):
    raise SystemExit('FAIL: unexpected trailing code')
for i in (0, 16):
    if not re.match(r'r\d+,\s*0x06\b', instructions[i][3]):
        raise SystemExit('FAIL: sampler must read PINC')
target = re.search(r';\s*0x([0-9a-f]+)', instructions[-1][3])
if not target or int(target[1], 16) != int(start[1], 16):
    raise SystemExit('FAIL: loop branch changed')
cycles = lambda part: sum(2 if op in ('st', 'sbiw', 'brne') else 1 for _, _, op, _ in part)
first, second = cycles(instructions[:16]), cycles(instructions[16:])
if (first, second) != (16, 16):
    raise SystemExit(f'FAIL: sample spacing is {first}/{second} cycles')
print('PASS fast I2C: both sample intervals are 16 CPU cycles (1 us at 16 MHz); bench qualification pending')
