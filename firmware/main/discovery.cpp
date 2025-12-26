#pragma once

#include <string>
#include <cstring>

#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"

#include "protocol.cpp"

#define DISCOVERY_REQUEST_PORT 142
#define DISCOVERY_RESPONSE_PORT 143
#define DISCOVERY_MAX_PACKET_LENGTH 512

#define TAG "DISCOVERY"

class Discovery {
	public:
		Discovery(char* identifier) {
			requestPacket = new char[DISCOVERY_MAX_PACKET_LENGTH];
			sprintf(requestPacket, "PT login\ndevice: %s\n\n", identifier);
		}

		~Discovery() {
			if (handle != -1) {
				closeSocket();
			}
		}

		ip4_addr_t find() {
			sendRequest();

			while (true) {
				struct timeval tv{};
				tv.tv_sec = 0;
				tv.tv_usec = 250 * 1000;

				fd_set rfds;
				FD_ZERO(&rfds);
				FD_SET(this->handle, &rfds);

				int s = select(this->handle + 1, &rfds, nullptr, nullptr, &tv);

				if (s < 0) {
					ESP_LOGE(TAG, "reading failed: %d", errno);

					continue;
				}

				if (s == 0) {
					// timeout, no packet
					this->retries++;

					// resend request after 10 * 250ms = 2500ms
					if (this->retries == 10) {
						this->retries = 0;

						sendRequest();
					}

					continue;
				}

				if (!FD_ISSET(this->handle, &rfds)) {
					continue;
				}

				// read data
				struct sockaddr_in sourceAddr{};
				socklen_t addrLen = sizeof(sourceAddr);

				char* buffer = new char[DISCOVERY_MAX_PACKET_LENGTH];

				int len = recvfrom(
					this->handle,
					buffer,
					DISCOVERY_MAX_PACKET_LENGTH - 1,
					0,
					(struct sockaddr *)&sourceAddr,
					&addrLen
				);

				if (len < 0) {
					ESP_LOGE(TAG, "receiving failed, no data: %d", errno);

					continue;
				}

				buffer[len] = '\0';

				if (!verifyResponse(buffer)) {
					ESP_LOGE(TAG, "invalid response packet %s", inet_ntoa(sourceAddr.sin_addr));

					continue;
				}

				ip4_addr_t result;
				result.addr = sourceAddr.sin_addr.s_addr;

				return result;
			}
		}

	private:
		char* requestPacket;
		const char* responsePacket = "PT connect\n";

		int handle = -1;
		uint8_t retries = 0;

		void sendRequest() {
			if (!openSocket()) {
				return;
			}

			ESP_LOGI(TAG, "sending discovery request...");

			struct sockaddr_in request{};
			request.sin_family = AF_INET;
			request.sin_port = htons(DISCOVERY_REQUEST_PORT);
			request.sin_addr.s_addr = htonl(INADDR_BROADCAST);

			if (sendto(
				handle,
				requestPacket,
				strlen(requestPacket),
				0,
				(struct sockaddr *)&request,
				sizeof(request)
			) < 0) {
				ESP_LOGE(TAG, "could not send request: %d", errno);
			}
		}

		bool openSocket() {
			if (this->handle != -1) {
				return true;
			}

			this->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

			if (this->handle < 0) {
				ESP_LOGE(TAG, "could not open socket: %d", errno);

				return false;
			}

			int broadcast = 1;

			if (setsockopt(this->handle, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
				ESP_LOGE(TAG, "could not enable broadcast: %d", errno);
				closeSocket();

				return false;
			}

			struct sockaddr_in receivingAddress{};
			receivingAddress.sin_family = AF_INET;
			receivingAddress.sin_addr.s_addr = htonl(INADDR_ANY);
			receivingAddress.sin_port = htons(DISCOVERY_RESPONSE_PORT);

			if (bind(this->handle, (struct sockaddr *)&receivingAddress, sizeof(receivingAddress)) < 0) {
				ESP_LOGE(TAG, "could not open receiving port: %d", errno);
				closeSocket();

				return false;
			}

			return true;
		}

		bool verifyResponse(const char* response) {
			ESP_LOGI(TAG, "%d '%s'", strlen(response), response);

			Message message = Message::from(response);

			int expectedResponseLength = strlen(responsePacket);

			if (strlen(response) < expectedResponseLength) {
				return false;
			}

			for (int index = 0; index < expectedResponseLength; index++) {
				if (response[index] != responsePacket[index]) {
					return false;
				}
			}

			return true;
		}

		void closeSocket() {
			close(handle);
			handle = -1;
		}
};
