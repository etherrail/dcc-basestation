#include <stdio.h>
#include <cstdint>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "soc/gpio_sig_map.h"
#include "soc/io_mux_reg.h"
#include "esp_rom_gpio.h"

#include "dcc.h"

static const char *tag = "basestation";

const gpio_num_t ENABLE_TRACK_POWER_PIN = GPIO_NUM_1;

const int GENERATOR_FREQUENCY = 1 * 1000 * 1000; // 1us
const int SIGNAL_DURATION_1 = 58;
const int SIGNAL_DURATION_0 = 100;
static rmt_channel_handle_t generatorChannel;
static rmt_encoder_handle_t generatorEncoder;

const gpio_num_t CONTROL_A = GPIO_NUM_32;
const gpio_num_t CONTROL_B = GPIO_NUM_33;

DCCProtocol protocol(
	14 // start packet length (preamble)
);

void enableTrackPower();
void disableTrackPower();

void loop();

extern "C" void app_main(void) {
	ESP_LOGI(tag, "ready");

	gpio_reset_pin(ENABLE_TRACK_POWER_PIN);
	gpio_set_direction(ENABLE_TRACK_POWER_PIN, GPIO_MODE_OUTPUT);

	ESP_LOGI(tag, "gpio");

	// enable track power after setup
	enableTrackPower();

	ESP_LOGI(tag, "enable track power");

	// configure digital output channel
	rmt_tx_channel_config_t configuration = {};
	configuration.gpio_num = CONTROL_A;
	configuration.clk_src = RMT_CLK_SRC_DEFAULT;
	configuration.mem_block_symbols = 128;
	configuration.resolution_hz = GENERATOR_FREQUENCY;
	configuration.trans_queue_depth = 4;

	ESP_ERROR_CHECK(rmt_new_tx_channel(&configuration, &generatorChannel));

	ESP_LOGI(tag, "tx channel");

	rmt_copy_encoder_config_t encoder = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encoder, &generatorEncoder));

	ESP_ERROR_CHECK(rmt_enable(generatorChannel));

	ESP_LOGI(tag, "enable");

	// enable inverted output pin
	ESP_ERROR_CHECK(gpio_set_direction(CONTROL_B, GPIO_MODE_OUTPUT));
	esp_rom_gpio_connect_out_signal(CONTROL_B, RMT_SIG_PAD_OUT0_IDX + 0, true, false);

	ESP_LOGI(tag, "matrix");

	while (1) {
		ESP_LOGI(tag, "loop %u", esp_get_free_heap_size());

		loop();
	}
}

// write packet to tracks
// frees memory after writing
void writePacket(DCCPacket packet) {
	rmt_symbol_word_t *buffer = new rmt_symbol_word_t[packet.length + 1];

	if (!buffer) {
		ESP_LOGE(tag, "dcc sending buffer could not be allocated");

        return;
	}

	size_t index = 0;

	// write bits to buffer
	for (size_t i = 0; i < packet.length; i++) {
		uint16_t slot = packet.bits[i] ? SIGNAL_DURATION_1 : SIGNAL_DURATION_0;

		rmt_symbol_word_t symbol;

		symbol.level0 = 1;
		symbol.duration0 = slot;

		symbol.level1 = 0;
		symbol.duration1 = slot;

		buffer[index++] = symbol;
	}

	delete[] packet.bits;

	rmt_symbol_word_t endSymbol;
	endSymbol.level0 = 0;
	endSymbol.duration0 = 0;
	endSymbol.level1 = 0;
	endSymbol.duration1 = 0;

	buffer[index++] = endSymbol;

	rmt_transmit_config_t configuration = {};
	configuration.loop_count = 0;

	rmt_transmit(
		generatorChannel,
		generatorEncoder,
		buffer,
		index * sizeof(rmt_symbol_word_t),
		&configuration
	);

	rmt_tx_wait_all_done(generatorChannel, -1);

	delete[] buffer;
}


void loop() {
	DCCPacket packet = protocol.speed(4, 0.2f, FORWARD);
	writePacket(packet);

	vTaskDelay(1 / portTICK_PERIOD_MS);
}

void enableTrackPower() {
	gpio_set_level(ENABLE_TRACK_POWER_PIN, 1);

	// prevent too fast switching of the relays in some error state
	vTaskDelay(100 / portTICK_PERIOD_MS);
}

void disableTrackPower() {
	gpio_set_level(ENABLE_TRACK_POWER_PIN, 0);

	// prevent too fast switching of the relays in some error state
	vTaskDelay(2000 / portTICK_PERIOD_MS);
}
