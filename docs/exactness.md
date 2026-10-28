# How exact it is

Checked by `dsp3_check` against the original chip (running in the ares emulator, its program
taken at run time from an installation outside this repository; docs/clean-room.md).

## SD Gundam GX

The game uses commands 02, 03, 06, 07, 0F, 18, 1E, 1F, 2F, 38 and 3E. Three recordings with the
original chip, from power-on to the title, to the menu, and into a battle on the hex map:

| Recording | Reads | The same here |
|---|---|---|
| to the title | 44,292 | 43,603 |
| to the menu | 52,364 | 51,675 |
| into a battle | 97,903 | 97,212 |

Every difference is one of these two:

- **1F (the data ROM, 689 reads in each recording).** The game reads the chip's 1024-word table
  once at boot. It is the chip maker's data, so it isn't reproduced: zeros take its place. The
  game boots and plays with them in all three recordings; nothing it does afterwards differs.
- **1E's costs (2 reads, in the battle).** Phase 2's cost of reaching each cell is the cheapest
  path over the area here (Dijkstra's shortest paths). The original's own search gives a
  different cost for a few cells: 394 of the 396 cells are the same in the game's call,
  and about 94% of the cells on random boards. Its rule isn't worked out yet.

`dsp3_check -s` replays the recordings with the chip's state saved and loaded again after every
transfer: the same results.

## Not exact yet

- 1E's phase 2 costs, as above.
- 07 with directions above 12: here they stay put; the original's answers for them follow no
  rule found yet (the game doesn't use them).
- 1E with a range of 2 hangs the original (it never asks for its next word); here it answers.
- Commands not listed in docs/commands.md are taken as doing nothing.
