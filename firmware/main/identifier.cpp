#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "esp_mac.h"
#include "esp_err.h"

class Identifier {
	public:
		static char* read(const char* prefix) {
			uint8_t mac[6];
			ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_ETH));

			char* identifier = new char[strlen(prefix) + 6 * 2 + 1];
			sprintf(identifier, "%s-%02x%02x%02x%02x%02x%02x", prefix, mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

			return identifier;
		}
};
