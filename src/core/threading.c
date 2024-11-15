#include "core/core_common.h"
#include <assert.h>
#include <Windows.h>


#define MAX_TASKS_IN_QUEUE 100
#define MAX_WORK_THREADS 1
#define MAX_TASKS_PER_THREAD 50
#define MAX_HANDLES 100
#define LOW_PRIORITY_TIME_THRESHOLD 5000
#define THREAD_SHUTDOWN_TIME_THRESHOLD 10000

typedef enum
{
	TDR__SLEEP,
	TDR__NEW_TASK,
	TDR__EXIT
} TaskDispatchResult;

typedef enum
{
	TWS__EMPTY,
	TWS__READY_FOR_WORK,
	TWS__WORKING,
	TWS__COMPLETED
} TaskWorkStatus;

typedef enum
{
	TS__NOT_LAUNCHED,
	TS__LAUNCHED,
	TS__EXIT
} ThreadStatus;

typedef enum
{
	TCS__NO_WORK,
	TCS__WORK_AVAILABLE,
	TCS__FORCE_EXIT
} ThreadCoreStatus;

typedef struct
{
	TaskWorkStatus task_status;
	ReturnResult return_data;
} WorkHandle;

typedef struct
{
	ThreadTask_fun function;
	WorkHandleID handle_id;
	TaskWorkStatus status;
	WorkTask_Flags flags;
	char arg_data_buffer[THREAD_TASK_ARG_DATA_MAX_SIZE];
	void* arg_ptr;
} WorkTask;

typedef struct
{
	WorkTask tasks[MAX_TASKS_IN_QUEUE];

	int task_count;

	int task_pos;
	int thread_pos;
} WorkQueue;

typedef struct
{
	HANDLE handle;
	HANDLE mutex;
	WorkTask* tasks[MAX_TASKS_PER_THREAD];
	int assigned_tasks;
	ThreadStatus status;
	bool exit_request;
} WorkerThread;

typedef struct
{
	WorkHandle handles[MAX_HANDLES];
	bool active_handles_bitset[MAX_HANDLES];
	int pos;
} HandleManager;

typedef struct
{
	WorkerThread threads[MAX_WORK_THREADS];
	int active_thread_count;
	ThreadCoreStatus status;
} ThreadCore;

static ThreadCore thread_core;
static WorkQueue queue;
static HandleManager handle_manager;

static int Thread_ProjectPriorityToWindowsPriority(WorkTask_Flags p_flag)
{
	if (p_flag & TASK_FLAG__PRIORITY_ABOVE_NORMAL)
	{
		return THREAD_PRIORITY_ABOVE_NORMAL;
	}
	if (p_flag & TASK_FLAG__PRIORITY_HIGHEST)
	{
		return THREAD_PRIORITY_HIGHEST;
	}
	if (p_flag & TASK_FLAG__PRIORITY_BELOW_NORMAL)
	{
		return THREAD_PRIORITY_BELOW_NORMAL;
	}
	if (p_flag & TASK_FLAG__PRIORITY_LOWEST)
	{
		return THREAD_PRIORITY_BELOW_NORMAL;
	}

	return THREAD_PRIORITY_NORMAL;
}

static void Thread_ShutdownThread(WorkerThread* thread)
{
	//thread is not active
	if (thread->status != TS__LAUNCHED)
	{
		return;
	}
	//this enum forces thread to exit if called from other thread
	thread->status = TS__EXIT;

	WaitForSingleObject(thread->handle, INFINITE);

	//close handle
	CloseHandle(thread->handle);

	CloseHandle(thread->mutex);

	thread_core.active_thread_count--;
	
	memset(thread, 0, sizeof(WorkerThread));
}

static void Thread_SignalTaskCompleted(WorkerThread* _thread, WorkTask* _task, void* _return_data)
{
	WaitForSingleObject(_thread->mutex, INFINITE);

	_thread->assigned_tasks--;
	if (_thread->assigned_tasks < 0)
	{
		_thread->assigned_tasks = 0;
	}
	_task->status = TWS__EMPTY;
	handle_manager.handles[_task->handle_id].return_data.int_value = (long long)_return_data;
	handle_manager.handles[_task->handle_id].return_data.mem_value = _return_data;
	handle_manager.handles[_task->handle_id].task_status = (_task->flags & TASK_FLAG__FIRE_AND_FORGET) ? TWS__EMPTY : TWS__COMPLETED;
	handle_manager.active_handles_bitset[_task->handle_id] = (_task->flags & TASK_FLAG__FIRE_AND_FORGET) ? false : true;

	ReleaseMutex(_thread->mutex);
}

