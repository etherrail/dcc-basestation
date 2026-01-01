#pragma once

#include "esp_log.h"

#define TAG "DCC"

typedef struct {
	uint8_t *bits;
	int length;
} DCCPacket;

typedef enum {
	FORWARD,
	REVERSE
} Direction;

#define BIN8_FMT "%c%c%c%c%c%c%c%c"
#define BIN8_ARG(x) \
	((x)&0x80?'1':'0'), ((x)&0x40?'1':'0'), ((x)&0x20?'1':'0'), ((x)&0x10?'1':'0'), \
	((x)&0x08?'1':'0'), ((x)&0x04?'1':'0'), ((x)&0x02?'1':'0'), ((x)&0x01?'1':'0')

class DCCProtocol {
	public:
		DCCProtocol(
			int startBitLength // start packet length (preamble)
		): startBitLength(startBitLength) {}

		DCCPacket stop(uint8_t address) {
			return this->rawSpeed(address, 0, FORWARD);
		};

		DCCPacket emergencyStop(uint8_t address) {
			return this->rawSpeed(address, 1, FORWARD);
		};

		DCCPacket speed(uint8_t address, float speed, Direction direction) {
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
		};

	private:
		int startBitLength;

		// speed: 0–28 (0 = stop, 1 = emergency stop)
		DCCPacket rawSpeed(uint8_t address, uint8_t speed, Direction direction) {
			DCCPacket packet;

			const uint8_t addressByte = address;
			const uint8_t instructionByte = 0x3F;

			uint8_t dataByte = (speed & 0x7F);

			if (direction == FORWARD) {
				dataByte |= 0x80; // direction in bit7
			}

			const uint8_t signature = addressByte ^ instructionByte ^ dataByte;

			const int bitCount = startBitLength + 4 * 9 + 1;
			packet.bits = new uint8_t[bitCount];
			packet.length = bitCount;

			int bitIndex = 0;

			// preamble
			for (int j = 0; j < startBitLength; j++) {
				packet.bits[bitIndex++] = 1;
			}

			// write bits
			auto push = [&](uint8_t b) {
				packet.bits[bitIndex++] = 0;

				for (int bit = 7; bit >= 0; bit--) {
					packet.bits[bitIndex++] = (b >> bit) & 1;
				}
			};

			push(addressByte);
			push(instructionByte);
			push(dataByte);
			push(signature);

			// end bit
			packet.bits[bitIndex++] = 1;

			ESP_LOGI(
				TAG,
				"%d speed = %d, adr=" BIN8_FMT " ins=" BIN8_FMT " data=" BIN8_FMT " sig=" BIN8_FMT,
				address,
				speed,
				BIN8_ARG(addressByte),
				BIN8_ARG(instructionByte),
				BIN8_ARG(dataByte),
				BIN8_ARG(signature)
			);

			return packet;
		};
};
