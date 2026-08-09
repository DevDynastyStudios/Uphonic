typedef uint32_t Naui_ShortcutCtx;
typedef void (*Naui_ShortcutFn)(void *user_data);

typedef uint8_t Naui_ShortcutKind;
enum
{
	NAUI_SHORTCUT_CHORD,
	NAUI_SHORTCUT_SEQUENCE
};

typedef struct
{
	const Naui_Key* items;
	size_t count;
} Naui_KeyList;

typedef struct
{
	const Naui_ShortcutCtx* items;
	size_t count;
} Naui_ShortcutCtxList;

typedef struct
{
	Naui_KeyList keys;
	Naui_ShortcutCtxList contexts;
	Naui_ShortcutFn callback;
	void *user_data;
	float sequence_timeout; // -1 disables timeout. Ignored in NAUI_SHORTCUT_CHORD mode.
} Naui_Shortcut;

#define NAUI_SHORTCUT_CONTEXT_GLOBAL 0
#define NAUI_KEYS(...) ((Naui_KeyList){ (Naui_Key[]){ __VA_ARGS__ }, sizeof((Naui_Key[]){ __VA_ARGS__ }) / sizeof(Naui_Key) })
#define NAUI_CONTEXTS(...) ((Naui_ShortcutCtxList){ (Naui_ShortcutCtx[]){ __VA_ARGS__ }, sizeof((Naui_ShortcutCtx[]){ __VA_ARGS__ }) / sizeof(Naui_ShortcutCtx) })

typedef struct { Naui_Shortcut wrapped; } Naui_ShortcutWrapper;
#define naui_register_shortcut(name, ...) __naui_register_shortcut((name), (Naui_ShortcutWrapper){ __VA_ARGS__ }.wrapped)

void __naui_register_shortcut(Naui_StringView name, Naui_Shortcut shortcut);
void naui_unregister_shortcut(Naui_StringView name);

void naui_shortcut_init(Naui_ShortcutKind kind);

// Polls every registered shortcut against current key state
void naui_shortcut_update(void);

void naui_shortcut_context_enable(Naui_ShortcutCtx context);
void naui_shortcut_context_disable(Naui_ShortcutCtx context);
bool naui_shortcut_context_active(Naui_ShortcutCtx context);

// TODO: Add repeat key support, mg_app doesn't support it :(