static WorkTask* Thread_GetNextTask(TaskDispatchResult* r_dispatchResult, WorkerThread* _thread)
{
	if (thread_core.status == TCS__FORCE_EXIT || _thread->status == TS__EXIT)
	{
		*r_dispatchResult = TDR__EXIT;
		return NULL;
	}
	
	//check if we have any assigned work
	if (_thread->assigned_tasks > 0)
	{
		WorkTask* task = NULL;
		for (int i = 0; i < MAX_TASKS_PER_THREAD; i++)
		{
			if (_thread->tasks[i] && _thread->tasks[i]->status == TWS__READY_FOR_WORK)
			{
				task = _thread->tasks[i];
				break;
			}
		}
		//is the task ready for work?
		if (task && task->status == TWS__READY_FOR_WORK)
		{
			//set the the tread priority if needed
			int task_priority_win = Thread_ProjectPriorityToWindowsPriority(task->flags);
			if (GetThreadPriority(_thread->handle) != task_priority_win)
			{
				SetThreadPriority(_thread->handle, task_priority_win);
			}
			WaitForSingleObject(_thread->mutex, INFINITE);
			task->status = TWS__WORKING;
			ReleaseMutex(_thread->mutex);

			*r_dispatchResult = TDR__NEW_TASK;
			return task;
		}
		//otherwise we sleep
	}
	
	*r_dispatchResult = TDR__SLEEP;
	return NULL;
}

static void Thread_Loop(WorkerThread* _thread)
{
	float sleep_time = 0;
	WorkTask* task = NULL;
	TaskDispatchResult dispatch_result = 0;
	while (dispatch_result != TDR__EXIT)
	{
		task = Thread_GetNextTask(&dispatch_result, _thread);
		
		switch (dispatch_result)
		{
		case TDR__SLEEP:
		{
			//no task? sleep
			sleep_time += Core_getDeltaTime();
			
			//if sleep time is more than this threshold
			//request exit from the main thread this will shutdown the win handle and free it's data
			if (sleep_time >= THREAD_SHUTDOWN_TIME_THRESHOLD)
			{
				//_thread->exit_request = true;
			}
			else if (sleep_time >= LOW_PRIORITY_TIME_THRESHOLD)
			{
				//if sleep time goes over the threshold
				//set thread priority to lowest
				if (GetThreadPriority(_thread->handle) != THREAD_PRIORITY_LOWEST)
				{
					//SetThreadPriority(_thread->handle, THREAD_PRIORITY_LOWEST);
				}
			}
			continue;
		}
		case TDR__NEW_TASK:
		{
			//reset sleep time
			sleep_time = 0;

			//do the work
			void* return_value = NULL;

			if (task->flags & __INTERNAL_TASK_FLAG__INT_TYPE)
			{
				return_value = (*task->function)(task->arg_ptr);
			}
			else if (task->flags & __INTERNAL_TASK_FLAG__VOID_TYPE)
			{
				return_value = (*task->function)();
			}
			else
			{
				return_value = (*task->function)(task->arg_data_buffer);
			}

			Thread_SignalTaskCompleted(_thread, task, return_value);
			break;
		}
		case TDR__EXIT:
		{
			break;
		}
		}
	}
}

static bool Thread_Create(WorkerThread* _thread)
{
	//check if we can create a thread
	if (thread_core.active_thread_count + 1 > MAX_WORK_THREADS)
	{
		return;
	}
	//zero mem the possible old data
	memset(_thread, 0, sizeof(WorkerThread));

	LPDWORD thread_id; //We don't need to store the id

	//create thread
	_thread->handle = CreateThread
	(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)Thread_Loop,
		_thread,
		0,
		&thread_id
	);

	//failed to create the thread
	if (!_thread->handle)
	{
		return false;
	}

	//create mutex
	_thread->mutex = CreateMutex(NULL, FALSE, NULL);

	//failed to create mutex
	if (!_thread->mutex)
	{
		return false;
	}
	
	_thread->status = TS__LAUNCHED;

	thread_core.active_thread_count++;

	return true;
}

static int Thread_GetEmptyHandleIndex()
{
	int empty_loops = 0;
	for (;;)
	{	
		handle_manager.pos = (handle_manager.pos + 1) % MAX_HANDLES;
		if (handle_manager.handles[handle_manager.pos].task_status == TWS__EMPTY)
		{
			return handle_manager.pos;
		}
		empty_loops++;

		if (empty_loops > 10)
		{
			break;
		}
	}

	return -1;
}

