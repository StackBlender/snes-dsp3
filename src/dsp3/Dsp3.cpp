// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) StackBlender LLC
#include "dsp3/Dsp3.h"

#include "dsp3/Decoder.h"

#include <map>
#include <queue>

namespace dsp3 {

void Dsp3::reset() {
	m_command = 0;
	m_inCommand = false;
	m_in.clear();
	m_outRead = 0;
	m_wantsInput = false;
	m_pending = 0;
	m_finished = false;
	m_highByte = false;
	m_lowByte = 0;
	m_data = 0x8080;
	m_board = m_start = 0;
	m_boardRun = m_startRun = 0;
}

// Bit 7: ready (always). Bit 2: the next transfer is a byte (a command, or 8-bit data). Bit 6:
// command 38 wants its next word (after its two counts), as its program signals it.
uint8_t Dsp3::readStatus() const {
	bool byte = !m_inCommand || (m_wantsInput ? m_wantsByte : m_pendingByte);
	bool signal = m_inCommand && m_wantsInput && m_signal;
	return uint8_t(0x80 | (byte ? 0x04 : 0x00) | (signal ? 0x40 : 0x00));
}

uint8_t Dsp3::readData() {
	if(!m_inCommand) return uint8_t(m_highByte ? m_data >> 8 : m_data);
	if(!m_wantsInput && m_pendingByte) {  // a one-byte output
		uint8_t value = uint8_t(m_pending);
		m_data = uint16_t((m_data & 0xff00) | value);
		wordOut();
		return value;
	}
	if(!m_highByte) {
		m_highByte = true;
		return uint8_t((m_wantsInput ? m_data : m_pending) & 0xff);
	}
	m_highByte = false;
	if(m_wantsInput) {  // a read where an input is due: the chip takes the data register
		uint16_t word = m_data;
		wordIn(word);
		return uint8_t(word >> 8);
	}
	uint16_t word = m_pending;
	m_data = word;
	wordOut();
	return uint8_t(word >> 8);
}

void Dsp3::writeData(uint8_t value) {
	if(!m_inCommand) {  // a command byte
		m_command = value;
		m_inCommand = true;
		m_in.clear();
		m_outRead = 0;
		m_highByte = false;
		m_boardRun = m_board;
		m_startRun = m_start;
		run();
		return;
	}
	if(m_wantsInput && m_wantsByte) {
		m_data = uint16_t((m_data & 0xff00) | value);
		wordIn(value);
		return;
	}
	if(!m_highByte) {
		m_highByte = true;
		m_lowByte = value;
		return;
	}
	m_highByte = false;
	uint16_t word = uint16_t(m_lowByte | value << 8);
	m_data = word;
	if(m_wantsInput) wordIn(word);
	else wordOut();  // a write where an output is due takes its place
}

void Dsp3::wordIn(uint16_t word) {
	m_in.push_back(word);
	if(m_command == 0x38) {  // fed straight in: no replay
		std::vector<uint16_t> out;
		bool more = m_decoder.feed(word, out);
		for(auto w : out) m_queue.push_back({w, false});
		m_finished = !more;
		m_stopByte = false;
		m_stopSignal = m_in.size() >= 2;
		settle();
		return;
	}
	run();
}

void Dsp3::wordOut() {
	m_outRead++;
	m_queueAt++;
	settle();
}

uint16_t Dsp3::need() {
	if(m_inAt < m_in.size()) return m_in[m_inAt++];
	m_stopByte = false;
	m_stopSignal = (m_command == 0x38 && m_inAt >= 2) || (m_command == 0x1e && m_inAt >= 1);
	throw Stop{};
}

uint8_t Dsp3::needByte() {
	if(m_inAt < m_in.size()) return uint8_t(m_in[m_inAt++]);
	m_stopByte = true;
	m_stopSignal = false;
	throw Stop{};
}

// After a run or a read: the next queued output, else the input the command waits for, else idle
void Dsp3::settle() {
	if(m_queueAt < m_queue.size()) {
		m_wantsInput = false;
		m_pending = m_queue[m_queueAt].first;
		m_pendingByte = m_queue[m_queueAt].second;
		return;
	}
	if(m_finished) {
		m_inCommand = false;
		m_wantsInput = false;
		m_data = 0x8080;
		return;
	}
	m_wantsInput = true;
	m_wantsByte = m_stopByte;
	m_signal = m_stopSignal;
}

void Dsp3::give(uint16_t word) {
	if(m_outAt++ < m_outRead) return;  // already read
	m_queue.push_back({word, false});
}

void Dsp3::giveByte(uint8_t value) {
	if(m_outAt++ < m_outRead) return;
	m_queue.push_back({value, true});
}

// Runs the command from its start over the inputs it has, up to the next input it needs, queueing
// the outputs not yet read.
void Dsp3::run() {
	m_inAt = m_outAt = 0;
	m_queue.clear();
	m_queueAt = 0;
	m_finished = false;
	uint16_t board = m_boardRun, start = m_startRun;
	try {
		switch(m_command) {
		case 0x02: {  // sprites: scroll, then (tile, x, y) until a tile with bit 15 set
			int16_t sx = int16_t(need()), sy = int16_t(need());
			while(true) {
				uint16_t tile = need();
				if(tile & 0x8000) break;
				int16_t dx = int16_t(int16_t(need()) - sx), dy = int16_t(int16_t(need()) - sy);
				bool xOk = dx >= -64 && dx < 320, yOk = dy >= -64 && dy < 288, ok = xOk && yOk;
				int32_t x = dx >= 0x200 ? dx - 0x400 : dx < -0x200 ? dx + 0x400 : dx;
				give(uint16_t((ok ? 0 : 0x8000) | (yOk ? 1 : 0)));
				give(uint16_t(x));
				give(ok ? uint16_t(dy) : 0x8000);
				give(ok ? 0x8000 : 0x8080);
			}
			break;
		}
		case 0x03: {  // a cell's offset on the board
			uint16_t cell = need();
			give(uint16_t((cell >> 8) * (board & 0xff) + (cell & 0xff)));
			break;
		}
		case 0x06:  // the board's dimensions
			board = need();
			m_board = board;
			break;
		case 0x07: {  // the cell next to one, in one of six directions (a hex board; it wraps). The
			// direction is a byte: 0-11 are the six directions twice over, 12 stays; larger ones
			// (unused by the game, and odd on the chip) stay here too
			int direction = needByte();
			uint16_t cell = need();
			int rows = board >> 8, cols = board & 0xff;
			int row = cell >> 8, col = cell & 0xff;
			static const int even[6][2] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 0}, {0, -1}, {-1, -1}};
			static const int odd[6][2] = {{-1, 0}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}};
			static const int stay[2] = {0, 0};
			const int* move = direction >= 12 ? stay : col % 2 ? odd[direction % 6] : even[direction % 6];
			row = rows ? (row + move[0] + rows) % rows : row + move[0];
			col = cols ? (col + move[1] + cols) % cols : col + move[1];
			give(uint16_t(row << 8 | col));
			give(uint16_t(row * cols + col));
			break;
		}
		case 0x0f:  // memory test: passed
			need();
			give(0x0000);
			break;
		case 0x18: {  // 8-bit pixels to bitplanes: a count of rows, then per row 8 pixels (bytes) in,
			// 4 words out (planes 0/1, 2/3, 4/5, 6/7: low byte the even plane)
			uint16_t rows = need();
			for(unsigned r = 0; r < rows; r++) {
				uint8_t pixel[8];
				for(auto& p : pixel) p = needByte();
				for(int pair = 0; pair < 4; pair++) {
					uint16_t word = 0;
					for(int x = 0; x < 8; x++) {
						word |= uint16_t((pixel[x] >> (2 * pair) & 1) << (7 - x));
						word |= uint16_t((pixel[x] >> (2 * pair + 1) & 1) << (15 - x));
					}
					give(word);
				}
			}
			break;
		}
		case 0x1f:  // "transfer the data ROM": 1024 words. The chip's table isn't reproduced (it is
			// the chip maker's); zeros take its place (docs/exactness.md)
			need();
			for(int i = 0; i < 1024; i++) give(0x0000);
			break;
		case 0x1e: {  // movement costs from the start cell (3E) over a hex area of a given range
			// Phase 1: the range (high byte); then each cell of the area (offset), answered with two
			// bytes (a flag, and the cost of entering it; FFh: it can't be entered). Then FFFFh.
			// Phase 2: the range again; then each cell (offset) and its cost from the start (a byte,
			// 255 if it can't be reached). The cells come by sector (the six directions), each by
			// ring 1 to the range, k cells in ring k.
			int range = need() >> 8;
			int rows = board >> 8, cols = board & 0xff;
			auto step = [&](int& row, int& col, int direction) {
				static const int even[6][2] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 0}, {0, -1}, {-1, -1}};
				static const int odd[6][2] = {{-1, 0}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}};
				const int* move = col % 2 ? odd[direction % 6] : even[direction % 6];
				row = rows ? (row + move[0] + rows) % rows : row + move[0];
				col = cols ? (col + move[1] + cols) % cols : col + move[1];
			};
			std::vector<std::pair<int, int>> area;
			for(int sector = 0; sector < 6; sector++) {
				for(int k = 1; k <= range; k++) {
					int row = start >> 8, col = start & 0xff;
					for(int i = 0; i < k; i++) step(row, col, sector);
					for(int j = 0; j < k; j++) area.push_back({row, col}), step(row, col, sector + 2);
				}
			}
			std::vector<uint8_t> enter(area.size());
			for(size_t i = 0; i < area.size(); i++) {
				give(uint16_t(area[i].first * cols + area[i].second));
				needByte();
				enter[i] = needByte();
			}
			give(0xffff);
			need();
			// The shortest costs over the area (the original's own search differs in a few cells:
			// docs/exactness.md)
			std::map<std::pair<int, int>, size_t> at;
			for(size_t i = 0; i < area.size(); i++) at[area[i]] = i;
			std::vector<int> cost(area.size(), 1 << 30);
			std::priority_queue<std::pair<int, std::pair<int, int>>, std::vector<std::pair<int, std::pair<int, int>>>, std::greater<>> queue;
			std::pair<int, int> origin{start >> 8, start & 0xff};
			queue.push({0, origin});
			std::map<std::pair<int, int>, int> best{{origin, 0}};
			while(!queue.empty()) {
				auto [d, cell] = queue.top();
				queue.pop();
				if(d > best[cell]) continue;
				for(int direction = 0; direction < 6; direction++) {
					int row = cell.first, col = cell.second;
					step(row, col, direction);
					auto found = at.find({row, col});
					if(found == at.end() || enter[found->second] == 0xff) continue;
					int nd = d + enter[found->second];
					if(nd < cost[found->second]) {
						cost[found->second] = nd;
						best[{row, col}] = nd;
						queue.push({nd, {row, col}});
					}
				}
			}
			for(size_t i = 0; i < area.size(); i++) {
				give(uint16_t(area[i].first * cols + area[i].second));
				giveByte(uint8_t(cost[i] > 255 ? 255 : cost[i]));
			}
			give(0xffff);
			break;
		}
		case 0x2f:  // program version
			need();
			give(0x0300);
			break;
		case 0x38: {  // decode a coded stream: the Decoder, fed the words so far (wordIn() feeds
			// it directly while the command runs; this replays them, from the start or a load)
			m_decoder = Decoder();
			std::vector<uint16_t> out;
			bool more = true;
			for(auto w : m_in) {
				more = m_decoder.feed(w, out);
				if(!more) break;
			}
			for(auto w : out) give(w);
			if(more) {
				m_stopByte = false;
				m_stopSignal = m_in.size() >= 2;
				throw Stop{};
			}
			break;
		}
		case 0x3e: {  // the start cell for 1E, and its offset
			start = need();
			m_start = start;
			give(uint16_t((start >> 8) * (board & 0xff) + (start & 0xff)));
			break;
		}
		default:
			break;
		}
		m_finished = true;
	} catch(Stop&) {
	}
	settle();
}

