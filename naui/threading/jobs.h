#define NAUI_JOB_INVALID ((Naui_JobHandle){ 0 })

typedef struct { uint32_t _id; } Naui_JobHandle;

typedef uint8_t Naui_JobStatus;
enum
{
	NAUI_JOB_PENDING,
	NAUI_JOB_RUNNING,
	NAUI_JOB_DONE,
	NAUI_JOB_FAILED,
};

typedef uint8_t Naui_JobSubmitResult;
enum
{
	NAUI_JOB_SUBMIT_OK,
	NAUI_JOB_SUBMIT_OVERFLOW,
	NAUI_JOB_SUBMIT_INVALID,
};

typedef void (*Naui_JobFn)(void* data, char* err_buf, size_t err_size);

void naui_jobs_init(int32_t thread_count, int32_t job_capacity);

/* Finish all pending/running jobs, then tear down the pool. */
void naui_jobs_shutdown(void);

/* Submit a job.
 * On success, *out_handle is set and NAUI_JOB_SUBMIT_OK is returned.
 * On failure, *out_handle is set to NAUI_JOB_INVALID. */
Naui_JobSubmitResult naui_job_submit(Naui_JobHandle* out_handle, Naui_JobFn fn, void* data);

/* Submit a job that wont start execution until `after` completes.
 * Job gets cancelled (marked NAUI_JOB_FAILED) if `after` fails. */
Naui_JobSubmitResult naui_job_submit_after(Naui_JobHandle* out_handle, Naui_JobFn fn, void* data, Naui_JobHandle after);

bool naui_job_is_valid(Naui_JobHandle handle);
Naui_JobStatus naui_job_status(Naui_JobHandle handle);
bool naui_job_is_done(Naui_JobHandle handle);

/* Retrieve the error string.
 * Empty string if not NAUI_JOB_FAILED. */
const char* naui_job_error(Naui_JobHandle handle);

/* Block until the job reaches DONE or FAILED, then release it.
 * The handle is invalid after this call returns. */
void naui_job_wait(Naui_JobHandle handle);

/* Block until the job reaches DONE or FAILED, but does NOT release the handle - use this when you need to inspect status/error afterward.
 * Call naui_job_release() once you're done with it. */
void naui_job_wait_peek(Naui_JobHandle handle);

/* Block until ALL handles in the array are done, then release them all. */
void naui_job_wait_all(const Naui_JobHandle* handles, int32_t count);

/* Block until ANY ONE handle in the array is done.
 * Returns the index of the completed job.
 * Does NOT release any handle - the caller decides what to do with the rest of the array. */
int naui_job_wait_any(const Naui_JobHandle* handles, int32_t count);

/* Release the handle slot so it can be reused.
 * Safe to call on a pending or running job (detaches without waiting)
 * The slot is reclaimed once the job actually finishes. */
void naui_job_release(Naui_JobHandle handle);
