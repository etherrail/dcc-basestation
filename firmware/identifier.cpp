#include <Arduino.h>

#include "identifier.hpp"

Identifier::Identifier() {
	uint64_t chip = ESP.getEfuseMac();

	name = "etherrail-dcc-base-sstation-" + String((uint16_t)(chip >> 32), HEX) + String((uint32_t)chip, HEX);
	name.replace(":", "");
	name.toLowerCase();
};
