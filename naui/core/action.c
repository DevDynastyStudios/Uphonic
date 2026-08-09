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

static Naui_RegisteredAction* s_registered;
static size_t s_registered_count;
static size_t s_registered_capacity;

void __naui_register_action(const char* name, Naui_Action action)
{
	if(s_registered_count == s_registered_capacity)
	{
		s_registered_capacity = s_registered_capacity ? s_registered_capacity * 2 : 16;
		s_registered = (Naui_RegisteredAction*)realloc(s_registered, s_registered_capacity * sizeof(Naui_RegisteredAction));
	}

	s_registered[s_registered_count++] = (Naui_RegisteredAction){ name, action };
}

static const Naui_Action* find_action(const char* name)
{
	for(size_t i = 0; i < s_registered_count; i++)
	{
		if(strcmp(s_registered[i].name, name) == 0)
			return &s_registered[i].action;
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

static const Naui_Action s_group_action = {
	.execute = NULL,
	.undo = group_undo,
	.redo = group_redo,
	.destroy = group_destroy
};

static Naui_Arena s_ring_arena;
static Naui_ActionEntry* s_undo_entries;
static Naui_ActionEntry* s_redo_entries;
static size_t s_capacity;
static size_t s_undo_head;
static size_t s_undo_count;
static size_t s_redo_count;

static inline size_t undo_physical_index(size_t logical_index)
{
	return (s_undo_head + logical_index) % s_capacity;
}

static void ensure_initialized(void)
{
	if(s_capacity == 0)
		naui_action_set_history_capacity(NAUI_ACTION_DEFAULT_CAPACITY);
}

void naui_action_set_history_capacity(size_t capacity)
{
	if(capacity == 0)
		capacity = 1;

	while(s_undo_count > capacity)
	{
		destroy_entry(&s_undo_entries[s_undo_head]);
		s_undo_head = (s_undo_head + 1) % s_capacity;
		s_undo_count--;
	}

	while(s_redo_count > capacity)
	{
		destroy_entry(&s_redo_entries[0]);
		memmove(&s_redo_entries[0], &s_redo_entries[1], (s_redo_count - 1) * sizeof(Naui_ActionEntry));
		s_redo_count--;
	}

	size_t old_undo_count = s_undo_count;
	size_t old_redo_count = s_redo_count;
	Naui_ActionEntry* undo_tmp = old_undo_count ? (Naui_ActionEntry*)malloc(old_undo_count * sizeof(Naui_ActionEntry)) : NULL;
	for(size_t i = 0; i < old_undo_count; i++)
	{
		undo_tmp[i] = s_undo_entries[undo_physical_index(i)];
	}

	Naui_ActionEntry* redo_tmp = old_redo_count ? (Naui_ActionEntry*)malloc(old_redo_count * sizeof(Naui_ActionEntry)) : NULL;
	if(redo_tmp)
		memcpy(redo_tmp, s_redo_entries, old_redo_count * sizeof(Naui_ActionEntry));

	naui_arena_reset(&s_ring_arena);
	s_undo_entries = (Naui_ActionEntry*)naui_arena_alloc(&s_ring_arena, capacity * sizeof(Naui_ActionEntry));
	s_redo_entries = (Naui_ActionEntry*)naui_arena_alloc(&s_ring_arena, capacity * sizeof(Naui_ActionEntry));

	if(undo_tmp)
	{
		memcpy(s_undo_entries, undo_tmp, old_undo_count * sizeof(Naui_ActionEntry));
		free(undo_tmp);
	}

	if(redo_tmp)
	{
		memcpy(s_redo_entries, redo_tmp, old_redo_count * sizeof(Naui_ActionEntry));
		free(redo_tmp);
	}

	s_capacity = capacity;
	s_undo_head = 0;
	s_undo_count = old_undo_count;
	s_redo_count = old_redo_count;
}

size_t naui_action_get_history_capacity(void)
{
	ensure_initialized();
	return s_capacity;
}

static void history_push(Naui_ActionEntry entry)
{
	ensure_initialized();
	for(size_t i = 0; i < s_redo_count; i++)
	{
		destroy_entry(&s_redo_entries[i]);
	}

	s_redo_count = 0;

	if(s_undo_count == s_capacity)
	{
		destroy_entry(&s_undo_entries[s_undo_head]);
		s_undo_head = (s_undo_head + 1) % s_capacity;
		s_undo_count--;
	}

	s_undo_entries[undo_physical_index(s_undo_count)] = entry;
	s_undo_count++;
}

typedef struct Naui_ActionStageNode Naui_ActionStageNode;
struct Naui_ActionStageNode
{
	Naui_ActionEntry entry;
	Naui_ActionStageNode* next;
};

static Naui_Arena s_group_arena;
static Naui_ActionStageNode* s_group_head;
static Naui_ActionStageNode* s_group_tail;
static size_t s_group_count;
static const char* s_group_name;
static bool s_group_active;

void naui_action_group_start(const char* name)
{
	NAUI_ACTION_FATAL(s_group_active, "naui_action_group_start called while a group is already active. Nested groups aren't supported.");
	s_group_active = true;
	s_group_name = name;
	s_group_head = NULL;
	s_group_tail = NULL;
	s_group_count = 0;
}

bool naui_action_group_end(void)
{
	NAUI_ACTION_FATAL(!s_group_active, "naui_action_group_end called with no active group.");
	s_group_active = false;
	if(s_group_count == 0)
	{
		naui_arena_reset(&s_group_arena);
		return false;
	}

	Naui_ActionEntry* actions = (Naui_ActionEntry*)malloc(s_group_count * sizeof(Naui_ActionEntry));
	size_t i = 0;
	for(Naui_ActionStageNode* node = s_group_head; node; node = node->next)
	{
		actions[i++] = node->entry;
	}

	naui_arena_reset(&s_group_arena);
	Naui_GroupData* group_data = (Naui_GroupData*)malloc(sizeof(Naui_GroupData));
	*group_data = (Naui_GroupData){ .actions = actions, .count = i };
	history_push((Naui_ActionEntry){ .name = s_group_name, .action = &s_group_action, .data = group_data });
	return true;
}

bool naui_action_execute(const char* name, void* data)
{
	ensure_initialized();
	const Naui_Action* action = find_action(name);
	if(!action || !action->execute(data))
		return false;

	Naui_ActionEntry entry = { .name = name, .action = action, .data = data };
	if(s_group_active)
	{
		Naui_ActionStageNode* node = (Naui_ActionStageNode*)naui_arena_alloc(&s_group_arena, sizeof(Naui_ActionStageNode));
		node->entry = entry;
		node->next = NULL;

		if(s_group_tail)
			s_group_tail->next = node;
		else
			s_group_head = node;

		s_group_tail = node;
		s_group_count++;
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
	if(s_undo_count == 0)
		return false;

	Naui_ActionEntry* entry = &s_undo_entries[undo_physical_index(s_undo_count - 1)];
	if(!entry->action->undo(entry->data))
		return false;

	s_redo_entries[s_redo_count++] = *entry;
	s_undo_count--;
	return true;
}

bool naui_action_redo(void)
{
	ensure_initialized();
	if(s_redo_count == 0)
		return false;

	Naui_ActionEntry* entry = &s_redo_entries[s_redo_count - 1];
	Naui_ActionFn fn = entry->action->redo ? entry->action->redo : entry->action->execute;
	if(!fn(entry->data))
		return false;

	s_undo_entries[undo_physical_index(s_undo_count)] = *entry;
	s_undo_count++;
	s_redo_count--;
	return true;
}

bool naui_action_can_undo(void)
{
	ensure_initialized();
	return s_undo_count > 0;
}

bool naui_action_can_redo(void)
{
	ensure_initialized();
	return s_redo_count > 0;
}

const char* naui_action_undo_name(void)
{
	ensure_initialized();
	return s_undo_count > 0 ? s_undo_entries[undo_physical_index(s_undo_count - 1)].name : NULL;
}

const char* naui_action_redo_name(void)
{
	ensure_initialized();
	return s_redo_count > 0 ? s_redo_entries[s_redo_count - 1].name : NULL;
}

void naui_action_clear_history(void)
{
	ensure_initialized();
	for(size_t i = 0; i < s_undo_count; i++)
	{
		destroy_entry(&s_undo_entries[undo_physical_index(i)]);
	}

	for(size_t i = 0; i < s_redo_count; i++)
	{
		destroy_entry(&s_redo_entries[i]);
	}

	s_undo_head = 0;
	s_undo_count = 0;
	s_redo_count = 0;
}
