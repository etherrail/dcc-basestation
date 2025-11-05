#include "motor.hpp"

#include <Arduino.h>

#define SPEED_PERMIT_VALIDITY 1500

#define MOTOR_RESOLUTION 8

#define MOTOR_A_PIN 7
#define MOTOR_B_PIN 10

int steps[][2] = {
	{ 15, 230 },
	{ 15, 322 },
	{ 30, 392 },
	{ 15, 466 },
	{ 15, 622 },
	{ 15, 784 },
	{ 15, 932 },
	{ 15, 1244 }
};

const int stepCount = sizeof(steps) / sizeof(steps[0]);

Motor::Motor(SpeedPermit &permit, uint32_t frequency) : pin(pin), speedPermit(permit) {
	/// ledcAttach(MOTOR_A_PIN, frequency, MOTOR_RESOLUTION);
	/// ledcAttach(MOTOR_B_PIN, frequency, MOTOR_RESOLUTION);
};

void Motor::begin() {
	pinMode(MOTOR_A_PIN, OUTPUT);
	pinMode(MOTOR_B_PIN, OUTPUT);

	// turn off initially
	stop();
};

void Motor::setSpeed(uint8_t speed) {
	if (currentSpeed == speed) {
		return;
	}

	currentSpeed = speed;

	int stepRemainder = speed;

	for (byte stepIndex = 0; stepIndex < stepCount; stepIndex++) {
		int size = steps[stepIndex][0];
		int frequency = steps[stepIndex][1];

		if (size < stepRemainder) {
			setFrequency(frequency);
		}

		stepRemainder -= size;
	}

	if (speed > 0) {
		digitalWrite(MOTOR_A_PIN, HIGH);
		digitalWrite(MOTOR_B_PIN, LOW);
	} else {
		digitalWrite(MOTOR_A_PIN, HIGH);
		digitalWrite(MOTOR_B_PIN, HIGH);
	}
	
	/// ledcWrite(MOTOR_A_PIN, speed);
	/// ledcWrite(MOTOR_B_PIN, 0);
};

void Motor::stop() {
	setSpeed(0);
};

void Motor::setFrequency(uint32_t frequency) {
	/// ledcChangeFrequency(MOTOR_A_PIN, frequency, MOTOR_RESOLUTION);
	/// ledcChangeFrequency(MOTOR_B_PIN, frequency, MOTOR_RESOLUTION);
};

void Motor::update() {
	uint8_t speed = uint8_t(speedPermit.currentSpeed());

	setSpeed(speed);
};