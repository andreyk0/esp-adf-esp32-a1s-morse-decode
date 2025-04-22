file(GLOB_RECURSE srcs "*.c")

idf_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS "."
	PRIV_REQUIRES driver audio_board audio_pipeline audio_hal esp_peripherals audio_stream
)
