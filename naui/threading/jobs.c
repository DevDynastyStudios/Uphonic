#define NAUI_JOB_ERR_BUF_SIZE 128

typedef uint8_t Naui_JobSlotState;
enum
{
	NAUI_SLOT_FREE,
	NAUI_SLOT_WAITING,
	NAUI_SLOT_QUEUED,
	NAUI_SLOT_RUNNING,
	NAUI_SLOT_FINISHED,
};

typedef struct Naui_JobSlot
{
	uint32_t generation;
	Naui_JobSlotState slot_state;
	Naui_JobStatus status;

	Naui_JobFn fn;
	void* data;

	uint32_t dependent_plus_one;
	bool detached;
	char err_buf[NAUI_JOB_ERR_BUF_SIZE];
} Naui_JobSlot;

typedef struct Naui_JobSystem
{
	Naui_JobSlot* slots;
	int32_t capacity;

	uint32_t* ready_queue;
	int32_t ready_head;
	int32_t ready_tail;
	int32_t ready_count;

	Naui_Thread* workers;
	int32_t worker_count;

	Naui_Mutex lock;
	Naui_Cond cv_queue_not_empty;
	Naui_Cond cv_job_done;

	bool shutting_down;
	bool initialized;
} Naui_JobSystem;

static Naui_JobSystem naui__jobs_state;

static bool slot_index_from_handle(Naui_JobHandle handle, uint32_t* out_index)
{
	if (handle._id == 0)
		return false;

	uint32_t packed = handle._id - 1;
	uint32_t index = packed & 0xFFFFu;
	uint32_t gen = packed >> 16;

	if ((int32_t)index >= naui__jobs_state.capacity)
		return false;

	if (naui__jobs_state.slots[index].generation != gen)
		return false;

	if (naui__jobs_state.slots[index].detached)
		return false;

	*out_index = index;
	return true;
}

static Naui_JobHandle handle_from_slot_index(uint32_t index)
{
	uint32_t gen = naui__jobs_state.slots[index].generation;
	uint32_t packed = (gen << 16) | (index & 0xFFFFu);

	Naui_JobHandle h;
	h._id = packed + 1;
	return h;
}

static void ready_queue_push(uint32_t slot_index)
{
	naui__jobs_state.ready_queue[naui__jobs_state.ready_tail] = slot_index;
	naui__jobs_state.ready_tail = (naui__jobs_state.ready_tail + 1) % naui__jobs_state.capacity;
	naui__jobs_state.ready_count += 1;

	naui__jobs_state.slots[slot_index].slot_state = NAUI_SLOT_QUEUED;
	naui_cond_signal(naui__jobs_state.cv_queue_not_empty);
}

/* Returns false if the queue is empty. */
static bool ready_queue_pop(uint32_t* out_index)
{
	if (naui__jobs_state.ready_count == 0)
		return false;

	*out_index = naui__jobs_state.ready_queue[naui__jobs_state.ready_head];
	naui__jobs_state.ready_head = (naui__jobs_state.ready_head + 1) % naui__jobs_state.capacity;
	naui__jobs_state.ready_count -= 1;
	return true;
}

/* Find a FREE slot and mark it WAITING.
 * Returns false if the pool is full. */
static bool slot_acquire(uint32_t* out_index)
{
	for (int32_t i = 0; i < naui__jobs_state.capacity; ++i)
	{
		if (naui__jobs_state.slots[i].slot_state != NAUI_SLOT_FREE)
			continue;

		*out_index = (uint32_t)i;
		return true;
	}

	return false;
}

static void slot_finish(uint32_t index, Naui_JobStatus final_status);

static void release_locked(uint32_t index)
{
	Naui_JobSlot* slot = &naui__jobs_state.slots[index];
	slot->slot_state = NAUI_SLOT_FREE;
	slot->fn = NULL;
	slot->data = NULL;
	slot->detached = false;
	slot->generation++;
}

/* Called when a job transitions to DONE/FAILED.
 * Pushes its dependent onto the ready queue, or fails it transitively.
 * Must be called with the lock held. */
static void slot_finish(uint32_t index, Naui_JobStatus final_status)
{
	Naui_JobSlot* slot = &naui__jobs_state.slots[index];
	slot->status = final_status;
	slot->slot_state = NAUI_SLOT_FINISHED;

	if (slot->dependent_plus_one != 0)
	{
		uint32_t dep_index = slot->dependent_plus_one - 1;
		Naui_JobSlot* dep = &naui__jobs_state.slots[dep_index];
		slot->dependent_plus_one = 0;

		if (final_status == NAUI_JOB_FAILED)
		{
			snprintf(dep->err_buf, sizeof(dep->err_buf), "cancelled: dependency failed");
			slot_finish(dep_index, NAUI_JOB_FAILED);
		}
		else
		{
			dep->status = NAUI_JOB_PENDING;
			ready_queue_push(dep_index);
		}
	}

	if (slot->detached)
		release_locked(index);

	naui_cond_broadcast(naui__jobs_state.cv_job_done);
}

