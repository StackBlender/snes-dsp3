# The DSP-3's commands, as measured

How each command behaves, worked out by putting questions to the original chip (running in an
emulator; docs/clean-room.md) and recording SD Gundam GX's traffic with it. A command is a byte;
its inputs and outputs are 16-bit words, least significant byte first, unless marked as bytes
(the chip's 8-bit transfers: status bit 2 shows when the next transfer is a byte). A cell is
row << 8 | column; its offset is row * width + column. The board is a hex grid of 06's size.

| Command | Inputs | Outputs |
|---|---|---|
| 02 | scroll x, scroll y; then per sprite: tile, x, y; a tile with bit 15 set ends the command | per sprite: dx = x - scroll x, dy = y - scroll y (16-bit signed); on screen when -64 <= dx < 320 and -64 <= dy < 288. Flag: (on screen ? 0 : 8000h) \| (dy in range ? 1 : 0); dx (less 400h if >= 200h, plus 400h if < -200h); on screen ? dy : 8000h; on screen ? 8000h : 8080h. The tile isn't used |
| 03 | cell | its offset |
| 06 | rows << 8 \| columns | none: sets the board (kept between commands) |
| 07 | direction (a byte), cell | the next cell that way, and its offset. Directions 0-5: up, up-right, down-right, down, down-left, up-left (odd columns sit half a row lower than even ones). 6-11 are 0-5 again; 12 stays put. It wraps at the board's edges |
| 0F | a word | 0000 (a memory test: passed) |
| 18 | a count of rows; per row 8 pixels (bytes, 8 bits each) | per row 4 words, the bitplanes in pairs (0/1, 2/3, 4/5, 6/7), the even plane in the low byte, the leftmost pixel in bit 7 |
| 1E | phase 1: the range (high byte) | each cell of the area around the start cell (3E), by sector (the six directions in turn), then ring 1 to the range, k cells in ring k: its offset, answered with two bytes (a flag, then the cost of entering it, FFh where it can't be entered); then FFFFh |
| | phase 2: the range again | each cell as before: its offset and the cost of reaching it from the start (a byte; 255 where it can't be reached); then FFFFh |
| 1F | a word | 1024 words: the chip's data ROM. Not reproduced (docs/exactness.md) |
| 2F | a word | 0300h, the program's version |
| 38 | the number of distinct symbols n, the number of symbols to decode, then a coded bit stream (below) | the symbols, each given as soon as the words so far complete its code; the chip asks for a word (status bit 6) when it runs out of bits |
| 3E | cell | its offset; it becomes 1E's start cell (kept between commands) |

Commands not in the table do nothing here.

## Command 38's stream

The bits run from the most significant bit of each word. First the n symbols, each by a 2-bit
mode: 00, a 9-bit value; 01, the previous + 1; 10, the previous + 2 + 1 bit; 11, the previous +
4 + 4 bits. A value of 100h or more is a copy length, given as 8000h + (value - 100h + 2). Then 1
bit (0: 4 groups, 1: 8) and each group's index width less 1, in 3 bits. The symbols fill the
groups in order, 2^width each, the last group taking the rest; each group is sorted. A symbol's
code is its group number (2 or 3 bits) and its index in the group. A copy length is followed by
its distance: a 0 bit and 8 bits, or a 1 bit and 12 bits (given as one more output, after the
length).

## Status

Bit 7 is always set (ready). Bit 2: the next transfer is a byte (a command byte, or a byte input
or output). Bit 6: command 38 (after its two counts) or 1E (after its range) is waiting for a word.

As on the chip, a write where an output is due takes the output's place (the written word is
lost); SD Gundam GX skips 02's fourth output that way.
