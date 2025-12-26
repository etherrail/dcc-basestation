#pragma once

#include "esp_log.h"
#include "esp_system.h"
#include <stdio.h>

extern "C" {
	#include "nvs_flash.h"

	void app_main();
}

#include "network.cpp"
#include "identifier.cpp"
#include "discovery.cpp"
#include "dcc.cpp"
#include "track.cpp"

#define TAG "MAIN"

char* identifier;

void start(void* arguments);
void monitorMemory(void *arguments);

void app_main(void) {
	ESP_ERROR_CHECK(nvs_flash_init());

	identifier = Identifier::read("etherrail-dcc-basestation");
	ESP_LOGI(TAG, "identifier: %s", identifier);

	/// xTaskCreate(monitorMemory, "memory-monitor", 1024, NULL, 1, NULL);
	/// ESP_LOGI(TAG, "start memory watcher");

	// protocol message generator
	DCCProtocol protocol(14);

	// main track
	Track track(GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_1);
	track.prepareSignalGenerator();

	vTaskDelay(1000 / portTICK_PERIOD_MS);

	while (1) {
		DCCPacket packet = protocol.speed(4, 0.02f, FORWARD);
		track.writePacket(packet);

		ESP_LOGI(TAG, "wrote packet");
	}

	/// network.begin();

	// must run in new task, message parsing uses too much memory for the main task
	/// xTaskCreate(start, "start", 2048, NULL, 1, NULL);
}

void start(void* arguments) {
	Discovery discovery(identifier);
	ip4_addr_t directorAddress = discovery.find();
	ESP_LOGI(TAG, "director: %s", inet_ntoa(directorAddress));
}

void monitorMemory(void* arguments) {
	while (1) {
		/// ESP_LOGI(TAG, "memory: %u", esp_get_free_heap_size());

		vTaskDelay(10000);
	}
}
