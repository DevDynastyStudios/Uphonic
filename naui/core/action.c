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

typedef struct Naui_ActionStageNode Naui_ActionStageNode;

typedef struct
{
	Naui_RegisteredAction* registered;
	size_t registered_count;
	size_t registered_capacity;
	Naui_Arena ring_arena;
	Naui_ActionEntry* undo_entries;
	Naui_ActionEntry* redo_entries;
	size_t capacity;
	size_t undo_head;
	size_t undo_count;
	size_t redo_count;
	uint64_t revision;
	Naui_Arena group_arena;
	Naui_ActionStageNode* group_head;
	Naui_ActionStageNode* group_tail;
	size_t group_count;
	const char* group_name;
	bool group_active;
}
Naui_ActionData;
static Naui_ActionData _naui_action_data;

void __naui_register_action(const char* name, Naui_Action action)
{
	if(_naui_action_data.registered_count == _naui_action_data.registered_capacity)
	{
		_naui_action_data.registered_capacity = _naui_action_data.registered_capacity ? _naui_action_data.registered_capacity * 2 : 16;
		_naui_action_data.registered = (Naui_RegisteredAction*)realloc(_naui_action_data.registered, _naui_action_data.registered_capacity * sizeof(Naui_RegisteredAction));
	}

	_naui_action_data.registered[_naui_action_data.registered_count++] = (Naui_RegisteredAction){ name, action };
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
	for(size_t i = 0; i < _naui_action_data.registered_count; i++)
	{
		if(strcmp(_naui_action_data.registered[i].name, name) == 0)
			return &_naui_action_data.registered[i].action;
	}

	return NULL;
}

typedef struct
{
	Naui_ActionEntry* actions;
	size_t count;
}
Naui_GroupData;

static void destroy_entry(Naui_ActionEntry* entry)
{
	if(entry->action->destroy)
		entry->action->destroy(entry->data);

	free(entry->data);
}

static bool group_undo(void* data)
{
	Naui_GroupData* group = (Naui_GroupData*)data;
	for(size_t i = group->count; i > 0; i--)
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
	for(size_t i = 0; i < group->count; i++)
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
	for(size_t i = 0; i < group->count; i++)
	{
		destroy_entry(&group->actions[i]);
	}

	free(group->actions);
}

static const Naui_Action _naui_group_action = {
	.execute = NULL,
	.undo = group_undo,
	.redo = group_redo,
	.destroy = group_destroy
};

uint64_t naui_action_get_revision(void)
{
	return _naui_action_data.revision;
}

static inline size_t undo_physical_index(size_t logical_index)
{
	return (_naui_action_data.undo_head + logical_index) % _naui_action_data.capacity;
}

static void ensure_initialized(void)
{
	if(_naui_action_data.capacity == 0)
		naui_action_set_history_capacity(NAUI_ACTION_DEFAULT_CAPACITY);
}

