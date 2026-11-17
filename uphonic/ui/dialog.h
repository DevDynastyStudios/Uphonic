#define UPH_DIALOG_MAX_BUTTONS 3

typedef void (*Uph_DialogCallback)(int32_t button, void *user_data);

typedef struct
{
	const char* title;
	const char* message;
	const char* buttons[UPH_DIALOG_MAX_BUTTONS];
	int32_t default_button;
	int32_t cancel_button;
	Uph_DialogCallback callback;
	void* user_data;
	Naui_String* input;
	const char* input_placeholder;
	const float* progress;
}
Uph_DialogConfig;

bool uph_dialog_open(
	const Uph_DialogConfig* config
);

bool uph_dialog_blocks_input(void);
bool uph_dialog_is_open(void);
