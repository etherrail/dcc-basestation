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

			return this->rawSpeed(address, 2 + (int)(speed * 126), direction);
		};

	private:
		int startBitLength;

		// speed: 0–28 (0 = stop, 1 = emergency stop)
		DCCPacket rawSpeed(uint8_t address, uint8_t speed, Direction direction) {
			DCCPacket packet;

			ESP_LOGI(TAG, "%d speed = %d", address, speed);

			// 01 = 28-step control
			uint8_t instruction = 0b01000000;

			// set direction bit
			if (direction == FORWARD) {
				instruction |= 0b00100000;
			}

			// lower 5 bits = speed step
			instruction |= (speed & 0x1F);

			// XOR checksum
			uint8_t xor_byte = address ^ instruction;

			// Calculate total bit count: preamble (14) + (3 bytes × 9 bits) + final end bit
			const int total_bits = startBitLength + 3 * 9 + 1;
			packet.bits = new uint8_t[total_bits];
			packet.length = total_bits;

			int i = 0;

			// start bits
			for (int j = 0; j < startBitLength; j++) {
				packet.bits[i++] = 1;
			}

			// address
			packet.bits[i++] = 0;
			for (int bit = 7; bit >= 0; bit--) {
				packet.bits[i++] = (address >> bit) & 1;
			}

			// instruction
			packet.bits[i++] = 0;
			for (int bit = 7; bit >= 0; bit--) {
				packet.bits[i++] = (instruction >> bit) & 1;
			}

			// verification byte
			packet.bits[i++] = 0;
			for (int bit = 7; bit >= 0; bit--) {
				packet.bits[i++] = (xor_byte >> bit) & 1;
			}

			// end bit
			packet.bits[i++] = 1;

			return packet;
		};
};