static int worker_main(void* unused)
{
	(void)unused;
	while(true)
	{
		naui_mutex_lock(naui__jobs_state.lock);

		while (naui__jobs_state.ready_count == 0 && !naui__jobs_state.shutting_down)
		{
			naui_cond_wait(naui__jobs_state.cv_queue_not_empty, naui__jobs_state.lock);
		}

		if (naui__jobs_state.shutting_down && naui__jobs_state.ready_count == 0)
		{
			naui_mutex_unlock(naui__jobs_state.lock);
			return 0;
		}

		uint32_t index;
		bool got = ready_queue_pop(&index);
		if (!got)
		{
			naui_mutex_unlock(naui__jobs_state.lock);
			continue;
		}

		Naui_JobSlot* slot = &naui__jobs_state.slots[index];
		slot->slot_state = NAUI_SLOT_RUNNING;
		slot->status = NAUI_JOB_RUNNING;
		slot->err_buf[0] = '\0';

		Naui_JobFn fn = slot->fn;
		void* data = slot->data;
		naui_mutex_unlock(naui__jobs_state.lock);

		char local_err[NAUI_JOB_ERR_BUF_SIZE];
		local_err[0] = '\0';
		fn(data, local_err, sizeof(local_err));
		naui_mutex_lock(naui__jobs_state.lock);

		Naui_JobStatus final_status = (local_err[0] != '\0') ? NAUI_JOB_FAILED : NAUI_JOB_DONE;
		if (final_status == NAUI_JOB_FAILED)
			memcpy(slot->err_buf, local_err, sizeof(local_err));

		slot_finish(index, final_status);
		naui_mutex_unlock(naui__jobs_state.lock);
	}
}

void naui_jobs_init(int32_t thread_count, int32_t job_capacity)
{
	if (naui__jobs_state.initialized)
		return;

	memset(&naui__jobs_state, 0, sizeof(naui__jobs_state));
	naui__jobs_state.capacity = job_capacity > 0 ? job_capacity : 1;
	naui__jobs_state.slots = (Naui_JobSlot*)calloc((size_t)naui__jobs_state.capacity, sizeof(Naui_JobSlot));

	naui__jobs_state.ready_queue = (uint32_t*)calloc((size_t)naui__jobs_state.capacity, sizeof(uint32_t));
	naui__jobs_state.ready_head = 0;
	naui__jobs_state.ready_tail = 0;
	naui__jobs_state.ready_count = 0;

	naui__jobs_state.lock = naui_mutex_create();
	naui__jobs_state.cv_queue_not_empty = naui_cond_create();
	naui__jobs_state.cv_job_done = naui_cond_create();

	if (!naui__jobs_state.slots || !naui__jobs_state.ready_queue || !naui__jobs_state.lock || !naui__jobs_state.cv_queue_not_empty || !naui__jobs_state.cv_job_done)
	{
		naui_mutex_destroy(naui__jobs_state.lock);
		naui_cond_destroy(naui__jobs_state.cv_queue_not_empty);
		naui_cond_destroy(naui__jobs_state.cv_job_done);
		free(naui__jobs_state.slots);
		free(naui__jobs_state.ready_queue);
		memset(&naui__jobs_state, 0, sizeof(naui__jobs_state));
		return;
	}

	naui__jobs_state.worker_count = thread_count > 0 ? thread_count : 1;
	naui__jobs_state.workers = (Naui_Thread*)calloc((size_t)naui__jobs_state.worker_count, sizeof(Naui_Thread));
	naui__jobs_state.shutting_down = false;
	naui__jobs_state.initialized = true;

	for (int32_t i = 0; i < naui__jobs_state.worker_count; ++i)
	{
		naui__jobs_state.workers[i] = naui_thread_create(worker_main, NULL);
	}
}

