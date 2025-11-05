#ifndef CONNECTION_HEADER
#define CONNECTION_HEADER

#include "discovery.hpp"

#include <Arduino.h>
#include <WiFi.h>

using ConnectionLoopHandler = void (*)(WiFiClient);

class Connection {
    public: 
		void begin(ConnectionLoopHandler connectionLoopHandler);

    private:
		IPAddress host;
        WiFiClient client;
		Discovery discovery;

		char reconnectionAttempts;
		ConnectionLoopHandler connectionLoopHandler;

		void findHost();
		void connect();
};

#endif