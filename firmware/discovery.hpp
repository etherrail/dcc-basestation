#ifndef DISCOVERY_HEADER
#define DISCOVERY_HEADER

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define UDP_TX_PACKET_MAX_SIZE 128

class Discovery {
    public: 
		Discovery();

        IPAddress find();

    private:
        WiFiUDP network;
        char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
        long retires = 0;
        String requestPacket;
        String responsePacketHeader = "PT connect\n";

        void sendRequest();
        IPAddress readResponse();
};

#endif