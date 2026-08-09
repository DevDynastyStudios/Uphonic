typedef struct Naui_ArenaBlock Naui_ArenaBlock;

typedef struct Naui_Arena
{
	Naui_ArenaBlock* head;
} Naui_Arena;

NAUI_API void naui_arena_init(Naui_Arena* arena, size_t size);
NAUI_API void naui_arena_free(Naui_Arena* arena);
NAUI_API void naui_arena_reset(Naui_Arena* arena);
NAUI_API void* naui_arena_alloc(Naui_Arena* arena, size_t size);

NAUI_API Naui_Arena *naui_arena_frame(void);
