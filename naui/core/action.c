#define NAUI_ACTION_DEFAULT_CAPACITY 64
#define NAUI_ACTION_FATAL(cond, msg) do { if(cond) { fprintf(stderr, "[naui action] %s\n", msg); abort(); } } while(0)

typedef struct
{
	const char* name;
	const Naui_Action* action;
	void* data;
}
Naui_ActionEntry;

typedef struct
{
	const char* name;
	Naui_Action action;
}
Naui_RegisteredAction;

typedef struct
{
	Naui_List(Naui_RegisteredAction) registered;
	Naui_List(Naui_ActionEntry) undo_entries;
	Naui_List(Naui_ActionEntry) redo_entries;
	Naui_List(Naui_ActionEntry) group_entries;
	size_t capacity;
	uint64_t revision;
	const char* group_name;
	bool group_active;
}
Naui_ActionState;
static Naui_ActionState _naui_action_state = { .capacity = NAUI_ACTION_DEFAULT_CAPACITY };

void __naui_register_action(const char* name, Naui_Action action)
{
	Naui_ActionState* state = &_naui_action_state;
	Naui_RegisteredAction registered = { name, action };
	naui_list_push(state->registered, registered);
}

bool __naui_action_execute_stack(const char* name, const void* data, size_t size)
{
	void* owned = malloc(size);
	memcpy(owned, data, size);

	if (!naui_action_execute(name, owned))
	{
		free(owned);
		return false;
	}

	return true;
}

static const Naui_Action* find_action(const char* name)
{
	const Naui_ActionState* state = &_naui_action_state;
	for(ptrdiff_t i = 0; i < naui_list_len(state->registered); i++)
	{
		if(strcmp(state->registered[i].name, name) == 0)
			return &state->registered[i].action;
	}

	return NULL;
}

static void destroy_entry(Naui_ActionEntry* entry)
{
	if(entry->action->destroy)
		entry->action->destroy(entry->data);

	free(entry->data);
}

static void destroy_entries(Naui_List(Naui_ActionEntry) entries)
{
	for(ptrdiff_t i = 0; i < naui_list_len(entries); i++)
		destroy_entry(&entries[i]);
}

typedef struct
{
	Naui_List(Naui_ActionEntry) actions;
}
Naui_GroupData;

static bool group_undo(void* data)
{
	Naui_GroupData* group = (Naui_GroupData*)data;
	for(ptrdiff_t i = naui_list_len(group->actions); i > 0; i--)
	{
		Naui_ActionEntry* entry = &group->actions[i - 1];
		if(!entry->action->undo(entry->data))
			return false;
	}

	return true;
}

static bool group_redo(void* data)
{
	Naui_GroupData* group = (Naui_GroupData*)data;
	for(ptrdiff_t i = 0; i < naui_list_len(group->actions); i++)
	{
		Naui_ActionEntry* entry = &group->actions[i];
		Naui_ActionFn fn = entry->action->redo ? entry->action->redo : entry->action->execute;
		if(!fn(entry->data))
			return false;
	}

	return true;
}

static void group_destroy(void* data)
{
	Naui_GroupData* group = (Naui_GroupData*)data;
	destroy_entries(group->actions);
	naui_list_free(group->actions);
}

static const Naui_Action _naui_group_action = {
	.execute = NULL,
	.undo = group_undo,
	.redo = group_redo,
	.destroy = group_destroy
};

uint64_t naui_action_get_revision(void)
{
	return _naui_action_state.revision;
}

static void trim_oldest(Naui_List(Naui_ActionEntry) entries, size_t capacity)
{
	while((size_t)naui_list_len(entries) > capacity)
	{
		destroy_entry(&entries[0]);
		naui_list_remove(entries, 0);
	}
}

void naui_action_set_history_capacity(size_t capacity)
{
	Naui_ActionState* state = &_naui_action_state;
	state->capacity = capacity ? capacity : 1;
	trim_oldest(state->undo_entries, state->capacity);
	trim_oldest(state->redo_entries, state->capacity);
}

