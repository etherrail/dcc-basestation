#include <string>
#include <cstring>

#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"

#define DISCOVERY_REQUEST_PORT 142
#define DISCOVERY_RESPONSE_PORT 143
#define DISCOVERY_MAX_PACKET_LEN 512

#define TAG "DISCOVERY"

class Discovery {
	public:
		Discovery(char* identifier) {
			requestPacket = new char[DISCOVERY_MAX_PACKET_LEN];
			sprintf(requestPacket, "PT login\ndevice: %s\n\n", identifier);
		}

		~Discovery() {
			if (handle != -1) {
				closeSocket();
			}
		}

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

	private:
		uint8_t retries = 0;
		char* requestPacket;

		int handle = -1;

		bool openSocket() {
			if (handle != -1) {
				return true;
			}

			handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

			if (handle < 0) {
				ESP_LOGE(TAG, "could not open socket: %d", errno);

				return false;
			}

			int broadcast = 1;

			if (setsockopt(handle, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
				ESP_LOGE(TAG, "could not enable broadcast: %d", errno);
				closeSocket();

				return false;
			}

			struct sockaddr_in receivingAddress{};
			receivingAddress.sin_family = AF_INET;
			receivingAddress.sin_addr.s_addr = htonl(INADDR_ANY);
			receivingAddress.sin_port = htons(DISCOVERY_RESPONSE_PORT);

			if (bind(handle, (struct sockaddr *)&receivingAddress, sizeof(receivingAddress)) < 0) {
				ESP_LOGE(TAG, "could not open receiving port: %d", errno);
				closeSocket();

				return false;
			}

			return true;
		}

		void closeSocket() {
			close(handle);
			handle = -1;
		}
};
