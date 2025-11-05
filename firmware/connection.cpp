#include <Arduino.h>
#include <WiFi.h>

#include "connection.hpp"
#include "discovery.hpp"
#include "status-indicator.hpp"

#define CONNECTION_PORT 141

void Connection::begin(ConnectionLoopHandler handler) {
	connectionLoopHandler = handler;

	Serial.println("creating discovery...");
	Discovery discovery();

	Serial.println("finding host...");

	// find host on first connect
	findHost();

	WiFiClient client();

	// start connecting
	reconnectionAttempts = 0;

	while (true) {
		connect();
	}
};


void Connection::findHost() {
	Serial.println("find host...");

	host = discovery.find();
};

void Connection::connect() {
	Serial.println("connecting");
	status.connection(STATUS_ACTIVE);

	client.stop();
	reconnectionAttempts++;

	Serial.println("stopped client");

	// search for a host again after 25 failed attempts
	// the old host might have gone down and been replaced by a new host
	if (reconnectionAttempts == 25) {
		reconnectionAttempts = 0;

		status.connection(STATUS_ERROR);

		findHost();
	}

	Serial.print("connecting to ");
	Serial.println(host);

	if (client.connect(host, CONNECTION_PORT)) {
		reconnectionAttempts = 0;

		Serial.println("connected");

		status.connection(STATUS_SUCCESS);

		while (client.connected()) {
			connectionLoopHandler(client);
		}

		Serial.println("disconnected");
	} else {
		Serial.println("retrying connection");
	}
};