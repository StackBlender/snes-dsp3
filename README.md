# snes-dsp3

A free, clean-room implementation of the DSP-3, the NEC coprocessor in SD Gundam GX, under the
GPL. It will answer the chip's commands with the same results as the original, so emulators can
run the game without the original chip's program, which is the console maker's code.

**Status:** SD Gundam GX plays with it. In three recordings of the game with the original chip,
every read is the same except the chip's data ROM table (not reproduced: zeros take its place,
and the game doesn't mind) and 2 path costs; see [docs/exactness.md](docs/exactness.md). The
commands: [docs/commands.md](docs/commands.md).

## Using it

`src/dsp3` is plain C++17 with no dependencies. `dsp3::Dsp3` has the chip's two registers
(`readData`, `writeData`, `readStatus`) and `saveState` / `loadState`. Map the data register and
the status register where the cartridge has them (as for the DSP-1 in the same game slot).
`tests/state_test.cpp` runs with `ctest`; `dsp3_check` compares recordings of the original's
traffic (kept outside the repository).

Made by [StackBlender](https://stackblender.com) for Super Apollo, an emulator that plays
SNES games fast with their music at normal speed. Its other chips are published the same
way: [snes-dsp1](https://github.com/StackBlender/snes-dsp1),
[snes-dsp2](https://github.com/StackBlender/snes-dsp2),
[snes-st010](https://github.com/StackBlender/snes-st010),
[snes-cx4](https://github.com/StackBlender/snes-cx4),
[snes-dsp4](https://github.com/StackBlender/snes-dsp4).

## How it is made

Clean room: written from the public list of the chip's commands (their names) and from measuring
what the original answers, never from the original program's bytes or a disassembly of it,
which nobody on this project reads. It is checked against the original running in the
[ares](https://github.com/ares-emulator/ares) emulator, answer for answer; the tests take the
original from the emulator at run time, and it is never in this repository. The rules:
[docs/clean-room.md](docs/clean-room.md).

## License

GNU General Public License, version 3 or (at your option) any later version; see
[LICENSE](LICENSE). For other terms, contact [StackBlender](https://stackblender.com).
