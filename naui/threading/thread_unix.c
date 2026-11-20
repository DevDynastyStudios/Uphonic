#if !defined(_WIN32) && !defined(_WIN64)

#include <pthread.h>
#include <time.h>
#include <errno.h>

struct Naui_ThreadImpl
{
	pthread_t handle;
};

struct Naui_MutexImpl
{
	pthread_mutex_t handle;
};

struct Naui_CondImpl
{
	pthread_cond_t handle;
};

typedef struct
{
	Naui_ThreadFn fn;
	void* arg;
} Naui_ThreadTrampolineArgs;

static void* thread_trampoline(void* raw_args)
{
	Naui_ThreadTrampolineArgs* args = (Naui_ThreadTrampolineArgs*)raw_args;
	Naui_ThreadFn fn = args->fn;
	void* arg = args->arg;
	free(args);

	int result = fn(arg);
	return (void*)(intptr_t)result;
}

Naui_Thread naui_thread_create(Naui_ThreadFn fn, void* arg)
{
	if (!fn)
		return NULL;

	Naui_ThreadTrampolineArgs* args = (Naui_ThreadTrampolineArgs*)malloc(sizeof(Naui_ThreadTrampolineArgs));
	if (!args)
		return NULL;

	args->fn = fn;
	args->arg = arg;
	struct Naui_ThreadImpl* impl = (struct Naui_ThreadImpl*)malloc(sizeof(struct Naui_ThreadImpl));
	if (!impl)
	{
		free(args);
		return NULL;
	}

	int rc = pthread_create(&impl->handle, NULL, thread_trampoline, args);
	if (rc != 0)
	{
		free(impl);
		free(args);
		return NULL;
	}

	return impl;
}

void naui_thread_join(Naui_Thread thread, int* out_result)
{
	if (!thread)
		return;

	void* raw_result = NULL;
	pthread_join(thread->handle, &raw_result);

	if (out_result)
		*out_result = (int)(intptr_t)raw_result;

	free(thread);
}

void naui_thread_detach(Naui_Thread thread)
{
	if (!thread)
		return;

	pthread_detach(thread->handle);
	free(thread);
}

void naui_thread_sleep_ms(uint32_t milliseconds)
{
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (long)(milliseconds % 1000) * 1000000L;

	while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
		continue;
}

uint64_t naui_thread_current_id(void)
{
	pthread_t self = pthread_self();
	uint64_t hash = 14695981039346656037ull;
	const unsigned char* bytes = (const unsigned char*)&self;
	for (size_t i = 0; i < sizeof(self); ++i)
	{
		hash ^= bytes[i];
		hash *= 1099511628211ull;
	}

	return hash;
}

Naui_Mutex naui_mutex_create(void)
{
	struct Naui_MutexImpl* impl = (struct Naui_MutexImpl*)malloc(sizeof(struct Naui_MutexImpl));
	if (!impl)
		return NULL;

	if (pthread_mutex_init(&impl->handle, NULL) != 0)
	{
		free(impl);
		return NULL;
	}

	return impl;
}

void naui_mutex_destroy(Naui_Mutex mutex)
{
	if (!mutex)
		return;

	pthread_mutex_destroy(&mutex->handle);
	free(mutex);
}

void naui_mutex_lock(Naui_Mutex mutex)
{
	pthread_mutex_lock(&mutex->handle);
}

void naui_mutex_unlock(Naui_Mutex mutex)
{
	pthread_mutex_unlock(&mutex->handle);
}

bool naui_mutex_try_lock(Naui_Mutex mutex)
{
	return pthread_mutex_trylock(&mutex->handle) == 0;
}

Naui_Cond naui_cond_create(void)
{
	struct Naui_CondImpl* impl = (struct Naui_CondImpl*)malloc(sizeof(struct Naui_CondImpl));
	if (!impl)
		return NULL;

	if (pthread_cond_init(&impl->handle, NULL) != 0)
	{
		free(impl);
		return NULL;
	}

	return impl;
}

void naui_cond_destroy(Naui_Cond cond)
{
	if (!cond)
		return;

	pthread_cond_destroy(&cond->handle);
	free(cond);
}

void naui_cond_wait(Naui_Cond cond, Naui_Mutex mutex)
{
	pthread_cond_wait(&cond->handle, &mutex->handle);
}

void naui_cond_signal(Naui_Cond cond)
{
	pthread_cond_signal(&cond->handle);
}

void naui_cond_broadcast(Naui_Cond cond)
{
	pthread_cond_broadcast(&cond->handle);
}
#endif
