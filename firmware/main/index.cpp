#pragma once

#include "esp_log.h"
#include "esp_system.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/gpio_types.h"
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

	vTaskDelay(1000 / portTICK_PERIOD_MS);

	gpio_num_t faultPin = GPIO_NUM_21;

	gpio_config_t faultPinInput = {};
	faultPinInput.pin_bit_mask = 1ULL << faultPin;
	faultPinInput.mode = GPIO_MODE_INPUT;
	faultPinInput.pull_up_en = GPIO_PULLUP_ENABLE;
	faultPinInput.pull_down_en = GPIO_PULLDOWN_DISABLE;
	faultPinInput.intr_type = GPIO_INTR_DISABLE;

	gpio_config(&faultPinInput);

	adc_oneshot_unit_handle_t currentSensingHandle;
	auto currentSensingUnit = ADC_UNIT_1;
	auto currentSensingChannel = ADC_CHANNEL_6;

	adc_oneshot_unit_init_cfg_t currentConfiguration = {};
	currentConfiguration.unit_id = currentSensingUnit;

	adc_oneshot_new_unit(&currentConfiguration, &currentSensingHandle);

	adc_oneshot_chan_cfg_t oneshotConfiguration = {};
	oneshotConfiguration.atten = ADC_ATTEN_DB_12; // 0–3.3V
	oneshotConfiguration.bitwidth = ADC_BITWIDTH_12; // 0–4095

	adc_oneshot_config_channel(currentSensingHandle, currentSensingChannel, &oneshotConfiguration);

	int currentSense;

	track.prepareSignalGenerator();

	while (1) {
		DCCPacket packet = protocol.speed(3, 0.5f, FORWARD);
		track.writePacket(packet);

		int level = gpio_get_level(faultPin);
		adc_oneshot_read(currentSensingHandle, currentSensingChannel, &currentSense);

		ESP_LOGI(TAG, "wrote packet %d / %d", level, currentSense);
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
