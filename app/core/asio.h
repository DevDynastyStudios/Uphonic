#pragma once

#define UPH_ASIO_MAX_DRIVERS 32

typedef enum
{
	UPH_AUDIO_DRIVER_SYSTEM,
	UPH_AUDIO_DRIVER_ASIO
}
Uph_AudioDriver;

bool uph_asio_supported(void);
uint32_t uph_asio_list_drivers(Naui_String *names);
void uph_asio_get_backend_callbacks(ma_backend_callbacks *callbacks);
bool uph_asio_show_control_panel(void);
bool uph_asio_consume_reset_request(void);
uint32_t uph_asio_input_count(void);
