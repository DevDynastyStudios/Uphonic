/*

NOTE(chimpchi): one quick thing i should mention.

if an action fails, it will return false, and when that happens it doesn't push
the data onto the undo stack. so you have to manually free data if you expect
a function to fail.

also if you don't handle the undo case properly if the execute failed, then you can have
some fucky behaviour...

*/

typedef bool (*Naui_ActionFn)(void* data);
typedef void (*Naui_ActionDestroyFn)(void* data);

typedef struct
{
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

// name of the action or group sitting at the top of each stack, NULL if empty.
const char* naui_action_undo_name(void);

// name of the action or group sitting at the top of each stack, NULL if empty.
const char* naui_action_redo_name(void);

// wipes both undo/redo histories.
void naui_action_clear_history(void);

// fixed capacity of undo/redo history.
void naui_action_set_history_capacity(size_t capacity);
size_t naui_action_get_history_capacity(void);

void naui_action_group_start(const char* name);
bool naui_action_group_end(void);