void naui_jobs_shutdown(void)
{
	if (!naui__jobs_state.initialized)
		return;

	naui_mutex_lock(naui__jobs_state.lock);
	naui__jobs_state.shutting_down = true;
	naui_mutex_unlock(naui__jobs_state.lock);
	naui_cond_broadcast(naui__jobs_state.cv_queue_not_empty);

	for (int32_t i = 0; i < naui__jobs_state.worker_count; ++i)
	{
		naui_thread_join(naui__jobs_state.workers[i], NULL);
	}

	free(naui__jobs_state.workers);
	free(naui__jobs_state.ready_queue);
	free(naui__jobs_state.slots);

	naui_mutex_destroy(naui__jobs_state.lock);
	naui_cond_destroy(naui__jobs_state.cv_queue_not_empty);
	naui_cond_destroy(naui__jobs_state.cv_job_done);
	memset(&naui__jobs_state, 0, sizeof(naui__jobs_state));
}

Naui_JobSubmitResult naui_job_submit(Naui_JobHandle* out_handle, Naui_JobFn fn, void* data)
{
	if (out_handle)
		*out_handle = NAUI_JOB_INVALID;

	if (!naui__jobs_state.initialized || !fn || !out_handle)
		return NAUI_JOB_SUBMIT_INVALID;

	naui_mutex_lock(naui__jobs_state.lock);
	uint32_t index;
	if (!slot_acquire(&index))
	{
		naui_mutex_unlock(naui__jobs_state.lock);
		return NAUI_JOB_SUBMIT_OVERFLOW;
	}

	Naui_JobSlot* slot = &naui__jobs_state.slots[index];
	slot->fn = fn;
	slot->data = data;
	slot->status = NAUI_JOB_PENDING;
	slot->dependent_plus_one = 0;
	slot->detached = false;
	slot->err_buf[0] = '\0';

	*out_handle = handle_from_slot_index(index);
	ready_queue_push(index);
	naui_mutex_unlock(naui__jobs_state.lock);
	return NAUI_JOB_SUBMIT_OK;
}

Naui_JobSubmitResult naui_job_submit_after(Naui_JobHandle* out_handle, Naui_JobFn fn, void* data, Naui_JobHandle after)
{
	if (out_handle)
		*out_handle = NAUI_JOB_INVALID;

	if (!naui__jobs_state.initialized || !fn || !out_handle)
		return NAUI_JOB_SUBMIT_INVALID;

	naui_mutex_lock(naui__jobs_state.lock);
	uint32_t after_index;
	bool after_valid = slot_index_from_handle(after, &after_index);

	if (after_valid)
	{
		Naui_JobSlot* after_slot_check = &naui__jobs_state.slots[after_index];
		if (after_slot_check->slot_state != NAUI_SLOT_FINISHED && after_slot_check->dependent_plus_one != 0)
		{
			naui_mutex_unlock(naui__jobs_state.lock);
			return NAUI_JOB_SUBMIT_INVALID;
		}
	}

	uint32_t index;
	if (!slot_acquire(&index))
	{
		naui_mutex_unlock(naui__jobs_state.lock);
		return NAUI_JOB_SUBMIT_OVERFLOW;
	}

	Naui_JobSlot* slot = &naui__jobs_state.slots[index];
	slot->fn = fn;
	slot->data = data;
	slot->dependent_plus_one = 0;
	slot->detached = false;
	slot->err_buf[0] = '\0';
	*out_handle = handle_from_slot_index(index);

	if (!after_valid)
	{
		slot->status = NAUI_JOB_PENDING;
		ready_queue_push(index);
		naui_mutex_unlock(naui__jobs_state.lock);
		return NAUI_JOB_SUBMIT_OK;
	}

	Naui_JobSlot* after_slot = &naui__jobs_state.slots[after_index];
	if (after_slot->slot_state == NAUI_SLOT_FINISHED)
	{
		if (after_slot->status == NAUI_JOB_FAILED)
		{
			snprintf(slot->err_buf, sizeof(slot->err_buf), "cancelled: dependency failed");
			slot->slot_state = NAUI_SLOT_WAITING;
			slot_finish(index, NAUI_JOB_FAILED);
		}
		else
		{
			slot->status = NAUI_JOB_PENDING;
			ready_queue_push(index);
		}
	}
	else
	{
		slot->status = NAUI_JOB_PENDING;
		slot->slot_state = NAUI_SLOT_WAITING;
		after_slot->dependent_plus_one = index + 1;
	}

	naui_mutex_unlock(naui__jobs_state.lock);
	return NAUI_JOB_SUBMIT_OK;
}

bool naui_job_is_valid(Naui_JobHandle handle)
{
	if (!naui__jobs_state.initialized)
		return false;

	naui_mutex_lock(naui__jobs_state.lock);
	uint32_t index;
	bool valid = slot_index_from_handle(handle, &index);
	naui_mutex_unlock(naui__jobs_state.lock);
	return valid;
}