std::vector<uint8_t> Dsp3::saveState() const {
	std::vector<uint8_t> s = {'D', '3', 1, m_command, uint8_t(m_inCommand), uint8_t(m_highByte), m_lowByte};
	auto put16 = [&](uint32_t v) { s.push_back(uint8_t(v)); s.push_back(uint8_t(v >> 8)); };
	put16(m_data); put16(m_board); put16(m_start); put16(m_boardRun); put16(m_startRun);
	put16(uint32_t(m_outRead)); put16(uint32_t(m_outRead >> 16));
	put16(uint32_t(m_in.size())); put16(uint32_t(m_in.size() >> 16));
	for(auto w : m_in) put16(w);
	return s;
}

bool Dsp3::loadState(const uint8_t* data, size_t size) {
	if(size < 25 || data[0] != 'D' || data[1] != '3' || data[2] != 1) return false;
	auto get16 = [&](size_t at) { return uint16_t(data[at] | data[at + 1] << 8); };
	size_t n = get16(21) | size_t(get16(23)) << 16;
	if(size != 25 + 2 * n) return false;
	m_command = data[3];
	m_inCommand = data[4];
	m_highByte = data[5];
	m_lowByte = data[6];
	m_data = get16(7); m_board = get16(9); m_start = get16(11); m_boardRun = get16(13); m_startRun = get16(15);
	m_outRead = get16(17) | size_t(get16(19)) << 16;
	m_in.clear();
	for(size_t i = 0; i < n; i++) m_in.push_back(get16(25 + 2 * i));
	if(m_inCommand) run();
	return true;
}

} // namespace dsp3
