"""Build and check a side copy. Never install into Steam."""
import argparse
from hashlib import sha256
from pathlib import Path
import os
import struct
import subprocess as sp

STOCK = 'fca6628b5a4d76a945e11b9796f327004edc65420d9f9cc23f883143508edd78'
root = Path(__file__).resolve().parent
args = argparse.ArgumentParser(description=__doc__)
args.add_argument('stock', type=Path, help='unmodified Linux build 10211 hw.so')
args.add_argument('--movement-only', action='store_true', help='omit camera interpolation')
a = args.parse_args()
b = bytearray(a.stock.read_bytes())
if sha256(b).hexdigest() != STOCK:
    raise SystemExit('Refusing unsupported or already modified engine. See README for stock SHA-256.')
out = root / 'build' / ('movement' if a.movement_only else 'smooth')
out.mkdir(parents=True, exist_ok=True)
os.chdir(out)
def run(*cmd):
    sp.run(cmd, check=True)

cflags = ['-DMOVEMENT_ONLY'] if a.movement_only else []
asflags = ['--defsym', 'MOVEMENT_ONLY=1'] if a.movement_only else []
run('gcc', '-m32', '-Os', '-ffreestanding', '-fPIE', '-fno-stack-protector', '-fno-asynchronous-unwind-tables', '-mpreferred-stack-boundary=2', *cflags, '-c', root/'move.c', '-o', 'move.o')
run('as', '--32', *asflags, root/'stub.S', '-o', 'stub.o')
rx, state = 0x2ba6f8, 0x13cf830
Path('layout.ld').write_text(f'''SECTIONS {{ . = {rx}; .payload : {{ *(.text.entry) *(.text*) *(.rodata*) *(.got*) }}
 state = {state}; resume = 0x13a4b1; done = 0x13a5a0; original_view = 0x158be0;
 /DISCARD/ : {{ *(.comment) *(.note*) *(.eh_frame*) }} }}''')
run('ld', '-m', 'elf_i386', '-T', 'layout.ld', 'stub.o', 'move.o', '-o', 'payload.elf')
run('objcopy', '-O', 'binary', '-j', '.payload', 'payload.elf', 'payload.bin')
payload = Path('payload.bin').read_bytes()
end = rx + len(payload)
if not payload or end > 0x2bae0c or any(b[rx:end]):
    raise SystemExit('Payload does not fit verified ELF segment gap')
b[rx:end] = payload
site = 0x13a4ab
b[site:site+6] = b'\xe9' + struct.pack('<i', rx-site-5) + b'\x90'
struct.pack_into('<II', b, 52+16, end, end)  # first LOAD: filesz and memsz
struct.pack_into('<I', b, 52+32+20, state+(64 if a.movement_only else 128)-0x2bbe0c)
if not a.movement_only:
    symbols = sp.check_output(['nm', 'payload.elf'], text=True).splitlines()
    camera = int(next(x.split()[0] for x in symbols if x.endswith(' T camera')), 16)
    view = 0x169ca6
    struct.pack_into('<i', b, view+1, camera-view-5)
Path('hw.so').write_bytes(b)
for test in ('test', 'verify'):
    run('gcc', '-m32', '-O2', *cflags, root/(test+'.c'), '-o', test)
    run('./'+test)
print(sha256(b).hexdigest(), out/'hw.so')
