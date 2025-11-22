#ifndef DCC_HEADER
#define DCC_HEADER

#include <cstdint>

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
		DCCProtocol(int startBitLength);

		DCCPacket stop(uint8_t address);
		DCCPacket emergencyStop(uint8_t address);
		DCCPacket speed(uint8_t address, float speed, Direction direction);

	private:
		int startBitLength;

		DCCPacket rawSpeed(uint8_t address, uint8_t speed, Direction direction);
};

#endif
