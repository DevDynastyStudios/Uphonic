/*
	An action that fails (returns false) will NOT push data onto the undo stack.
	Any data that you expect to return false, must be checked and manually freed.
	Undo functions must be written to accommodate this behavior.
*/

typedef bool (*Naui_ActionFn)(void* data);
typedef void (*Naui_ActionDestroyFn)(void* data);

typedef struct
{
	Naui_String name;
	Naui_ActionFn execute;
	Naui_ActionFn undo;
	Naui_ActionFn redo;
	Naui_ActionDestroyFn destroy;
}
Naui_Action;

typedef struct { Naui_Action wrapped; } Naui_ActionWrapper;
#define naui_register_action(name, ...) __naui_register_action(name, (Naui_ActionWrapper){ __VA_ARGS__ }.wrapped)
void __naui_register_action(const char* name, Naui_Action type);

// runs the named action's `execute` with `data`, then pushes it onto undo history.
bool naui_action_execute(const char* name, void* data);

// undo/redo the most recent action or group.
// returns false if there was nothing to undo/redo, OR if the stored undo()/redo() itself returned false
bool naui_action_undo(void);
bool naui_action_redo(void);

bool naui_action_can_undo(void);
bool naui_action_can_redo(void);

const Naui_List(Naui_Action) naui_action_get_undo_history(void);
const Naui_List(Naui_Action) naui_action_get_redo_history(void);

// name of the action or group sitting at the top of each stack, NULL if empty.
//const char* naui_action_undo_name(void);

// name of the action or group sitting at the top of each stack, NULL if empty.
//const char* naui_action_redo_name(void);

// wipes both undo/redo histories.
void naui_action_clear_history(void);

// fixed capacity of undo/redo history.
void naui_action_set_history_capacity(size_t capacity);
size_t naui_action_get_history_capacity(void);

void naui_action_group_start(const char* name);
bool naui_action_group_end(void);
