#include "esp_log.h"
#include <stdio.h>

extern "C" {
	#include "nvs_flash.h"

	void app_main();
}

#define TAG "MAIN"

#include "network.cpp"
#include "identifier.cpp"

void app_main(void) {
	ESP_ERROR_CHECK(nvs_flash_init());

	char* identifier = Identifier::read("etherrail-dcc-basestation");
	ESP_LOGI(TAG, "Identifier: %s", identifier);

	network.begin();
}
