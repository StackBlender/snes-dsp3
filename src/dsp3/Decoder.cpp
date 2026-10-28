// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) StackBlender LLC
#include "dsp3/Decoder.h"

#include <algorithm>
#include <vector>

namespace dsp3 {

unsigned Decoder::bits(int n) {
	unsigned v = 0;
	while(n--) {
		if(!m_left) m_word = m_need(), m_left = 16;
		v = v << 1 | (m_word >> --m_left & 1);
	}
	return v;
}

// Words: the number of distinct symbols n, the number of symbols to decode, then a bit stream
// (most significant bit first): the n symbols, each by a 2-bit mode (00: a 9-bit value; 01: the
// previous + 1; 10: the previous + 2 + 1 bit; 11: the previous + 4 + 4 bits); values 100h and up
// are copy lengths, given as 8000h + (value - 100h + 2). Then 1 bit (4 or 8 groups) and each
// group's index width less 1, in 3 bits. The symbols fill the groups in order, 2^width each, the
// last group taking the rest; each group sorted. A symbol's code: its group, then its index. A
// copy length is followed by its distance: 0 and 8 bits, or 1 and 12 bits.
void Decoder::run() {
	unsigned n = m_need(), count = m_need();
	std::vector<unsigned> values;
	unsigned previous = 0;
	for(unsigned i = 0; i < n; i++) {
		unsigned v;
		switch(bits(2)) {
		case 0: v = bits(9); break;
		case 1: v = previous + 1; break;
		case 2: v = previous + 2 + bits(1); break;
		default: v = previous + 4 + bits(4); break;
		}
		values.push_back(v);
		previous = v;
	}
	unsigned groups = bits(1) ? 8 : 4, prefix = groups == 8 ? 3 : 2;
	std::vector<unsigned> width(groups);
	for(auto& w : width) w = bits(3) + 1;
	std::vector<std::vector<uint16_t>> table(groups);
	size_t at = 0;
	for(unsigned g = 0; g < groups; g++) {
		size_t size = g + 1 == groups ? values.size() - at : std::min<size_t>(size_t(1) << width[g], values.size() - at);
		for(size_t i = at; i < at + size; i++) {
			unsigned v = values[i];
			table[g].push_back(uint16_t(v >= 0x100 ? 0x8000 | (v - 0x100 + 2) : v));
		}
		std::sort(table[g].begin(), table[g].end());
		at += size;
	}
	for(unsigned i = 0; i < count; i++) {
		unsigned g = bits(prefix), index = bits(int(width[g]));
		uint16_t symbol = index < table[g].size() ? table[g][index] : 0;
		m_give(symbol);
		if(symbol & 0x8000) m_give(uint16_t(bits(1) ? bits(12) : bits(8)));
	}
}

} // namespace dsp3
