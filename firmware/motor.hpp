#ifndef MOTOR_HEADER
#define MOTOR_HEADER

#include <Arduino.h>

#include "speed-permit.hpp"

class Motor {
	public:
		uint8_t pin;

		Motor(SpeedPermit &permit, uint32_t frequency = 440);
		void begin();

		// updates sound sequence
		void update();

		void setFrequency(uint32_t frequency);

	private:
		SpeedPermit &speedPermit;
		uint8_t currentSpeed;
		
		void setSpeed(uint8_t speed);
		void stop();
};

#endif