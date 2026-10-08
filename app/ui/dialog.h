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
}
Uph_DialogConfig;

// False if one is already open or the config has no buttons
bool uph_dialog_open(const Uph_DialogConfig* config);
bool uph_dialog_is_open(void);

// Returns if it's blocking UI inputs
bool uph_dialog_blocks_input(void);
void uph_dialog_render(void);