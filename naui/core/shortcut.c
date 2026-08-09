#define NAUI_SHORTCUT_FATAL_IF(cond, msg) do { if(cond) { fprintf(stderr, "[naui shortcut] %s\n", msg); abort(); } } while(0)

typedef struct
{
	Naui_StringView name;
	Naui_List(Naui_Key) keys;
	Naui_List(Naui_ShortcutCtx) contexts;

	float sequence_timeout;
	Naui_ShortcutFn callback;
	void *user_data;

	bool armed;
	size_t step;
	float last_step_time;
}
Naui_RegisteredShortcut;

static Naui_List(Naui_RegisteredShortcut) s_shortcuts;
static bool s_initialized = false;
static Naui_ShortcutKind s_kind;
static Naui_List(Naui_ShortcutCtx) s_active_contexts;

bool naui_shortcut_context_active(Naui_ShortcutCtx context)
{
	if(context == NAUI_SHORTCUT_CONTEXT_GLOBAL)
		return true;

	for(size_t i = 0; i < naui_list_len(s_active_contexts); i++)
	{
		if(s_active_contexts[i] == context)
			return true;
	}

	return false;
}

void naui_shortcut_context_enable(Naui_ShortcutCtx context)
{
	if(naui_shortcut_context_active(context))
		return;

	naui_list_push(s_active_contexts, context);
}

void naui_shortcut_context_disable(Naui_ShortcutCtx context)
{
	for(size_t i = 0; i < naui_list_len(s_active_contexts); i++)
	{
		if(s_active_contexts[i] == context)
		{
			naui_list_uremove(s_active_contexts, i);
			return;
		}
	}
}

static bool shortcut_context_eligible(Naui_RegisteredShortcut *s)
{
	if(naui_list_len(s->contexts) == 0)
		return true;

	for(size_t i = 0; i < naui_list_len(s->contexts); i++)
	{
		if(naui_shortcut_context_active(s->contexts[i]))
			return true;
	}

	return false;
}

static void free_registered_shortcut(Naui_RegisteredShortcut *s)
{
	naui_list_free(s->keys);
	naui_list_free(s->contexts);
}

void naui_shortcut_init(Naui_ShortcutKind kind)
{
	for(size_t i = 0; i < naui_list_len(s_shortcuts); i++)
	{
		free_registered_shortcut(&s_shortcuts[i]);
	}

	naui_list_clear(s_shortcuts);
	s_kind = kind;
	s_initialized = true;
}

void __naui_register_shortcut(Naui_StringView name, Naui_Shortcut shortcut)
{
	NAUI_SHORTCUT_FATAL_IF(!s_initialized, "naui_register_shortcut called before naui_shortcut_init -- the system doesn't know which mode to validate against.");
	NAUI_SHORTCUT_FATAL_IF(shortcut.keys.count == 0, "No keys were registered. A shortcut needs at least one key.");

	if(s_kind == NAUI_SHORTCUT_CHORD)
	{
		NAUI_SHORTCUT_FATAL_IF(shortcut.sequence_timeout != 0.0f && shortcut.sequence_timeout != -1.0f,
			"sequence_timeout was set on a shortcut registered while the system is in CHORD mode -- "
			"this field only applies to SEQUENCE mode and setting it here almost certainly means the "
			"wrong mode is active, or this registration belongs in a different part of the codebase.");
	}

	Naui_RegisteredShortcut entry = {
		.name = name,
		.keys = NULL,
		.contexts = NULL,
		.sequence_timeout = shortcut.sequence_timeout,
		.callback = shortcut.callback,
		.user_data = shortcut.user_data,
		.armed = true,
		.step = 0,
		.last_step_time = 0.0f
	};

	naui_list_reserve(entry.keys, shortcut.keys.count);
	for(size_t i = 0; i < shortcut.keys.count; i++)
	{
		naui_list_push(entry.keys, shortcut.keys.items[i]);
	}

	naui_list_reserve(entry.contexts, shortcut.contexts.count);
	for(size_t i = 0; i < shortcut.contexts.count; i++)
	{
		naui_list_push(entry.contexts, shortcut.contexts.items[i]);
	}

	naui_list_push(s_shortcuts, entry);
}

void naui_unregister_shortcut(Naui_StringView name)
{
	for(size_t i = 0; i < naui_list_len(s_shortcuts); i++)
	{
		if(naui_string_eq(s_shortcuts[i].name, name, true))
		{
			free_registered_shortcut(&s_shortcuts[i]);
			naui_list_uremove(s_shortcuts, i);
			return;
		}
	}
}

static void update_chord(Naui_RegisteredShortcut *s)
{
	bool all_down = true;
	bool any_fresh_press = false;
	for(size_t i = 0; i < naui_list_len(s->keys); i++)
	{
		Naui_Key key = s->keys[i];
		if(!naui_key_down(key))
			all_down = false;

		if(naui_key_pressed(key))
			any_fresh_press = true;
	}

	if(!all_down)
	{
		s->armed = true;
		return;
	}

	if(s->armed && any_fresh_press && shortcut_context_eligible(s))
	{
		s->armed = false;
		s->callback(s->user_data);
	}
}

static void reset_sequence(Naui_RegisteredShortcut *s)
{
	s->step = 0;
}

static void update_sequence(Naui_RegisteredShortcut *s)
{
	if(s->sequence_timeout >= 0.0f && s->step > 0)
	{
		if(naui_time() - s->last_step_time > s->sequence_timeout)
			reset_sequence(s);
	}

	if(s->step == 0 && !shortcut_context_eligible(s))
		return;

	Naui_Key expected = s->keys[s->step];
	if(naui_key_pressed(expected))
	{
		s->step++;
		s->last_step_time = naui_time();
		if(s->step == naui_list_len(s->keys))
		{
			reset_sequence(s);
			s->callback(s->user_data);
		}

		return;
	}

	for(size_t i = 0; i < naui_list_len(s->keys); i++)
	{
		if(i != s->step && naui_key_pressed(s->keys[i]))
		{
			reset_sequence(s);
			break;
		}
	}
}

void naui_shortcut_update(void)
{
	if(!s_initialized)
		return;

	if(s_kind == NAUI_SHORTCUT_CHORD)
	{
		for(size_t i = 0; i < naui_list_len(s_shortcuts); i++)
		{
			update_chord(&s_shortcuts[i]);
		}
	}
	else
	{
		for(size_t i = 0; i < naui_list_len(s_shortcuts); i++)
		{
			update_sequence(&s_shortcuts[i]);
		}
	}
}
