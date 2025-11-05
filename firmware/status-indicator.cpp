#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "status-indicator.hpp"

#define STATUS_INDICATOR_PIN 9

StatusIndicator status;

StatusIndicator::StatusIndicator() {
	pixels = Adafruit_NeoPixel(3, STATUS_INDICATOR_PIN, NEO_GRB + NEO_KHZ800);
	pixels.setBrightness(10);
};

void StatusIndicator::network(byte status) {
	setLight(0, status);
};

void StatusIndicator::connection(byte status) {
	setLight(1, status);
};

void StatusIndicator::permit(byte status) {
	setLight(2, status);
};

void StatusIndicator::setLight(int index, byte status) {
	byte red = 0xff;
	byte green = 0xff;
	byte blue = 0xff;

	switch (status) {
		case STATUS_SUCCESS: { red = 0x00; green = 0xff; blue = 0x00; break; }
		case STATUS_ACTIVE: { red = 0x00; green = 0x00; blue = 0xff; break; }
		case STATUS_WARNING: { red = 0xff; green = 0xff; blue = 0x00; break; }
		case STATUS_ERROR: { red = 0xff; green = 0x00; blue = 0x00; break; }
		case STATUS_IDLE: { red = 0xff; green = 0x00; blue = 0xff; break; }
	}

	pixels.setPixelColor(index, red, green, blue);
	pixels.show();
};