Naui_JobStatus naui_job_status(Naui_JobHandle handle)
{
	if (!naui__jobs_state.initialized)
		return NAUI_JOB_FAILED;

	naui_mutex_lock(naui__jobs_state.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index))
	{
		naui_mutex_unlock(naui__jobs_state.lock);
		return NAUI_JOB_FAILED;
	}

	Naui_JobStatus status = naui__jobs_state.slots[index].status;
	naui_mutex_unlock(naui__jobs_state.lock);
	return status;
}

bool naui_job_is_done(Naui_JobHandle handle)
{
	Naui_JobStatus status = naui_job_status(handle);
	return status == NAUI_JOB_DONE || status == NAUI_JOB_FAILED;
}

const char* naui_job_error(Naui_JobHandle handle)
{
	static const char empty[1] = "";
	if (!naui__jobs_state.initialized)
		return empty;

	naui_mutex_lock(naui__jobs_state.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index) || naui__jobs_state.slots[index].status != NAUI_JOB_FAILED)
	{
		naui_mutex_unlock(naui__jobs_state.lock);
		return empty;
	}

	const char* err = naui__jobs_state.slots[index].err_buf;
	naui_mutex_unlock(naui__jobs_state.lock);
	return err;
}

void naui_job_wait(Naui_JobHandle handle)
{
	if (!naui__jobs_state.initialized)
		return;

	naui_mutex_lock(naui__jobs_state.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index))
	{
		naui_mutex_unlock(naui__jobs_state.lock);
		return;
	}

	while (naui__jobs_state.slots[index].slot_state != NAUI_SLOT_FINISHED)
	{
		naui_cond_wait(naui__jobs_state.cv_job_done, naui__jobs_state.lock);
	}

	release_locked(index);
	naui_mutex_unlock(naui__jobs_state.lock);
}

void naui_job_wait_peek(Naui_JobHandle handle)
{
	if (!naui__jobs_state.initialized)
		return;

	naui_mutex_lock(naui__jobs_state.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index))
	{
		naui_mutex_unlock(naui__jobs_state.lock);
		return;
	}

	while (naui__jobs_state.slots[index].slot_state != NAUI_SLOT_FINISHED)
	{
		naui_cond_wait(naui__jobs_state.cv_job_done, naui__jobs_state.lock);
	}

	naui_mutex_unlock(naui__jobs_state.lock);
}

void naui_job_wait_all(const Naui_JobHandle* handles, int32_t count)
{
	if (!naui__jobs_state.initialized || !handles)
		return;

	naui_mutex_lock(naui__jobs_state.lock);
	for (int32_t i = 0; i < count; ++i)
	{
		uint32_t index;
		if (!slot_index_from_handle(handles[i], &index))
			continue;

		while (naui__jobs_state.slots[index].slot_state != NAUI_SLOT_FINISHED)
		{
			naui_cond_wait(naui__jobs_state.cv_job_done, naui__jobs_state.lock);
		}
	}

	for (int32_t i = 0; i < count; ++i)
	{
		uint32_t index;
		if (!slot_index_from_handle(handles[i], &index))
			continue;

		release_locked(index);
	}

	naui_mutex_unlock(naui__jobs_state.lock);
}

int naui_job_wait_any(const Naui_JobHandle* handles, int32_t count)
{
	if (!naui__jobs_state.initialized || !handles || count <= 0)
		return -1;

	naui_mutex_lock(naui__jobs_state.lock);

	while(true)
	{
		for (int32_t i = 0; i < count; ++i)
		{
			uint32_t index;
			if (!slot_index_from_handle(handles[i], &index))
				continue;

			if (naui__jobs_state.slots[index].slot_state == NAUI_SLOT_FINISHED)
			{
				naui_mutex_unlock(naui__jobs_state.lock);
				return i;
			}
		}

		naui_cond_wait(naui__jobs_state.cv_job_done, naui__jobs_state.lock);
	}
}

void naui_job_release(Naui_JobHandle handle)
{
	if (!naui__jobs_state.initialized)
		return;

	naui_mutex_lock(naui__jobs_state.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index))
	{
		naui_mutex_unlock(naui__jobs_state.lock);
		return;
	}

	Naui_JobSlot* slot = &naui__jobs_state.slots[index];
	if (slot->slot_state == NAUI_SLOT_FINISHED)
		release_locked(index);
	else
		slot->detached = true;

	naui_mutex_unlock(naui__jobs_state.lock);
}
