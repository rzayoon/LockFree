
#include <Windows.h>
#include <conio.h>
#include <stdio.h>
#include "LockFreePool.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"

LockFreeStack<int> stack;

#define dfTHREAD 4
#define dfTHREAD_ALLOC 100
#define dfMEMORY_POOL_MAX dfTHREAD * dfTHREAD_ALLOC 

const int NUM_THREAD = 4;

struct st_TEST_DATA
{
	volatile LONG64 lData;
	volatile LONG64 lCount;
};

LockFreePool<st_TEST_DATA> g_MemoryPool(dfMEMORY_POOL_MAX);

LockFreeQueue<st_TEST_DATA*> g_Queue;


DWORD WINAPI QueueTestWorker(LPVOID param);

DWORD WINAPI worker3(LPVOID param);

bool close = false;



int main()
{
	st_TEST_DATA* data[dfMEMORY_POOL_MAX];

	st_TEST_DATA* q_data[40];
	

	for (int i = 0; i < dfMEMORY_POOL_MAX; i++)
	{
		data[i] = g_MemoryPool.Alloc();
		data[i]->lData = 0x0000000055555555;
		data[i]->lCount = 0;

	}

	for (int i = 0; i < dfMEMORY_POOL_MAX; i++)
	{
		g_MemoryPool.Free(data[i]);
	}

	for (int i = 0; i < 40; i++)
	{
		q_data[i] = new st_TEST_DATA;
		q_data[i]->lCount = 0;
		q_data[i]->lData = 0x0000000055555555;
		g_Queue.Enqueue(q_data[i]);
	}


	HANDLE thread[dfTHREAD];

	for (int i = 0; i < dfTHREAD; i++)
	{
		thread[i] = CreateThread(NULL, 0, QueueTestWorker, 0, 0, NULL);

	}

	while (1)
	{
		char c = _getch();
		if (c == 'q' || c == 'Q')
		{
			close = true;
			break;
		}
	}


	WaitForMultipleObjects(dfTHREAD, thread, TRUE, INFINITE);


	return 0;
}


DWORD WINAPI worker3(LPVOID param)
{

	DWORD id = GetCurrentThreadId();


	st_TEST_DATA* arr[dfTHREAD_ALLOC];

	while (!close)
	{
		for (int i = 0; i < dfTHREAD_ALLOC; i++) {

			arr[i] = g_MemoryPool.Alloc();

			if (arr[i] == nullptr)
				Crash();
			if (arr[i]->lData != 0x0000000055555555)
				Crash();
			if (arr[i]->lCount != 0)
				Crash();

		}
		
		for (int i = 0; i < dfTHREAD_ALLOC; i++)
		{
			InterlockedIncrement64(&arr[i]->lData);
			InterlockedIncrement64(&arr[i]->lCount);

		}
		
		Sleep(0);

		for (int i = 0; i < dfTHREAD_ALLOC; i++)
		{
			if (arr[i]->lData != 0x0000000055555556)
			{
				Crash();
			}
			if (arr[i]->lCount != 1)
			{
				Crash();
			}
		}

		for (int i = 0; i < dfTHREAD_ALLOC; i++)
		{
			InterlockedDecrement64(&arr[i]->lData);
			InterlockedDecrement64(&arr[i]->lCount);

		}


		
		Sleep(0);

		for (int i = 0; i < dfTHREAD_ALLOC; i++)
		{
			if (arr[i]->lData != 0x0000000055555555)
			{
				Crash();
			}
			if (arr[i]->lCount != 0)
			{
				Crash();
			}
		}

		for (int i = dfTHREAD_ALLOC - 1; i >= 0; i--) {
			g_MemoryPool.Free(arr[i]);
		}

	}

	return 0;

}

#define dfQueueTest 5
DWORD WINAPI QueueTestWorker(LPVOID param)
{

	DWORD id = GetCurrentThreadId();


	st_TEST_DATA* arr[dfQueueTest];

	while (!close)
	{
		for (int i = 0; i < dfQueueTest; i++) {

			g_Queue.Dequeue(&arr[i]);

			if (arr[i] == nullptr)
				Crash();
			if (arr[i]->lData != 0x0000000055555555)
				Crash();
			if (arr[i]->lCount != 0)
				Crash();
			InterlockedIncrement64(&arr[i]->lCount);

		}

		for (int i = 0; i < dfQueueTest; i++)
		{
			InterlockedIncrement64(&arr[i]->lData);
		}

		for (int i = 0; i < dfQueueTest; i++)
		{
			if (arr[i]->lData != 0x0000000055555556)
			{
				Crash();
			}
			if (arr[i]->lCount != 1)
			{
				Crash();
			}
		}

		Sleep(0);

		for (int i = 0; i < dfQueueTest; i++)
		{
			InterlockedDecrement64(&arr[i]->lData);

		}



		Sleep(0);

		for (int i = 0; i < dfQueueTest; i++)
		{
			if (arr[i]->lData != 0x0000000055555555)
			{
				Crash();
			}
		}
		

		for (int i = dfQueueTest-1; i >= 0; i--) {
			InterlockedDecrement64(&arr[i]->lCount);
			g_Queue.Enqueue(arr[i]);
		}

	}

	return 0;

}