#pragma once

// One producer thread and one consumer thread, no locks. The UI thread is the
// producer and the audio thread the consumer: the audio thread must never wait
// on the UI. A full queue drops the new note instead of blocking.

#define NOTE_QUEUE_CAPACITY 256 // Power of two, so the index math is a mask.

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