void naui_action_set_history_capacity(size_t capacity)
{
	if(capacity == 0)
		capacity = 1;

	while(_naui_action_data.undo_count > capacity)
	{
		destroy_entry(&_naui_action_data.undo_entries[_naui_action_data.undo_head]);
		_naui_action_data.undo_head = (_naui_action_data.undo_head + 1) % _naui_action_data.capacity;
		_naui_action_data.undo_count--;
	}

	while(_naui_action_data.redo_count > capacity)
	{
		destroy_entry(&_naui_action_data.redo_entries[0]);
		memmove(&_naui_action_data.redo_entries[0], &_naui_action_data.redo_entries[1], (_naui_action_data.redo_count - 1) * sizeof(Naui_ActionEntry));
		_naui_action_data.redo_count--;
	}

	size_t old_undo_count = _naui_action_data.undo_count;
	size_t old_redo_count = _naui_action_data.redo_count;
	Naui_ActionEntry* undo_tmp = old_undo_count ? (Naui_ActionEntry*)malloc(old_undo_count * sizeof(Naui_ActionEntry)) : NULL;
	for(size_t i = 0; i < old_undo_count; i++)
	{
		undo_tmp[i] = _naui_action_data.undo_entries[undo_physical_index(i)];
	}

	Naui_ActionEntry* redo_tmp = old_redo_count ? (Naui_ActionEntry*)malloc(old_redo_count * sizeof(Naui_ActionEntry)) : NULL;
	if(redo_tmp)
		memcpy(redo_tmp, _naui_action_data.redo_entries, old_redo_count * sizeof(Naui_ActionEntry));

	naui_arena_reset(&_naui_action_data.ring_arena);
	_naui_action_data.undo_entries = (Naui_ActionEntry*)naui_arena_alloc(&_naui_action_data.ring_arena, capacity * sizeof(Naui_ActionEntry));
	_naui_action_data.redo_entries = (Naui_ActionEntry*)naui_arena_alloc(&_naui_action_data.ring_arena, capacity * sizeof(Naui_ActionEntry));

	if(undo_tmp)
	{
		memcpy(_naui_action_data.undo_entries, undo_tmp, old_undo_count * sizeof(Naui_ActionEntry));
		free(undo_tmp);
	}

	if(redo_tmp)
	{
		memcpy(_naui_action_data.redo_entries, redo_tmp, old_redo_count * sizeof(Naui_ActionEntry));
		free(redo_tmp);
	}

	_naui_action_data.capacity = capacity;
	_naui_action_data.undo_head = 0;
	_naui_action_data.undo_count = old_undo_count;
	_naui_action_data.redo_count = old_redo_count;
}

size_t naui_action_get_history_capacity(void)
{
	ensure_initialized();
	return _naui_action_data.capacity;
}

static void history_push(Naui_ActionEntry entry)
{
	ensure_initialized();
	for(size_t i = 0; i < _naui_action_data.redo_count; i++)
	{
		destroy_entry(&_naui_action_data.redo_entries[i]);
	}

	_naui_action_data.redo_count = 0;

	if(_naui_action_data.undo_count == _naui_action_data.capacity)
	{
		destroy_entry(&_naui_action_data.undo_entries[_naui_action_data.undo_head]);
		_naui_action_data.undo_head = (_naui_action_data.undo_head + 1) % _naui_action_data.capacity;
		_naui_action_data.undo_count--;
	}

	_naui_action_data.undo_entries[undo_physical_index(_naui_action_data.undo_count)] = entry;
	_naui_action_data.undo_count++;
}

struct Naui_ActionStageNode
{
	Naui_ActionEntry entry;
	Naui_ActionStageNode* next;
};

void naui_action_group_start(const char* name)
{
	NAUI_ACTION_FATAL(_naui_action_data.group_active, "naui_action_group_start called while a group is already active. Nested groups aren't supported.");
	_naui_action_data.group_active = true;
	_naui_action_data.group_name = name;
	_naui_action_data.group_head = NULL;
	_naui_action_data.group_tail = NULL;
	_naui_action_data.group_count = 0;
}

bool naui_action_group_end(void)
{
	NAUI_ACTION_FATAL(!_naui_action_data.group_active, "naui_action_group_end called with no active group.");
	_naui_action_data.group_active = false;
	if(_naui_action_data.group_count == 0)
	{
		naui_arena_reset(&_naui_action_data.group_arena);
		return false;
	}

	Naui_ActionEntry* actions = (Naui_ActionEntry*)malloc(_naui_action_data.group_count * sizeof(Naui_ActionEntry));
	size_t i = 0;
	for(Naui_ActionStageNode* node = _naui_action_data.group_head; node; node = node->next)
	{
		actions[i++] = node->entry;
	}

	naui_arena_reset(&_naui_action_data.group_arena);
	Naui_GroupData* group_data = (Naui_GroupData*)malloc(sizeof(Naui_GroupData));
	*group_data = (Naui_GroupData){ .actions = actions, .count = i };
	history_push((Naui_ActionEntry){ .name = _naui_action_data.group_name, .action = &_naui_group_action, .data = group_data });
	return true;
}