static WorkTask* Thread_GetEmptyTask()
{
	WorkTask* task = &queue.tasks[queue.task_pos];

	//assign task to a thread
	WorkerThread* thread = NULL;

	const int MAX_EMPTY_LOOPS = 5;

	int loops = 0;

	for (;;)
	{
		thread = &thread_core.threads[queue.thread_pos];

		//is the thread not created
		if (thread->status == TS__NOT_LAUNCHED)
		{
			//we create the thread
			if (!Thread_Create(thread))
			{
				return NULL;
			}
		}

		//Is the thread full of tasks already?
		if (thread->assigned_tasks >= MAX_TASKS_PER_THREAD)
		{
			return NULL;
			queue.thread_pos = (queue.thread_pos + 1) % MAX_WORK_THREADS;
			continue;
		}

		if (loops >= MAX_EMPTY_LOOPS)
		{
			return NULL;
		}
		loops++;
			
		//assign to the thread task array
		WaitForSingleObject(thread->mutex, INFINITE);

		for (int i = 0; i < MAX_TASKS_PER_THREAD; i++)
		{
			if (!thread->tasks[i] || thread->tasks[i]->status == TWS__EMPTY)
			{
				thread->tasks[i] = task;
				thread->assigned_tasks = (thread->assigned_tasks + 1) % (MAX_TASKS_PER_THREAD + 1);
				break;
			}
		}
		ReleaseMutex(thread->mutex);

		break;
	}
	
	task->handle_id = Thread_GetEmptyHandleIndex();
	if (task->handle_id == -1)
	{
		return NULL;
	}

	task->status = TWS__EMPTY;
	
	queue.task_pos = (queue.task_pos + 1) % MAX_TASKS_IN_QUEUE;

	return task;
}


WorkHandleID __Internal_Thread_AssignTaskArgCopy(ThreadTask_fun p_taskFun, void* p_argData, size_t p_allocSize, WorkTask_Flags p_taskFlags)
{	
	assert(p_taskFun && "Null task function");
	assert(p_allocSize < THREAD_TASK_ARG_DATA_MAX_SIZE && "The arg data can not exceed the max size");

	WorkTask* task = Thread_GetEmptyTask();

	if (!task)
	{
		return -1;
	}

	if (p_taskFlags & __INTERNAL_TASK_FLAG__INT_TYPE)
	{
		assert(p_allocSize <= 8 && "Invalid int type size");
		task->arg_ptr = p_argData;
	}
	else if (p_allocSize > 0)
	{
		memcpy(task->arg_data_buffer, p_argData, p_allocSize);
	}
	task->function = p_taskFun;
	task->flags = p_taskFlags;

	handle_manager.handles[task->handle_id].task_status = TWS__READY_FOR_WORK;
	handle_manager.active_handles_bitset[task->handle_id] = true;

	//this always must be last
	task->status = TWS__READY_FOR_WORK;

	return task->handle_id;
}

bool Thread_IsCompleted(WorkHandleID p_workHandleID)
{
	assert(p_workHandleID < MAX_HANDLES && "Invalid work handle id");
	assert(handle_manager.active_handles_bitset[p_workHandleID] == true && "Invalid Work Handle id");

	return handle_manager.handles[p_workHandleID].task_status == TWS__COMPLETED;
}

void Thread_ReleaseWorkHandle(WorkHandleID p_workHandleID)
{
	assert(p_workHandleID < MAX_HANDLES && "Invalid work handle id");
	assert(handle_manager.active_handles_bitset[p_workHandleID] == true && "Invalid Work Handle id");

	handle_manager.handles[p_workHandleID].return_data.int_value = 0;
	handle_manager.handles[p_workHandleID].task_status = TWS__EMPTY;
	handle_manager.active_handles_bitset[p_workHandleID] = false;
}

ReturnResult Thread_WaitForResult(WorkHandleID p_workHandleID, size_t p_waitTime)
{
	assert(p_workHandleID < MAX_HANDLES && "Invalid work handle id");
	assert(handle_manager.active_handles_bitset[p_workHandleID] == true && "Invalid Work Handle id");

	ReturnResult ret_value;
	size_t wait_counter = 0;
	while (!Thread_IsCompleted(p_workHandleID))
	{
		wait_counter++;

		if (wait_counter >= p_waitTime)
		{
			ret_value.int_value = -1;
			return ret_value;
		}
	}
	ret_value = handle_manager.handles[p_workHandleID].return_data;

	Thread_ReleaseWorkHandle(p_workHandleID);

	return ret_value;
}


/*
	~~~~~~~~~~~~~~~~~
	NON GLOBAL FUNCTIONS
	~~~~~~~~~~~~~~~~~
*/
void ThreadCore_ShutdownInactiveThreads()
{
	for (int i = 0; i < MAX_WORK_THREADS; i++)
	{
		WorkerThread* thread = &thread_core.threads[i];

		if (thread->status == TS__LAUNCHED && thread->exit_request == true)
		{
			Thread_ShutdownThread(thread);
		}
	}


}

void ThreadCore_Init()
{
	memset(&thread_core, 0, sizeof(ThreadCore));
	memset(&queue, 0, sizeof(WorkQueue));
	memset(&handle_manager, 0, sizeof(HandleManager));

	handle_manager.pos = -1; //this will be increased to 0 on the Thread_GetEmptyHandleIndex() function
	
}

void ThreadCore_Cleanup()
{
	//shut down the threads and cleanup handles
	for (int i = 0; i < MAX_WORK_THREADS; i++)
	{
		Thread_ShutdownThread(&thread_core.threads[i]);
	}
}
