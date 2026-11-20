#if defined(_WIN32) || defined(_WIN64)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

struct Naui_ThreadImpl
{
	HANDLE handle;
};

struct Naui_MutexImpl
{
	SRWLOCK handle;
};

struct Naui_CondImpl
{
	CONDITION_VARIABLE handle;
	HANDLE responsive_wake;
	volatile LONG responsive_waiters;
};

typedef struct
{
	Naui_ThreadFn fn;
	void* arg;
} Naui_ThreadTrampolineArgs;

static unsigned __stdcall thread_trampoline(void* raw_args)
{
	Naui_ThreadTrampolineArgs* args = (Naui_ThreadTrampolineArgs*)raw_args;
	Naui_ThreadFn fn = args->fn;
	void* arg = args->arg;
	free(args);

	int result = fn(arg);
	return (unsigned)result;
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

	uintptr_t handle = _beginthreadex(NULL, 0, thread_trampoline, args, 0, NULL);
	if (handle == 0)
	{
		free(impl);
		free(args);
		return NULL;
	}

	impl->handle = (HANDLE)handle;
	return impl;
}

void naui_thread_join(Naui_Thread thread, int* out_result)
{
	if (!thread)
		return;

	WaitForSingleObject(thread->handle, INFINITE);
	if (out_result)
	{
		DWORD exit_code = 0;
		GetExitCodeThread(thread->handle, &exit_code);
		*out_result = (int)exit_code;
	}

	CloseHandle(thread->handle);
	free(thread);
}

void naui_thread_detach(Naui_Thread thread)
{
	if (!thread)
		return;

	CloseHandle(thread->handle);
	free(thread);
}

void naui_thread_sleep_ms(uint32_t milliseconds)
{
	Sleep((DWORD)milliseconds);
}

uint64_t naui_thread_current_id(void)
{
	return (uint64_t)GetCurrentThreadId();
}

Naui_Mutex naui_mutex_create(void)
{
	struct Naui_MutexImpl* impl = (struct Naui_MutexImpl*)malloc(sizeof(struct Naui_MutexImpl));
	if (!impl)
		return NULL;

	InitializeSRWLock(&impl->handle);
	return impl;
}

void naui_mutex_destroy(Naui_Mutex mutex)
{
	if (!mutex)
		return;

	free(mutex);
}

void naui_mutex_lock(Naui_Mutex mutex)
{
	AcquireSRWLockExclusive(&mutex->handle);
}

void naui_mutex_unlock(Naui_Mutex mutex)
{
	ReleaseSRWLockExclusive(&mutex->handle);
}

bool naui_mutex_try_lock(Naui_Mutex mutex)
{
	return TryAcquireSRWLockExclusive(&mutex->handle) != 0;
}

Naui_Cond naui_cond_create(void)
{
	struct Naui_CondImpl* impl = (struct Naui_CondImpl*)malloc(sizeof(struct Naui_CondImpl));
	if (!impl)
		return NULL;

	impl->responsive_wake = CreateSemaphoreA(NULL, 0, LONG_MAX, NULL);
	if (!impl->responsive_wake)
	{
		free(impl);
		return NULL;
	}

	InitializeConditionVariable(&impl->handle);
	impl->responsive_waiters = 0;
	return impl;
}

void naui_cond_destroy(Naui_Cond cond)
{
	if (!cond)
		return;

	CloseHandle(cond->responsive_wake);
	free(cond);
}

void naui_cond_wait(Naui_Cond cond, Naui_Mutex mutex)
{
	SleepConditionVariableSRW(&cond->handle, &mutex->handle, INFINITE, 0);
}

void naui_cond_wait_responsive(Naui_Cond cond, Naui_Mutex mutex)
{
	InterlockedIncrement(&cond->responsive_waiters);
	naui_mutex_unlock(mutex);

	MsgWaitForMultipleObjectsEx(1, &cond->responsive_wake, INFINITE, QS_SENDMESSAGE, 0);
	MSG msg;
	PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);

	InterlockedDecrement(&cond->responsive_waiters);
	naui_mutex_lock(mutex);
}

void naui_cond_wait_pumping(Naui_Cond cond, Naui_Mutex mutex)
{
	InterlockedIncrement(&cond->responsive_waiters);
	naui_mutex_unlock(mutex);

	MsgWaitForMultipleObjectsEx(1, &cond->responsive_wake, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
	MSG msg;
	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	InterlockedDecrement(&cond->responsive_waiters);
	naui_mutex_lock(mutex);
}

void naui_cond_signal(Naui_Cond cond)
{
	WakeConditionVariable(&cond->handle);
	if (InterlockedCompareExchange(&cond->responsive_waiters, 0, 0) > 0)
		ReleaseSemaphore(cond->responsive_wake, 1, NULL);
}

void naui_cond_broadcast(Naui_Cond cond)
{
	WakeAllConditionVariable(&cond->handle);
	const LONG waiters = InterlockedCompareExchange(&cond->responsive_waiters, 0, 0);
	if (waiters > 0)
		ReleaseSemaphore(cond->responsive_wake, waiters, NULL);
}
#endif
