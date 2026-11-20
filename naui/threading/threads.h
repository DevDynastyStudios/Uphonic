typedef struct Naui_ThreadImpl* Naui_Thread;
typedef struct Naui_MutexImpl* Naui_Mutex;
typedef struct Naui_CondImpl* Naui_Cond;

typedef int (*Naui_ThreadFn)(void* arg);

/* Creates and starts a new thread running `fn(arg)`.
 * Returns NULL on failure. */
Naui_Thread naui_thread_create(Naui_ThreadFn fn, void* arg);

/* Blocks until the thread finishes.
 * If out_result is non-NULL, it receives the int returned by the thread's function.
 * Frees the Naui_Thread handle - it is invalid after this call. */
void naui_thread_join(Naui_Thread thread, int* out_result);

/* Detaches the thread - it will clean up its own resources on exit and naui_thread_join() must NOT be called on it afterward.
 * Frees the Naui_Thread handle - it is invalid after this call. */
void naui_thread_detach(Naui_Thread thread);

/* Puts the CALLING thread to sleep for at least `milliseconds`. */
void naui_thread_sleep_ms(uint32_t milliseconds);

/* Returns an opaque id for the calling thread, for logging/debugging.
 * Not guaranteed stable across platforms beyond uniqueness while the thread is alive. */
uint64_t naui_thread_current_id(void);

/* Returns NULL on failure. */
Naui_Mutex naui_mutex_create(void);
void naui_mutex_destroy(Naui_Mutex mutex);

void naui_mutex_lock(Naui_Mutex mutex);
void naui_mutex_unlock(Naui_Mutex mutex);

/* Returns true if the lock was acquired, false if it was already held. */
bool naui_mutex_try_lock(Naui_Mutex mutex);

/* Returns NULL on failure. */
Naui_Cond naui_cond_create(void);
void naui_cond_destroy(Naui_Cond cond);

/* Atomically unlocks `mutex` and blocks the calling thread until signalled, then re-locks `mutex` before returning.
 * `mutex` must be held by the calling thread when this is called. */
void naui_cond_wait(Naui_Cond cond, Naui_Mutex mutex);

/* Wakes at least one thread waiting on `cond`. */
void naui_cond_signal(Naui_Cond cond);

/* Wakes every thread waiting on `cond`. */
void naui_cond_broadcast(Naui_Cond cond);
