// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) StackBlender LLC
//
// Command 38: a stream coded with a table of its own (docs/commands.md). Words are taken with
// need() only when the next bit is wanted; symbols are given with give().
#pragma once

#include <cstdint>
#include <functional>

namespace dsp3 {

class Decoder {
public:
	Decoder(std::function<uint16_t ()> need, std::function<void (uint16_t)> give) : m_need(std::move(need)), m_give(std::move(give)) {}
	void run();

private:
	unsigned bits(int n);
	std::function<uint16_t ()> m_need;
	std::function<void (uint16_t)> m_give;
	uint16_t m_word = 0;
	int m_left = 0;  // bits of m_word not yet taken
};

} // namespace dsp3
