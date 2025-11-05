#ifndef SPEED_PERMIT_HEADER
#define SPEED_PERMIT_HEADER

#include <Arduino.h>

#define SPEED_PERMIT_VALIDITY 1000
#define SPEED_PERMIT_HOLD 500
#define SPEED_PERMIT_BREAKING_FACTOR 0.1
#define SPEED_PERMIT_EMERGENCY 5000

class SpeedPermit {
	public:
		SpeedPermit();

		void update(float speed);
		float currentSpeed();

	private:
		unsigned long updateTime;
		float permittedSpeed;

		float calculateDefaultBreakingDeceleration();
};

#endif