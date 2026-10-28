// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) StackBlender LLC
// Replays recorded traffic with the original chip through Dsp3 and counts the reads that differ.
// A recording has one line per transfer: "w XX" (a byte written: a command, or data in the
// chip's 8-bit mode), "W XXXX" (a word written), "r XX" (a byte read), "R XXXX" (a word read);
// other lines are ignored. Recordings come from the original
// chip (docs/clean-room.md) and stay out of this repository.
// With -s, the chip's state is saved and loaded again after every transfer.
//   dsp3_check [-s] RECORDING...
#include "dsp3/Dsp3.h"

#include <cstdio>
#include <fstream>
#include <string>

int main(int argc, char** argv) {
	bool states = argc > 1 && std::string(argv[1]) == "-s";
	if(argc < 2 + states) return std::fprintf(stderr, "usage: dsp3_check [-s] RECORDING...\n"), 2;
	int failed = 0;
	for(int f = 1 + states; f < argc; f++) {
		std::ifstream in(argv[f]);
		dsp3::Dsp3 chip;
		std::string kind, value;
		long reads = 0, same = 0, line = 0, firstDiff = -1;
		while(in >> kind) {
			if(kind != "w" && kind != "W" && kind != "R" && kind != "r") { std::getline(in, value); continue; }
			in >> value;
			line++;
			if(states) {
				auto state = chip.saveState();
				dsp3::Dsp3 fresh;
				if(!fresh.loadState(state.data(), state.size())) return std::fprintf(stderr, "a state didn't load\n"), 1;
				chip = fresh;
			}
			unsigned v = unsigned(std::stoul(value, nullptr, 16));
			if(kind == "w") { chip.writeData(uint8_t(v)); continue; }
			if(kind == "W") { chip.writeData(uint8_t(v)); chip.writeData(uint8_t(v >> 8)); continue; }
			reads++;
			unsigned got = chip.readData();
			if(kind == "R") got |= unsigned(chip.readData()) << 8;
			if(got == v) same++;
			else if(firstDiff < 0) firstDiff = line;
		}
		std::printf("%s: %ld of %ld reads the same", argv[f], same, reads);
		if(firstDiff >= 0) std::printf(" (first difference at transfer %ld)", firstDiff), failed++;
		std::printf("\n");
	}
	return failed ? 1 : 0;
}
