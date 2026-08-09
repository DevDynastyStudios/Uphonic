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

static Naui_JobSystem g_jobs;

static bool slot_index_from_handle(Naui_JobHandle handle, uint32_t* out_index)
{
	if (handle._id == 0)
		return false;

	uint32_t packed = handle._id - 1;
	uint32_t index = packed & 0xFFFFu;
	uint32_t gen = packed >> 16;

	if ((int32_t)index >= g_jobs.capacity)
		return false;

	if (g_jobs.slots[index].generation != gen)
		return false;

	if (g_jobs.slots[index].detached)
		return false;

	*out_index = index;
	return true;
}

static Naui_JobHandle handle_from_slot_index(uint32_t index)
{
	uint32_t gen = g_jobs.slots[index].generation;
	uint32_t packed = (gen << 16) | (index & 0xFFFFu);

	Naui_JobHandle h;
	h._id = packed + 1;
	return h;
}

static void ready_queue_push(uint32_t slot_index)
{
	g_jobs.ready_queue[g_jobs.ready_tail] = slot_index;
	g_jobs.ready_tail = (g_jobs.ready_tail + 1) % g_jobs.capacity;
	g_jobs.ready_count += 1;

	g_jobs.slots[slot_index].slot_state = NAUI_SLOT_QUEUED;
	naui_cond_signal(g_jobs.cv_queue_not_empty);
}

/* Returns false if the queue is empty. */
static bool ready_queue_pop(uint32_t* out_index)
{
	if (g_jobs.ready_count == 0)
		return false;

	*out_index = g_jobs.ready_queue[g_jobs.ready_head];
	g_jobs.ready_head = (g_jobs.ready_head + 1) % g_jobs.capacity;
	g_jobs.ready_count -= 1;
	return true;
}

/* Find a FREE slot and mark it WAITING.
 * Returns false if the pool is full. */
static bool slot_acquire(uint32_t* out_index)
{
	for (int32_t i = 0; i < g_jobs.capacity; ++i)
	{
		if (g_jobs.slots[i].slot_state != NAUI_SLOT_FREE)
			continue;

		*out_index = (uint32_t)i;
		return true;
	}

	return false;
}

static void slot_finish(uint32_t index, Naui_JobStatus final_status);

