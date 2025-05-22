file(GLOB_RECURSE srcs "*.c")

idf_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS "."

	PRIV_REQUIRES
	  audio_pipeline
	  audio_stream
	  driver
	  esp-dsp
	  esp_peripherals
	  u8g2
	  u8g2-hal-esp-idf
)
