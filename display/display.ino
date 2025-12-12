#include "SPI.h"
#include <Wire.h>
#include <INA226_WE.h>

const int DISPLAY_DIN_PIN = 0;
const int DISPLAY_CLK_PIN = 1;
const int DISPLAY_CS_PIN = 2;

const int DISPLAY_COUNT = 4;

const int VOLTAGE_SENSOR_SDA = 3;
const int VOLTAGE_SENSOR_SCL = 4;
const char VOLTAGE_SENSOR_ADDRESS = 0x40;

INA226_WE powerSensor = INA226_WE(VOLTAGE_SENSOR_ADDRESS);

enum {
	REG_NOOP = 0x00,
	REG_DIGIT0 = 0x01,
	REG_DIGIT1 = 0x02,
	REG_DIGIT2 = 0x03,
	REG_DIGIT3 = 0x04,
	REG_DIGIT4 = 0x05,
	REG_DIGIT5 = 0x06,
	REG_DIGIT6 = 0x07,
	REG_DIGIT7 = 0x08,
	REG_DECODE_MODE = 0x09,
	REG_INTENSITY = 0x0A,
	REG_SCAN_LIMIT = 0x0B,
	REG_SHUTDOWN = 0x0C,
	REG_DISPLAYTEST = 0x0F
};

void send16(uint8_t reg, uint8_t data) {
	SPI.transfer(reg);
	SPI.transfer(data);
}

// Send the same command/data to ALL chips at once
void sendAll(uint8_t reg, uint8_t data) {
	digitalWrite(DISPLAY_CS_PIN, LOW);

	for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
		send16(reg, data);
	}
	
	digitalWrite(DISPLAY_CS_PIN, HIGH);
}

// Send a command to ONE specific chip (0 = first nearest the Arduino)
void sendOne(uint8_t chipIndex, uint8_t reg, uint8_t data, bool dot = false) {
	data = dot ? (data | 0x80) : (data & 0x7F);

	digitalWrite(DISPLAY_CS_PIN, LOW);

	// Shift out for each position in the chain:
	// the farthest chip is shifted first.
	for (int8_t i = DISPLAY_COUNT - 1; i >= 0; i--) {
		if (i == chipIndex) {
			send16(reg, data);
		} else {
			send16(REG_NOOP, 0x00);
		}
	}

	digitalWrite(DISPLAY_CS_PIN, HIGH);
}

void clearAll() {
	for (uint8_t d = REG_DIGIT0; d <= REG_DIGIT7; d++) {
		sendAll(d, 0x00);
	}
}

void setupMax7219() {
	sendAll(REG_DISPLAYTEST, 0x00);
	sendAll(REG_SHUTDOWN, 0x01);
	sendAll(REG_DECODE_MODE, 0xFF);
	sendAll(REG_SCAN_LIMIT, 0x07);
	sendAll(REG_INTENSITY, 0x07); // 0 - 7
	
	clearAll();
}

static uint32_t pow10u(uint8_t e) {
	static const uint32_t p10[] = {1u, 10u, 100u, 1000u, 10000u, 100000u, 1000000u, 10000000u};
	return p10[e];
}

void show(char displayIndex, char displaySize, float value) {
	const uint8_t n = (uint8_t)displaySize;
	const uint32_t maxInt = pow10u(n) - 1;
	const uint32_t limitForOverflow = pow10u(n) - 1; // values >= this cannot be shown → show all 9s

	if (value < 0.0f) value = 0.0f;

	uint8_t decimals = 0;
	uint32_t scaledInt = 0;

	if (value >= (float)limitForOverflow) {
		// 1000 on a 3-digit display, etc. → show 999
		decimals = 0;
		scaledInt = maxInt;
	} else {
		// Try most decimals first and reduce until it fits
		// We round to nearest so 5.1241 on 3 digits → 5.12, 12.4234 → 12.4, 491.444 → 491
		for (int d = n - 1; d >= 0; --d) {
			float scaledF = value * (float)pow10u(d);
			// Fast positive rounding without <math.h> roundf
			uint32_t rounded = (uint32_t)(scaledF + 0.5f); 
			if (rounded <= maxInt) {
				decimals = (uint8_t)d;
				scaledInt = rounded;
				break;
			}
		}
	}

	// 3) Emit digits least-significant first. 
	// The decimal point belongs to the ones place, which occurs AFTER 'decimals' fractional digits.
	uint32_t tmp = scaledInt;

	for (uint8_t i = 0; i < n; ++i) {
		uint8_t digitVal = (uint8_t)(tmp % 10u);
		bool showDot = (decimals > 0) && (i == decimals); // turn dot on for the ones digit
		sendOne(displayIndex, REG_DIGIT0 + displaySize - i - 1, digitVal, showDot);
		tmp /= 10u;
	}
}

void setup() {
	Serial.begin(9600);

	Wire.begin(VOLTAGE_SENSOR_SDA, VOLTAGE_SENSOR_SCL);

	powerSensor.init();
	powerSensor.setAverage(INA226_AVERAGE_16);
	powerSensor.setMeasureMode(INA226_CONTINUOUS);
	powerSensor.setCorrectionFactor(1.00);
	powerSensor.setResistorRange(0.4);
	powerSensor.waitUntilConversionCompleted();

	pinMode(DISPLAY_CS_PIN, OUTPUT);
	digitalWrite(DISPLAY_CS_PIN, HIGH);
	SPI.begin(DISPLAY_CLK_PIN, -1, DISPLAY_DIN_PIN);

	// MAX7219 is happy at up to ~10 MHz, but long wires prefer slower
	SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
	setupMax7219();

	// Demo: write “01234567” on each chip’s 8 digits
	const uint8_t segVals[8] = {0,1,2,3,4,5,6,7};
	
	for (uint8_t chip = 0; chip < DISPLAY_COUNT; chip++) {
		for (uint8_t d = 0; d < 8; d++) {
			// For decode-mode, data is the numeral 0..15 (with bit 7 = DP)
			sendOne(chip, REG_DIGIT0 + d, segVals[d]);
		}
	}
}

void loop() {
	// powerSensor.readAndClearFlags();
	float voltage = powerSensor.getBusVoltage_V();
	float current = powerSensor.getCurrent_mA();
	float shuntVoltage_mV = powerSensor.getShuntVoltage_mV();

	show(0, 4, voltage);
	show(1, 3, current);

	Serial.println(shuntVoltage_mV);
}