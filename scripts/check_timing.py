#!/usr/bin/env python3
"""Conservative AVR instruction-path budget for the linked Timer1 capture loop.

Usage: avr-objdump -d -C firmware.elf > firmware.disasm
       python3 scripts/check_timing.py firmware.disasm
This checks CPU budget, not electrical timing, oscillator accuracy or input bandwidth.
"""
import re
import sys
from functools import lru_cache
from pathlib import Path

instructions = {}
for line in Path(sys.argv[1]).read_text().splitlines():
    match = re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s*(\w+)\s*(.*)', line)
    if match:
        address, raw, op, args = match.groups()
        instructions[int(address, 16)] = (len(raw.split()), op, args)
polls = [addr for addr, (_, op, args) in instructions.items()
         if op == 'sbis' and re.match(r'0x16,\s*1\b', args)]
if len(polls) != 1:
    raise SystemExit('Expected exactly one Timer1 OCF1A polling loop; inspect the generated code')
poll = polls[0]
start = poll + 4  # SBIS skips a two-byte RJMP, then clears OCF1A.
assert instructions[start][1] == 'out' and instructions[start][2].startswith('0x16,')
ONE = set('ldi mov movw in out and andi or ori eor cp cpc cpi subi sbci sub sbc add adc inc dec lsr asr ror rol swap com neg clr clc sec cli sei nop'.split())
TWO = set('lds sts ld st ldd std mul muls mulsu adiw sbiw push pop sbi cbi'.split())
visiting = set()

@lru_cache(None)
def longest(addr):
    if addr == poll:
        return 0
    if addr in visiting:
        raise ValueError('Unexpected cycle inside sampling work; inspect generated code')
    size, op, args = instructions[addr]
    # Any path that stops Timer1 is timeout, overrun or completion, not a next-sample path.
    if op == 'sts' and args.startswith('0x0081,'):
        return None
    visiting.add(addr)
    nxt = addr + size
    paths = []
    if op.startswith('br') or op in ('rjmp', 'jmp'):
        target = int(re.search(r';\s*0x([0-9a-f]+)', args)[1], 16)
        paths.append((target, 3 if op == 'jmp' else 2))
        if op.startswith('br'):
            paths.append((nxt, 1))
    elif op in ('sbic', 'sbis', 'sbrc', 'sbrs', 'cpse'):
        skipped_size = instructions[nxt][0]
        paths = [(nxt, 1), (nxt + skipped_size, 1 + skipped_size // 2)]
    elif op in ONE or op in TWO:
        paths = [(nxt, 1 if op in ONE else 2)]
    else:
        raise ValueError(f'Unsupported instruction in sampling path: {addr:x} {op} {args}')
    totals = []
    for dest, cost in paths:
        rest = longest(dest)
        if rest is not None:
            totals.append(cost + rest)
    visiting.remove(addr)
    return max(totals) if totals else None

work = longest(start)
# Two cycles to skip the wait RJMP on readiness, plus up to three cycles poll phase.
worst = work + 2 + 3
print(f'Conservative sampling path: {worst}/160 CPU cycles ({worst / 16:.4f} us at 16 MHz)')
if worst >= 160:
    raise SystemExit('FAIL: sampling path does not fit the 10 us period')
print('PASS: CPU path fits; physical timing still requires bench verification')