size_t naui_action_get_history_capacity(void)
{
	return _naui_action_state.capacity;
}

static void history_push(Naui_ActionEntry entry)
{
	Naui_ActionState* state = &_naui_action_state;
	destroy_entries(state->redo_entries);
	naui_list_clear(state->redo_entries);

	trim_oldest(state->undo_entries, state->capacity - 1);
	naui_list_push(state->undo_entries, entry);
}

void naui_action_group_start(const char* name)
{
	Naui_ActionState* state = &_naui_action_state;
	NAUI_ACTION_FATAL(state->group_active, "naui_action_group_start called while a group is already active. Nested groups aren't supported.");
	state->group_active = true;
	state->group_name = name;
}

bool naui_action_group_end(void)
{
	Naui_ActionState* state = &_naui_action_state;
	NAUI_ACTION_FATAL(!state->group_active, "naui_action_group_end called with no active group.");
	state->group_active = false;
	if(naui_list_len(state->group_entries) == 0)
		return false;

	Naui_GroupData* group_data = (Naui_GroupData*)malloc(sizeof(Naui_GroupData));
	group_data->actions = state->group_entries;
	state->group_entries = NULL;
	history_push((Naui_ActionEntry){ .name = state->group_name, .action = &_naui_group_action, .data = group_data });
	return true;
}

bool naui_action_execute(const char* name, void* data)
{
	Naui_ActionState* state = &_naui_action_state;
	const Naui_Action* action = find_action(name);
	if(!action || !action->execute(data))
		return false;

	state->revision++;
	Naui_ActionEntry entry = { .name = name, .action = action, .data = data };
	if(state->group_active)
		naui_list_push(state->group_entries, entry);
	else
		history_push(entry);

	return true;
}

bool naui_action_undo(void)
{
	Naui_ActionState* state = &_naui_action_state;
	if(naui_list_len(state->undo_entries) == 0)
		return false;

	Naui_ActionEntry entry = state->undo_entries[naui_list_len(state->undo_entries) - 1];
	if(!entry.action->undo(entry.data))
		return false;

	state->revision++;
	naui_list_pop(state->undo_entries);
	naui_list_push(state->redo_entries, entry);
	return true;
}

bool naui_action_redo(void)
{
	Naui_ActionState* state = &_naui_action_state;
	if(naui_list_len(state->redo_entries) == 0)
		return false;

	Naui_ActionEntry entry = state->redo_entries[naui_list_len(state->redo_entries) - 1];
	Naui_ActionFn fn = entry.action->redo ? entry.action->redo : entry.action->execute;
	if(!fn(entry.data))
		return false;

	state->revision++;
	naui_list_pop(state->redo_entries);
	naui_list_push(state->undo_entries, entry);
	return true;
}

bool naui_action_can_undo(void)
{
	return naui_list_len(_naui_action_state.undo_entries) > 0;
}

bool naui_action_can_redo(void)
{
	return naui_list_len(_naui_action_state.redo_entries) > 0;
}

static Naui_List(Naui_Action) history_newest_first(Naui_List(Naui_ActionEntry) entries)
{
	Naui_List(Naui_Action) result = NULL;
	naui_list_reserve(result, naui_list_len(entries));

	for(ptrdiff_t i = naui_list_len(entries); i > 0; i--)
	{
		Naui_Action item = *entries[i - 1].action;
		item.name = naui_string_from_cstr(entries[i - 1].name);
		naui_list_push(result, item);
	}

	return result;
}

const Naui_List(Naui_Action) naui_action_get_undo_history(void)
{
	return history_newest_first(_naui_action_state.undo_entries);
}

const Naui_List(Naui_Action) naui_action_get_redo_history(void)
{
	return history_newest_first(_naui_action_state.redo_entries);
}

void naui_action_clear_history(void)
{
	Naui_ActionState* state = &_naui_action_state;
	destroy_entries(state->undo_entries);
	destroy_entries(state->redo_entries);
	naui_list_clear(state->undo_entries);
	naui_list_clear(state->redo_entries);
}
