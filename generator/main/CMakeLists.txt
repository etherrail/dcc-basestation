idf_component_register(
	SRCS "index.cpp"
	SRCS "dcc.cpp"

	PRIV_REQUIRES spi_flash
	PRIV_REQUIRES esp_driver_rmt
	PRIV_REQUIRES esp_driver_gpio
	INCLUDE_DIRS "."
)
