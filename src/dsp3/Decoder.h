// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) StackBlender LLC
//
// Command 38: a stream coded with a table of its own (docs/commands.md). Fed a word at a time;
// after each it gives every symbol the words so far hold, as the chip does before asking for
// the next word.
#pragma once

#include <cstdint>
#include <vector>

namespace dsp3 {

class Decoder {
public:
	// A word in; the symbols (and distances) it completes are appended to out. Returns false once
	// the stream is done (every symbol given).
	bool feed(uint16_t word, std::vector<uint16_t>& out);
	bool done() const { return m_phase == Phase::Done; }

private:
	struct Short {};
	unsigned bits(int n);
	enum class Phase { Count, Total, Symbols, Groups, Codes, Distance, Done } m_phase = Phase::Count;
	std::vector<uint16_t> m_words;
	size_t m_bit = 0;  // the next bit, counted from the first stream word
	unsigned m_n = 0, m_total = 0, m_given = 0, m_previous = 0;
	std::vector<unsigned> m_values;
	unsigned m_groups = 0, m_prefix = 0;
	std::vector<unsigned> m_width;
	std::vector<std::vector<uint16_t>> m_table;
};

} // namespace dsp3
