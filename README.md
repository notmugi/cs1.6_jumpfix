# CS 1.6 high-FPS landing fix (native Linux)

An experimental client-side patch for **GoldSrc build 10211, Linux i386**. Keep rendering at your chosen FPS while combining movement input into roughly 100 commands/second. The default build also interpolates local camera **position**, not mouse angles. This is an engine patch, not a csldr plugin or a ReGameDLL install. No Valve binaries are distributed.

## Build and check

Requirements: x86 Linux with 32-bit executable support, Python 3, GCC with `-m32` and 32-bit libc development files, and GNU binutils (`as`, `ld`, `nm`, `objcopy`). On Debian/Ubuntu the relevant packages are `python3 gcc-multilib libc6-dev-i386 binutils`.

Supply your own **stock** `hw.so` from `steamapps/common/Half-Life`. The required SHA-256 is `fca6628b5a4d76a945e11b9796f327004edc65420d9f9cc23f883143508edd78`. An already patched library won't work as input; use the original backup. Don't relax the hash check to support another engine build.

```sh
python3 build.py '/path/to/stock/hw.so'                  # build/smooth/hw.so
python3 build.py --movement-only '/path/to/stock/hw.so'  # build/movement/hw.so
```

Both commands run timing tests and a 32-bit harness against the generated machine code. They write only build products under `build/`; neither starts the game nor installs the result. `ld` may warn about the intermediate `payload.elf`; it isn't installed. The final engine retains separate RX and RW load segments.

## Try it and roll back

**Quit the game first.** Back up the verified stock file outside the game directory. For a first install from stock, run from the repo root and substitute your Steam library path:

```sh
(
  set -eu
  game='/path/to/steamapps/common/Half-Life'
  backup="$HOME/hw.so.stock-10211"
  printf '%s  %s\n' fca6628b5a4d76a945e11b9796f327004edc65420d9f9cc23f883143508edd78 "$game/hw.so" | sha256sum -c -
  cp -pn -- "$game/hw.so" "$backup"
  cmp -- "$backup" "$game/hw.so"
  install -m 755 build/smooth/hw.so "$game/hw.so.jumpfix-new"
  mv -T -- "$game/hw.so.jumpfix-new" "$game/hw.so"
)
```

Use `build/movement/hw.so` instead for no camera interpolation. Test on a **local map** first; compatibility with secured servers and anti-cheat has not been verified. To restore, quit the game, then copy the verified backup back to `Half-Life/hw.so`, or use Steam's Verify Integrity to restore the current official files. A Steam update may replace the patch.

See [RESEARCH.md](RESEARCH.md) for the evidence, build-specific assumptions and what to test next.