static void release_locked(uint32_t index)
{
	Naui_JobSlot* slot = &g_jobs.slots[index];
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
	Naui_JobSlot* slot = &g_jobs.slots[index];
	slot->status = final_status;
	slot->slot_state = NAUI_SLOT_FINISHED;

	if (slot->dependent_plus_one != 0)
	{
		uint32_t dep_index = slot->dependent_plus_one - 1;
		Naui_JobSlot* dep = &g_jobs.slots[dep_index];
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

	naui_cond_broadcast(g_jobs.cv_job_done);
}

static int worker_main(void* unused)
{
	(void)unused;
	while(true)
	{
		naui_mutex_lock(g_jobs.lock);

		while (g_jobs.ready_count == 0 && !g_jobs.shutting_down)
		{
			naui_cond_wait(g_jobs.cv_queue_not_empty, g_jobs.lock);
		}

		if (g_jobs.shutting_down && g_jobs.ready_count == 0)
		{
			naui_mutex_unlock(g_jobs.lock);
			return 0;
		}

		uint32_t index;
		bool got = ready_queue_pop(&index);
		if (!got)
		{
			naui_mutex_unlock(g_jobs.lock);
			continue;
		}

		Naui_JobSlot* slot = &g_jobs.slots[index];
		slot->slot_state = NAUI_SLOT_RUNNING;
		slot->status = NAUI_JOB_RUNNING;
		slot->err_buf[0] = '\0';

		Naui_JobFn fn = slot->fn;
		void* data = slot->data;
		naui_mutex_unlock(g_jobs.lock);

		char local_err[NAUI_JOB_ERR_BUF_SIZE];
		local_err[0] = '\0';
		fn(data, local_err, sizeof(local_err));
		naui_mutex_lock(g_jobs.lock);

		Naui_JobStatus final_status = (local_err[0] != '\0') ? NAUI_JOB_FAILED : NAUI_JOB_DONE;
		if (final_status == NAUI_JOB_FAILED)
			memcpy(slot->err_buf, local_err, sizeof(local_err));

		slot_finish(index, final_status);
		naui_mutex_unlock(g_jobs.lock);
	}
}

void naui_jobs_init(int32_t thread_count, int32_t job_capacity)
{
	if (g_jobs.initialized)
		return;

	memset(&g_jobs, 0, sizeof(g_jobs));
	g_jobs.capacity = job_capacity > 0 ? job_capacity : 1;
	g_jobs.slots = (Naui_JobSlot*)calloc((size_t)g_jobs.capacity, sizeof(Naui_JobSlot));

	g_jobs.ready_queue = (uint32_t*)calloc((size_t)g_jobs.capacity, sizeof(uint32_t));
	g_jobs.ready_head = 0;
	g_jobs.ready_tail = 0;
	g_jobs.ready_count = 0;

	g_jobs.lock = naui_mutex_create();
	g_jobs.cv_queue_not_empty = naui_cond_create();
	g_jobs.cv_job_done = naui_cond_create();

	if (!g_jobs.slots || !g_jobs.ready_queue || !g_jobs.lock || !g_jobs.cv_queue_not_empty || !g_jobs.cv_job_done)
	{
		naui_mutex_destroy(g_jobs.lock);
		naui_cond_destroy(g_jobs.cv_queue_not_empty);
		naui_cond_destroy(g_jobs.cv_job_done);
		free(g_jobs.slots);
		free(g_jobs.ready_queue);
		memset(&g_jobs, 0, sizeof(g_jobs));
		return;
	}

	g_jobs.worker_count = thread_count > 0 ? thread_count : 1;
	g_jobs.workers = (Naui_Thread*)calloc((size_t)g_jobs.worker_count, sizeof(Naui_Thread));
	g_jobs.shutting_down = false;
	g_jobs.initialized = true;

	for (int32_t i = 0; i < g_jobs.worker_count; ++i)
	{
		g_jobs.workers[i] = naui_thread_create(worker_main, NULL);
	}
}

void naui_jobs_shutdown(void)
{
	if (!g_jobs.initialized)
		return;

	naui_mutex_lock(g_jobs.lock);
	g_jobs.shutting_down = true;
	naui_mutex_unlock(g_jobs.lock);
	naui_cond_broadcast(g_jobs.cv_queue_not_empty);

	for (int32_t i = 0; i < g_jobs.worker_count; ++i)
	{
		naui_thread_join(g_jobs.workers[i], NULL);
	}

	free(g_jobs.workers);
	free(g_jobs.ready_queue);
	free(g_jobs.slots);

	naui_mutex_destroy(g_jobs.lock);
	naui_cond_destroy(g_jobs.cv_queue_not_empty);
	naui_cond_destroy(g_jobs.cv_job_done);
	memset(&g_jobs, 0, sizeof(g_jobs));
}

Naui_JobSubmitResult naui_job_submit(Naui_JobHandle* out_handle, Naui_JobFn fn, void* data)
{
	if (out_handle)
		*out_handle = NAUI_JOB_INVALID;

	if (!g_jobs.initialized || !fn || !out_handle)
		return NAUI_JOB_SUBMIT_INVALID;

	naui_mutex_lock(g_jobs.lock);
	uint32_t index;
	if (!slot_acquire(&index))
	{
		naui_mutex_unlock(g_jobs.lock);
		return NAUI_JOB_SUBMIT_OVERFLOW;
	}

	Naui_JobSlot* slot = &g_jobs.slots[index];
	slot->fn = fn;
	slot->data = data;
	slot->status = NAUI_JOB_PENDING;
	slot->dependent_plus_one = 0;
	slot->detached = false;
	slot->err_buf[0] = '\0';

	*out_handle = handle_from_slot_index(index);
	ready_queue_push(index);
	naui_mutex_unlock(g_jobs.lock);
	return NAUI_JOB_SUBMIT_OK;
}

Naui_JobSubmitResult naui_job_submit_after(Naui_JobHandle* out_handle, Naui_JobFn fn, void* data, Naui_JobHandle after)
{
	if (out_handle)
		*out_handle = NAUI_JOB_INVALID;

	if (!g_jobs.initialized || !fn || !out_handle)
		return NAUI_JOB_SUBMIT_INVALID;

	naui_mutex_lock(g_jobs.lock);
	uint32_t after_index;
	bool after_valid = slot_index_from_handle(after, &after_index);

	if (after_valid)
	{
		Naui_JobSlot* after_slot_check = &g_jobs.slots[after_index];
		if (after_slot_check->slot_state != NAUI_SLOT_FINISHED && after_slot_check->dependent_plus_one != 0)
		{
			naui_mutex_unlock(g_jobs.lock);
			return NAUI_JOB_SUBMIT_INVALID;
		}
	}

	uint32_t index;
	if (!slot_acquire(&index))
	{
		naui_mutex_unlock(g_jobs.lock);
		return NAUI_JOB_SUBMIT_OVERFLOW;
	}

	Naui_JobSlot* slot = &g_jobs.slots[index];
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
		naui_mutex_unlock(g_jobs.lock);
		return NAUI_JOB_SUBMIT_OK;
	}

	Naui_JobSlot* after_slot = &g_jobs.slots[after_index];
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

	naui_mutex_unlock(g_jobs.lock);
	return NAUI_JOB_SUBMIT_OK;
}

