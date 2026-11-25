bool note_queue_push(NoteQueue* queue, QueuedNote note)
{
	const uint32_t write = atomic_load_explicit(&queue->write_index, memory_order_relaxed);
	const uint32_t read = atomic_load_explicit(&queue->read_index, memory_order_acquire);

	if(write - read >= NOTE_QUEUE_CAPACITY)
		return false;

	queue->items[write & (NOTE_QUEUE_CAPACITY - 1)] = note;
	atomic_store_explicit(&queue->write_index, write + 1, memory_order_release);
	return true;
}

bool note_queue_pop(NoteQueue* queue, QueuedNote* out)
{
	const uint32_t read = atomic_load_explicit(&queue->read_index, memory_order_relaxed);
	const uint32_t write = atomic_load_explicit(&queue->write_index, memory_order_acquire);

	if(read == write)
		return false;

	*out = queue->items[read & (NOTE_QUEUE_CAPACITY - 1)];
	atomic_store_explicit(&queue->read_index, read + 1, memory_order_release);
	return true;
}
