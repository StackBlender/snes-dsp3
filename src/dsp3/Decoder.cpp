// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) StackBlender LLC
#include "dsp3/Decoder.h"

#include <algorithm>

namespace dsp3 {

unsigned Decoder::bits(int n) {
	if(m_bit + size_t(n) > m_words.size() * 16) throw Short{};
	unsigned v = 0;
	while(n--) {
		v = v << 1 | (m_words[m_bit / 16] >> (15 - m_bit % 16) & 1);
		m_bit++;
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
bool Decoder::feed(uint16_t word, std::vector<uint16_t>& out) {
	if(m_phase == Phase::Count) { m_n = word; m_phase = Phase::Total; return true; }
	if(m_phase == Phase::Total) { m_total = word; m_phase = m_total ? Phase::Symbols : Phase::Done; return !done(); }
	m_words.push_back(word);
	while(m_phase != Phase::Done) {
		size_t mark = m_bit;
		try {
			switch(m_phase) {
			case Phase::Symbols: {
				unsigned v;
				switch(bits(2)) {
				case 0: v = bits(9); break;
				case 1: v = m_previous + 1; break;
				case 2: v = m_previous + 2 + bits(1); break;
				default: v = m_previous + 4 + bits(4); break;
				}
				m_values.push_back(v);
				m_previous = v;
				if(m_values.size() >= m_n) m_phase = Phase::Groups;
				break;
			}
			case Phase::Groups: {
				unsigned groups = bits(1) ? 8 : 4;
				std::vector<unsigned> width(groups);
				for(auto& w : width) w = bits(3) + 1;
				m_groups = groups, m_prefix = groups == 8 ? 3 : 2, m_width = width;
				m_table.assign(groups, {});
				size_t at = 0;
				for(unsigned g = 0; g < groups; g++) {
					size_t size = g + 1 == groups ? m_values.size() - at : std::min<size_t>(size_t(1) << width[g], m_values.size() - at);
					for(size_t i = at; i < at + size; i++) {
						unsigned v = m_values[i];
						m_table[g].push_back(uint16_t(v >= 0x100 ? 0x8000 | (v - 0x100 + 2) : v));
					}
					std::sort(m_table[g].begin(), m_table[g].end());
					at += size;
				}
				m_phase = Phase::Codes;
				break;
			}
			case Phase::Codes: {
				unsigned g = bits(int(m_prefix)), index = bits(int(m_width[g]));
				uint16_t symbol = index < m_table[g].size() ? m_table[g][index] : 0;
				out.push_back(symbol);  // given before its distance's bits are read
				if(symbol & 0x8000) m_phase = Phase::Distance;
				else if(++m_given >= m_total) m_phase = Phase::Done;
				break;
			}
			case Phase::Distance: {
				out.push_back(uint16_t(bits(1) ? bits(12) : bits(8)));
				m_phase = ++m_given >= m_total ? Phase::Done : Phase::Codes;
				break;
			}
			default: break;
			}
		} catch(Short&) {
			m_bit = mark;  // not all of it yet: again with the next word
			return true;
		}
	}
	return false;
}

} // namespace dsp3