bool naui_job_is_valid(Naui_JobHandle handle)
{
	if (!g_jobs.initialized)
		return false;

	naui_mutex_lock(g_jobs.lock);
	uint32_t index;
	bool valid = slot_index_from_handle(handle, &index);
	naui_mutex_unlock(g_jobs.lock);
	return valid;
}

Naui_JobStatus naui_job_status(Naui_JobHandle handle)
{
	if (!g_jobs.initialized)
		return NAUI_JOB_FAILED;

	naui_mutex_lock(g_jobs.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index))
	{
		naui_mutex_unlock(g_jobs.lock);
		return NAUI_JOB_FAILED;
	}

	Naui_JobStatus status = g_jobs.slots[index].status;
	naui_mutex_unlock(g_jobs.lock);
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
	if (!g_jobs.initialized)
		return empty;

	naui_mutex_lock(g_jobs.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index) || g_jobs.slots[index].status != NAUI_JOB_FAILED)
	{
		naui_mutex_unlock(g_jobs.lock);
		return empty;
	}

	const char* err = g_jobs.slots[index].err_buf;
	naui_mutex_unlock(g_jobs.lock);
	return err;
}

void naui_job_wait(Naui_JobHandle handle)
{
	if (!g_jobs.initialized)
		return;

	naui_mutex_lock(g_jobs.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index))
	{
		naui_mutex_unlock(g_jobs.lock);
		return;
	}

	while (g_jobs.slots[index].slot_state != NAUI_SLOT_FINISHED)
	{
		naui_cond_wait(g_jobs.cv_job_done, g_jobs.lock);
	}

	release_locked(index);
	naui_mutex_unlock(g_jobs.lock);
}

void naui_job_wait_peek(Naui_JobHandle handle)
{
	if (!g_jobs.initialized)
		return;

	naui_mutex_lock(g_jobs.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index))
	{
		naui_mutex_unlock(g_jobs.lock);
		return;
	}

	while (g_jobs.slots[index].slot_state != NAUI_SLOT_FINISHED)
	{
		naui_cond_wait(g_jobs.cv_job_done, g_jobs.lock);
	}

	naui_mutex_unlock(g_jobs.lock);
}

void naui_job_wait_all(const Naui_JobHandle* handles, int32_t count)
{
	if (!g_jobs.initialized || !handles)
		return;

	naui_mutex_lock(g_jobs.lock);
	for (int32_t i = 0; i < count; ++i)
	{
		uint32_t index;
		if (!slot_index_from_handle(handles[i], &index))
			continue;

		while (g_jobs.slots[index].slot_state != NAUI_SLOT_FINISHED)
		{
			naui_cond_wait(g_jobs.cv_job_done, g_jobs.lock);
		}
	}

	for (int32_t i = 0; i < count; ++i)
	{
		uint32_t index;
		if (!slot_index_from_handle(handles[i], &index))
			continue;

		release_locked(index);
	}

	naui_mutex_unlock(g_jobs.lock);
}

int naui_job_wait_any(const Naui_JobHandle* handles, int32_t count)
{
	if (!g_jobs.initialized || !handles || count <= 0)
		return -1;

	naui_mutex_lock(g_jobs.lock);

	while(true)
	{
		for (int32_t i = 0; i < count; ++i)
		{
			uint32_t index;
			if (!slot_index_from_handle(handles[i], &index))
				continue;

			if (g_jobs.slots[index].slot_state == NAUI_SLOT_FINISHED)
			{
				naui_mutex_unlock(g_jobs.lock);
				return i;
			}
		}

		naui_cond_wait(g_jobs.cv_job_done, g_jobs.lock);
	}
}

void naui_job_release(Naui_JobHandle handle)
{
	if (!g_jobs.initialized)
		return;

	naui_mutex_lock(g_jobs.lock);
	uint32_t index;
	if (!slot_index_from_handle(handle, &index))
	{
		naui_mutex_unlock(g_jobs.lock);
		return;
	}

	Naui_JobSlot* slot = &g_jobs.slots[index];
	if (slot->slot_state == NAUI_SLOT_FINISHED)
		release_locked(index);
	else
		slot->detached = true;

	naui_mutex_unlock(g_jobs.lock);
}
