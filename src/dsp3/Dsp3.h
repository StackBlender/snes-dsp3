// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) StackBlender LLC
//
// A clean-room DSP-3: the coprocessor in SD Gundam GX (an NEC uPD77C25 running the console
// maker's program), reimplemented from the public list of its commands and black-box measurement
// of what the chip returns, never from its program.
//
// The SNES sees two registers: data and status. A command is a byte; its inputs and outputs are
// 16-bit words, low byte first. Status bit 7 (ready) is always set here; bit 6 is set while the
// chip waits for an input word (games watch it in the streaming commands, 02 and 38). As on the
// chip, a write where an output is due takes the output's place (the written word is lost).
#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "dsp3/Decoder.h"

namespace dsp3 {

class Dsp3 {
public:
	Dsp3() { reset(); }
	void reset();

	uint8_t readData();
	void writeData(uint8_t value);
	uint8_t readStatus() const;

	std::vector<uint8_t> saveState() const;
	bool loadState(const uint8_t* data, size_t size);

private:
	// A command is a little program: it takes words with need(), gives them with give(). It is
	// run from the start each time an input arrives, over the inputs so far (they are kept), up
	// to the next input it needs, its outputs from there on queued. So the state to save is just
	// the command, its inputs and how many outputs were read.
	struct Stop {};
	uint16_t need();
	uint8_t needByte();  // an input transferred as one byte (the chip's 8-bit mode)
	void give(uint16_t word);
	void giveByte(uint8_t value);  // an output transferred as one byte
	void run();

	void wordIn(uint16_t word);
	void wordOut();
	void settle();

	uint8_t m_command = 0;
	bool m_inCommand = false;  // a command is running (else the next byte is a command)
	std::vector<uint16_t> m_in;   // the current command's input words
	size_t m_outRead = 0;      // its outputs read (or skipped) so far
	// while running: where it is
	size_t m_inAt = 0, m_outAt = 0;
	bool m_wantsInput = false; // the command waits for an input word (else it has an output)
	bool m_wantsByte = false;  // ... an input byte (8-bit mode)
	uint16_t m_pending = 0;    // the output waiting to be read
	bool m_pendingByte = false;  // ... as one byte
	std::vector<std::pair<uint16_t, bool>> m_queue;  // outputs not yet read (word, as a byte)
	size_t m_queueAt = 0;
	bool m_stopByte = false, m_stopSignal = false;  // how the last run's need() asked
	Decoder m_decoder;  // command 38's, fed as words arrive (replayed from m_in after a load)
	bool m_signal = false;     // status bit 6: the command signals that it wants a word
	bool m_finished = false;
	bool m_highByte = false;
	uint8_t m_lowByte = 0;
	uint16_t m_data = 0;       // the data register
	// the board (06) and start cell (3E), kept between commands
	uint16_t m_board = 0;
	uint16_t m_start = 0;
	uint16_t m_boardRun = 0, m_startRun = 0;  // as they were when the current command began
};

} // namespace dsp3
