#include "speed-permit.hpp"
#include "status-indicator.hpp"

#include <Arduino.h>

float tempMaxDecl = 10.0;

SpeedPermit::SpeedPermit() {
	updateTime = millis();
	permittedSpeed = 0;
};

void SpeedPermit::update(float speed) {
	updateTime = millis();
	permittedSpeed = speed;
};

float SpeedPermit::currentSpeed() {
	unsigned long elapsed = millis() - updateTime;
	
	if (elapsed < SPEED_PERMIT_VALIDITY + SPEED_PERMIT_HOLD) {
		status.permit(STATUS_SUCCESS);

		return permittedSpeed;
	}
	
	if (elapsed < SPEED_PERMIT_VALIDITY + SPEED_PERMIT_HOLD + SPEED_PERMIT_EMERGENCY) {
		status.permit(STATUS_WARNING);

		return max(permittedSpeed - calculateDefaultBreakingDeceleration() * float(elapsed / 1000), 0.0f);
	}

	status.permit(STATUS_ERROR);

	return max(permittedSpeed - tempMaxDecl * float(elapsed / 1000), 0.0f);
};

float SpeedPermit::calculateDefaultBreakingDeceleration() {
	return tempMaxDecl * SPEED_PERMIT_BREAKING_FACTOR;
};