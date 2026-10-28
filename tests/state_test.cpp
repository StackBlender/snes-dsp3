// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) StackBlender LLC
// Saving and loading the chip's state mid-command changes nothing; and the commands' known
// answers.
#include "dsp3/Dsp3.h"

#include <cstdio>
#include <vector>

static int failures = 0;
static void expect(bool ok, const char* what) {
	if(!ok) std::printf("FAIL %s\n", what), failures++;
}

static void command(dsp3::Dsp3& chip, uint8_t c) { chip.writeData(c); }
static void put(dsp3::Dsp3& chip, uint16_t w) { chip.writeData(uint8_t(w)); chip.writeData(uint8_t(w >> 8)); }
static uint16_t get(dsp3::Dsp3& chip) { uint16_t lo = chip.readData(); return uint16_t(lo | chip.readData() << 8); }

int main() {
	using dsp3::Dsp3;
	{
		Dsp3 chip;
		command(chip, 0x2f); put(chip, 0);
		expect(get(chip) == 0x0300, "2F: version");
		command(chip, 0x06); put(chip, 0x0808);  // an 8x8 board
		command(chip, 0x03); put(chip, 0x0203);
		expect(get(chip) == 2 * 8 + 3, "03: a cell's offset");
		command(chip, 0x07); chip.writeData(3); put(chip, 0x0202);  // direction 3 (a byte), even column: one row down
		expect(get(chip) == 0x0302 && get(chip) == 3 * 8 + 2, "07: the adjacent cell");
		command(chip, 0x02); put(chip, 100); put(chip, 50);
		put(chip, 1); put(chip, 110); put(chip, 60);
		expect(get(chip) == 0x0001 && get(chip) == 10 && get(chip) == 10 && get(chip) == 0x8000, "02: a sprite on screen");
		put(chip, 0x8000);
		// 38: symbols 5 and 6 (00 + 9 bits, then 01), 4 groups of width 1, then the codes 000 001 000
		command(chip, 0x38); put(chip, 2); put(chip, 3);
		expect(chip.readStatus() & 0x40, "38: signals for a word");
		put(chip, 0x00a8); put(chip, 0x0001);  // the symbols are given as their codes are complete
		expect(get(chip) == 5 && get(chip) == 6, "38: decodes");
		put(chip, 0x0000);
		expect(get(chip) == 5, "38: decodes the last");
		command(chip, 0x2f); put(chip, 0);
		expect(get(chip) == 0x0300, "in step after 38");
	}
	{
		// The same stream with a save and load after every transfer gives the same answers.
		std::vector<std::pair<int, uint16_t>> stream = {  // 0 command, 1 word in, 2 word out, 3 byte in
			{0, 0x06}, {1, 0x0a0c}, {0, 0x3e}, {1, 0x0304}, {2, 0},
			{0, 0x07}, {3, 1}, {1, 0x0304}, {2, 0}, {2, 0},
			{0, 0x38}, {1, 2}, {1, 3}, {1, 0x00a8}, {1, 0x0001}, {2, 0}, {2, 0}, {1, 0}, {2, 0},
			{0, 0x02}, {1, 7}, {1, 9}, {1, 1}, {1, 400}, {1, 20}, {2, 0}, {2, 0}, {2, 0}, {2, 0}, {1, 0xffff},
			{0, 0x2f}, {1, 0}, {2, 0},
		};
		Dsp3 straight, saved;
		std::vector<uint16_t> a, b;
		for(auto [kind, word] : stream) {
			if(kind == 0 || kind == 3) command(straight, uint8_t(word)), command(saved, uint8_t(word));
			if(kind == 1) put(straight, word), put(saved, word);
			if(kind == 2) a.push_back(get(straight)), b.push_back(get(saved));
			auto state = saved.saveState();
			Dsp3 fresh;
			expect(fresh.loadState(state.data(), state.size()), "load a saved state");
			saved = fresh;
		}
		expect(a == b, "saving and loading each transfer changes nothing");
		expect(b.back() == 0x0300, "in step at the end");
		Dsp3 chip;
		uint8_t junk[3] = {1, 2, 3};
		expect(!chip.loadState(junk, 3), "a blob that isn't a state is refused");
	}
	std::printf("%s\n", failures ? "dsp3: FAILED" : "dsp3: ok");
	return failures ? 1 : 0;
}
