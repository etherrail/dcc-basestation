#include "Discovery.hpp"
#include "Identifier.hpp"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define DISCOVERY_REQUEST_PORT 142
#define DISCOVERY_RESPONSE_PORT 143

Discovery::Discovery() {
    Identifier id;  // Create an Identifier object

    requestPacket = "PT login\ndevice: " + id.name + "\n\n";
}

void Discovery::sendRequest() {
	Serial.println("sent discovery request...");

	network.beginPacket(IPAddress(255, 255, 255, 255), DISCOVERY_REQUEST_PORT);
	network.write((uint8_t *)requestPacket.c_str(), requestPacket.length());
	network.endPacket();
}

IPAddress Discovery::readResponse() {
	Serial.print('#');

	int packetSize = network.parsePacket();

	// if no packet was received, retry later
	if (packetSize) {
		Serial.println("recieved discovery request");

		// read response
		network.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
		String response = String(packetBuffer);

		// other services might respond on this port too
		// the message has to match the ACTS connect reponse
		if (response.startsWith(responsePacketHeader)) {
			// for now we'll just ignore any headers
			// the responding requested is our server
			return network.remoteIP();
		}
	}

	retires++;

	// resend request after 2500ms
	if (retires == 10) {
		retires = 0;

		sendRequest();
	}

	delay(250);
	return readResponse();
}

IPAddress Discovery::find() {
	// start listening for responses
	network.begin(DISCOVERY_RESPONSE_PORT);

	sendRequest();

	return readResponse();
};