bool naui_action_execute(const char* name, void* data)
{
	ensure_initialized();
	const Naui_Action* action = find_action(name);
	if(!action || !action->execute(data))
		return false;

	_naui_action_data.revision++;
	Naui_ActionEntry entry = { .name = name, .action = action, .data = data };
	if(_naui_action_data.group_active)
	{
		Naui_ActionStageNode* node = (Naui_ActionStageNode*)naui_arena_alloc(&_naui_action_data.group_arena, sizeof(Naui_ActionStageNode));
		node->entry = entry;
		node->next = NULL;

		if(_naui_action_data.group_tail)
			_naui_action_data.group_tail->next = node;
		else
			_naui_action_data.group_head = node;

		_naui_action_data.group_tail = node;
		_naui_action_data.group_count++;
	}
	else
	{
		history_push(entry);
	}

	return true;
}

bool naui_action_undo(void)
{
	ensure_initialized();
	if(_naui_action_data.undo_count == 0)
		return false;

	Naui_ActionEntry* entry = &_naui_action_data.undo_entries[undo_physical_index(_naui_action_data.undo_count - 1)];
	if(!entry->action->undo(entry->data))
		return false;

	_naui_action_data.revision++;
	_naui_action_data.redo_entries[_naui_action_data.redo_count++] = *entry;
	_naui_action_data.undo_count--;
	return true;
}

bool naui_action_redo(void)
{
	ensure_initialized();
	if(_naui_action_data.redo_count == 0)
		return false;

	Naui_ActionEntry* entry = &_naui_action_data.redo_entries[_naui_action_data.redo_count - 1];
	Naui_ActionFn fn = entry->action->redo ? entry->action->redo : entry->action->execute;
	if(!fn(entry->data))
		return false;

	_naui_action_data.revision++;
	_naui_action_data.undo_entries[undo_physical_index(_naui_action_data.undo_count)] = *entry;
	_naui_action_data.undo_count++;
	_naui_action_data.redo_count--;
	return true;
}

bool naui_action_can_undo(void)
{
	ensure_initialized();
	return _naui_action_data.undo_count > 0;
}

bool naui_action_can_redo(void)
{
	ensure_initialized();
	return _naui_action_data.redo_count > 0;
}

const Naui_List(Naui_Action) naui_action_get_undo_history(void)
{
	ensure_initialized();
	Naui_List(Naui_Action) result = NULL;
	naui_list_reserve(result, _naui_action_data.undo_count);

	for(size_t i = _naui_action_data.undo_count; i > 0; i--)
	{
		const Naui_ActionEntry* entry = &_naui_action_data.undo_entries[undo_physical_index(i - 1)];
		Naui_Action item = *entry->action;
		item.name = naui_string_from_cstr(entry->name);
		naui_list_push(result, item);
	}

	return result;
}

const Naui_List(Naui_Action) naui_action_get_redo_history(void)
{
	ensure_initialized();
	Naui_List(Naui_Action) result = NULL;
	naui_list_reserve(result, _naui_action_data.redo_count);

	for(size_t i = _naui_action_data.redo_count; i > 0; i--)
	{
		const Naui_ActionEntry* entry = &_naui_action_data.redo_entries[i - 1];
		Naui_Action item = *entry->action;
		item.name = naui_string_from_cstr(entry->name);
		naui_list_push(result, item);
	}

	return result;
}

void naui_action_clear_history(void)
{
	ensure_initialized();
	for(size_t i = 0; i < _naui_action_data.undo_count; i++)
	{
		destroy_entry(&_naui_action_data.undo_entries[undo_physical_index(i)]);
	}

	for(size_t i = 0; i < _naui_action_data.redo_count; i++)
	{
		destroy_entry(&_naui_action_data.redo_entries[i]);
	}

	_naui_action_data.undo_head = 0;
	_naui_action_data.undo_count = 0;
	_naui_action_data.redo_count = 0;
}