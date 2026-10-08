#define NAUI_DIALOG_KEY_MAX 64
#define NAUI_DIALOG_TITLE_MAX 128
#define NAUI_DIALOG_LABEL_MAX 64
#define NAUI_DIALOG_MAX_FILTERS 8
#define NAUI_DIALOG_FILTER_MAX 16

typedef struct Naui_Dialog Naui_Dialog;

typedef enum
{
	NAUI_DIALOG_OPEN_FILE,
	NAUI_DIALOG_SAVE_FILE,
	NAUI_DIALOG_OPEN_FOLDER
} Naui_DialogMode;

typedef uint32_t Naui_DialogState;
enum
{
	NAUI_DIALOG_STATE_OPEN = 1 << 0,
	NAUI_DIALOG_STATE_EDITING = 1 << 1
};

typedef struct
{
	const Naui_Path *paths;
	uint32_t count;
	Naui_Path directory;
} Naui_DialogResult;

typedef void (*Naui_DialogCallback)(const Naui_DialogResult *result, void *user_data, bool is_cancelled);

typedef struct
{
	bool (*begin)(Naui_Dialog *dialog);
	void (*end)(Naui_Dialog *dialog);
} Naui_DialogRenderer;

struct Naui_Dialog
{
	Naui_DialogMode mode;
	Naui_DialogState state;

	char key[NAUI_DIALOG_KEY_MAX];
	char title[NAUI_DIALOG_TITLE_MAX];
	char confirm_label[NAUI_DIALOG_LABEL_MAX];
	char filters[NAUI_DIALOG_MAX_FILTERS][NAUI_DIALOG_FILTER_MAX];
	uint32_t filter_count;

	Naui_Path current_dir;
	Naui_String path_text;
	Naui_String name;
	bool show_hidden;

	Naui_List(Naui_DirEntry) entries;
	int32_t selected_index;

	Naui_DialogCallback callback;
	void *user_data;
	bool _in_use;
};

typedef enum
{
	NAUI_DIALOG_CONFIRM_DONE,
	NAUI_DIALOG_CONFIRM_NAVIGATED,
	NAUI_DIALOG_CONFIRM_NEEDS_OVERWRITE,
	NAUI_DIALOG_CONFIRM_NOTHING_SELECTED,
	NAUI_DIALOG_CONFIRM_INVALID_NAME,
	NAUI_DIALOG_CONFIRM_NOT_FOUND
} Naui_DialogConfirmResult;

bool naui_dialog_open_file(const char *key, const char **filters, Naui_DialogCallback callback, void *user_data);
bool naui_dialog_save_file(const char *key, const char **filters, Naui_DialogCallback callback, void *user_data);
bool naui_dialog_open_folder(const char *key, const char **filters, Naui_DialogCallback callback, void *user_data);

Naui_Dialog *naui_dialog_find(const char *key);
void naui_dialog_confirm_label_set(const char *key, const char *confirm_label);
void naui_dialog_title_set(const char *key, const char *title);
void naui_dialog_name_set(const char *key, const char *name);

// Where the NEXT dialog starts, overriding the remembered folder. Ignored if the folder doesn't exist.
bool naui_dialog_default_path_set(const Naui_Path path);

void naui_dialog_set_renderer(const Naui_DialogRenderer *renderer);

void naui_dialog_refresh(Naui_Dialog *dialog);
bool naui_dialog_navigate(Naui_Dialog *dialog, const Naui_Path folder);
bool naui_dialog_navigate_up(Naui_Dialog *dialog);
void naui_dialog_set_show_hidden(Naui_Dialog *dialog, bool show_hidden);

void naui_dialog_select(Naui_Dialog *dialog, int32_t entry_index);
Naui_DialogConfirmResult naui_dialog_activate(Naui_Dialog *dialog, int32_t entry_index);

bool naui_dialog_commit_path_text(Naui_Dialog *dialog);

Naui_DialogConfirmResult naui_dialog_confirm(Naui_Dialog *dialog, bool allow_overwrite);
void naui_dialog_cancel(Naui_Dialog *dialog);

bool naui_dialog_matches_filter(const Naui_Dialog *dialog, const char *file_name);