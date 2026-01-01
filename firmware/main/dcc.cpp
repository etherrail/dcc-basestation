#pragma once

#include "esp_log.h"
#include "pthread.h"
#include <cstdint>
#include <sys/types.h>

#define TAG "DCC"

typedef struct {
	uint8_t *bits;
	int length;
} DCCPacket;

typedef enum {
	FORWARD,
	REVERSE
} Direction;

typedef uint8_t DCCAddress;

#define BIN8_FMT "%c%c%c%c%c%c%c%c"
#define BIN8_ARG(x) \
	((x)&0x80?'1':'0'), ((x)&0x40?'1':'0'), ((x)&0x20?'1':'0'), ((x)&0x10?'1':'0'), \
	((x)&0x08?'1':'0'), ((x)&0x04?'1':'0'), ((x)&0x02?'1':'0'), ((x)&0x01?'1':'0')

class DCCProtocol {
	public:
		DCCProtocol(
			int startBitLength // start packet length (preamble)
		): startBitLength(startBitLength) {}

		DCCPacket stop(DCCAddress address) {
			return this->rawSpeed(address, 0, FORWARD);
		}

		DCCPacket emergencyStop(DCCAddress address) {
			return this->rawSpeed(address, 1, FORWARD);
		}

		DCCPacket speed(DCCAddress address, float speed, Direction direction) {
			if (speed == 0.0f) {
				return stop(address);
			}

			if (speed < 0.0f) {
				speed = 0.0f;
			}

			if (speed > 1.0f) {
				speed = 1.0f;
			}

			return this->rawSpeed(address, 2 + (int)(speed * 125), direction);
		}

		DCCPacket functionBlockA(DCCAddress address, bool f0, bool f1, bool f2, bool f3, bool f4) {
			uint8_t block = 0x80 | (f0 << 4) | (f4 << 3) | (f3 << 2) | (f2 << 1) | (f1 << 0);

			return this->package(address, (uint8_t[]){ block }, 1);
		}

		DCCPacket functionBlockB(DCCAddress address, bool f5, bool f6, bool f7, bool f8) {
			uint8_t block = 0xb0 | (f8 << 3) | (f7 << 2) | (f6 << 1) | (f5 << 0);

			return this->package(address, (uint8_t[]){ block }, 1);
		}

		DCCPacket functionBlockC(DCCAddress address, bool f9, bool f10, bool f11, bool f12) {
			uint8_t block = 0xa0 | (f12 << 3) | (f11 << 2) | (f10 << 1) | (f9 << 0);

			return this->package(address, (uint8_t[]){ block }, 1);
		}

		DCCPacket functionBlockD(DCCAddress address, bool f13, bool f14, bool f15, bool f16, bool f17, bool f18, bool f19, bool f20) {
			uint8_t data = (f13 << 0) | (f14 << 1) | (f15 << 2) | (f16 << 3) | (f17 << 4) | (f18 << 5) | (f19 << 6) | (f20 << 7);

			return this->package(address, (uint8_t[]){ 0xde, data }, 2);
		}

		DCCPacket functionBlockE(DCCAddress address, bool f21, bool f22, bool f23, bool f24, bool f25, bool f26, bool f27, bool f28) {
			uint8_t data = (f21 << 0) | (f22 << 1) | (f23 << 2) | (f24 << 3) | (f25 << 4) | (f26 << 5) | (f27 << 6) | (f28 << 7);

			return this->package(address, (uint8_t[]){ 0xde, data }, 2);
		}

	private:
		int startBitLength;

		// speed: 0–28 (0 = stop, 1 = emergency stop)
		DCCPacket rawSpeed(DCCAddress address, uint8_t speed, Direction direction) {
			uint8_t dataByte = (speed & 0x7F);

			if (direction == FORWARD) {
				dataByte |= 0x80; // direction in bit7
			}

			return this->package(address, (uint8_t[]){ 0x3F, dataByte }, 2);
		}

		DCCPacket package(DCCAddress address, uint8_t data[], uint8_t dataLength) {
			DCCPacket packet;

			const int bitCount = startBitLength + // preamble
				9 + // address
				dataLength * 9 + // data length
				9 + // xor
				1; // end bit

			packet.bits = new uint8_t[bitCount];
			packet.length = bitCount;

			int bitIndex = 0;

			for (int preambleIndex = 0; preambleIndex < startBitLength; preambleIndex++) {
				packet.bits[bitIndex++] = 1;
			}

			uint8_t signature = address;

			this->pushByte(packet.bits, bitIndex, address);

			for (uint8_t dataIndex = 0; dataIndex < dataLength; dataIndex++) {
				signature ^= data[dataIndex];

				this->pushByte(packet.bits, bitIndex, data[dataIndex]);
			}

			this->pushByte(packet.bits, bitIndex, signature);

			// end bit
			packet.bits[bitIndex++] = 1;

			return packet;
		}

		static inline void pushByte(uint8_t* bits, int& idx, uint8_t data) {
			bits[idx++] = 0;

			for (int bit = 7; bit >= 0; bit--) { // MSB first
				bits[idx++] = (data >> bit) & 1;
			}
		}
};
