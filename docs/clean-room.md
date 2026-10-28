# Clean room

How this implementation is kept free of anyone else's code and of the original chip's
program, and how it stays that way. Every change follows these rules.

## What it may be written from

- Public descriptions of the chip's behaviour: what its registers do and what each command
  takes and gives back (for example the SNESdev wiki and the nocash "fullsnes"
  documentation), read as descriptions, never as code to translate.
- Mathematics: trigonometry, projection and scaling are worked out from first principles.
- The original's answers, observed from outside: questions put to the chip (running in an
  emulator) and its replies compared with ours, to find where we differ.
- [ares](https://github.com/ares-emulator/ares)' emulation of the chip's processor (the NEC
  uPD77C25, ISC), only to run the original for those comparisons. It runs the program; it
  doesn't describe the commands.

## What it is never written from

- **The original program**, the console maker's code: its bytes, a disassembly, a
  decompilation, or notes made from one. Nobody on this project reads them. Files are
  identified by name and size only.
- **The chip's data ROM** (its tables of constants): whatever our code needs is computed
  from mathematics or worked out from the chip's behaviour, never copied, never filled in
  from the original's bytes. A formula's few parameters may be chosen by comparing with the
  original, as long as the comparison reports only how many answers match (owner,
  2026-10-28).
- **Other emulators' DSP-3 code**, whatever its licence: bsnes/higan, Snes9x, ZSNES, Mesen,
  MAME, and any code derived from them. Not read, not consulted.

## How the checks stay clean

- The tests take the original program from an emulator installation outside this
  repository at run time. It is never committed, copied into the build, embedded or
  printed, and logs show only our results, the original's results and whether they match.
- Recorded game traffic (the questions games ask and the original's answers) is kept on the
  machine that recorded it, git-ignored, never committed. Games are never committed.
- Before each push: no file in the repository or its history holds firmware or game data
  (`git log --stat` and the size and type of every added binary file are checked; binary
  files aren't expected at all).

## Licence

Our own work, copyright StackBlender LLC, published under the GPL (version 3 or later).
Outside contributions are only taken from authors who agree that StackBlender may also
license their work under other terms, and who confirm they followed these rules.
