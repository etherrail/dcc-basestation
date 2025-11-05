#include "status-indicator.hpp"
#include "identifier.hpp"

#include "connection.hpp"
#include "speed-permit.hpp"
#include "motor.hpp"
#include "message.hpp"

#include <SPI.h>
#include <ArduinoOTA.h>

Connection connection;
SpeedPermit speedPermit;

String readBuffer = "";

void connectionLoopHandler(WiFiClient client) {
	// read a couple bytes before parsing
	while (client.available()) {
		readBuffer += (char)client.read();
	}

	if (Message::valid(readBuffer)) {
		Message message = Message::from(readBuffer);

		Serial.print("* ");
		Serial.println(message.route);

		readBuffer = "";

		if (message.route.equals("train/speed/permit")) {
			speedPermit.update(message.getHeader("speed").toFloat());
		}
	}
}

void setup() {
	Serial.begin(9600);

	Identifier identifier;

  	while (WiFi.status() != WL_CONNECTED) {
    	delay(5);
  	}

	ArduinoOTA.setHostname(identifier.name.c_str());
	ArduinoOTA.begin();

	status.network(STATUS_SUCCESS);

	xTaskCreate(updateMotor, "", 10000, NULL, 1, NULL);
	xTaskCreate(checkUpdate, "", 10000, NULL, 1, NULL);
	// xTaskCreate(checkPower, "", 10000, NULL, 1, NULL);

	delay(10000);

	status.network(STATUS_IDLE);
	delay(1000);

	digitalWrite(MOTOR_A_PIN, HIGH);
	digitalWrite(MOTOR_B_PIN, LOW);

	while (true) {
		delay(1);
	}

	Serial.println("creating connection");
	connection.begin(connectionLoopHandler);
}

void updateMotor(void *parameter) {
	while (true) {
		speedPermit.currentSpeed();

		// motor.update();

		delay(50);
	}
}

void checkPower(void *parameter) {
	/*
	while (true) {
		if (analogRead(POWER_SENSE_PIN) < 25) {
			esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
			esp_sleep_enable_timer_wakeup(500 * 1000);

			while (analogRead(POWER_SENSE_PIN) < 25) {
				esp_light_sleep_start();

				status(true);
				delay(5);

				status(false);
			}

			esp_wifi_set_ps(WIFI_PS_NONE);
		}
	}
	*/
}

void checkUpdate(void *parameter) {
	while (true) {
		ArduinoOTA.handle();

		delay(10);
	}
}

void loop() {}
