#define NAUI_ARENA_BLOCK_SIZE (32 * 1024)

typedef struct Naui_ArenaBlock
{
	struct Naui_ArenaBlock* next;
	size_t used;
	size_t cap;
} Naui_ArenaBlock;

static void* arena_alloc_block(Naui_Arena* arena, size_t size)
{
	size_t cap = size > NAUI_ARENA_BLOCK_SIZE ? size : NAUI_ARENA_BLOCK_SIZE;
	Naui_ArenaBlock* block = (Naui_ArenaBlock*) malloc(sizeof(Naui_ArenaBlock) + cap);
	if(!block)
		return NULL;

	block->next = NULL;
	block->used = size;
	block->cap = cap;
	if(!arena->head)
		arena->head = block;
	else
	{
		Naui_ArenaBlock* last = arena->head;
		while(last->next)
		{
			last = last->next;
		}

		last->next = block;
	}
 
	memset((char*)(block + 1), 0, size);
	return block;
}

void naui_arena_init(Naui_Arena* arena, size_t size)
{
	arena->head = NULL;
	if(size > 0)
		arena_alloc_block(arena, size);
}

void naui_arena_free(Naui_Arena* arena)
{
	Naui_ArenaBlock* block = arena->head;
	while(block)
	{
		Naui_ArenaBlock* next = block->next;
		free(block);
		block = next;
	}

	arena->head = NULL;
}

void naui_arena_reset(Naui_Arena* arena)
{
	Naui_ArenaBlock* block = arena->head;
	while(block)
	{
		block->used = 0;
		block = block->next;
	}
}

void* naui_arena_alloc(Naui_Arena* arena, size_t size)
{
	size = (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
	Naui_ArenaBlock* block = arena->head;
	Naui_ArenaBlock* last = NULL;

	while(block)
	{
		last = block;
		block = block->next;
	}

	if(last && last->used + size <= last->cap)
	{
		void* ptr = (char*)(last + 1) + last->used;
		last->used += size;
		memset(ptr, 0, size);
		return ptr;
	}

	Naui_ArenaBlock* new_block = (Naui_ArenaBlock*)arena_alloc_block(arena, size);
	if(!new_block)
		return NULL;

	return (char*)(new_block + 1);
}

static Naui_Arena frame_arena = {0};

NAUI_API Naui_Arena *naui_arena_frame(void) {
    return &frame_arena;
}
