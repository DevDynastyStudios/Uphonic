#pragma once

#define NOTE_QUEUE_CAPACITY 256

typedef struct QueuedNote
{
	int16_t key;
	bool is_on;
} QueuedNote;

typedef struct NoteQueue
{
	QueuedNote items[NOTE_QUEUE_CAPACITY];
	_Atomic uint32_t write_index;
	_Atomic uint32_t read_index;
} NoteQueue;

bool note_queue_push(NoteQueue* queue, QueuedNote note);
bool note_queue_pop(NoteQueue* queue, QueuedNote* out);
