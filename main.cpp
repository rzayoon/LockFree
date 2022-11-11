
#include <Windows.h>
#include <conio.h>
#include <stdio.h>
#include "LockFreePool.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include "Tracer.h"

LockFreeStack<int> stack;

#define dfTHREAD 4
#define dfTHREAD_ALLOC 1
#define dfMEMORY_POOL_MAX dfTHREAD * dfTHREAD_ALLOC 

const int NUM_THREAD = 4;

struct st_TEST_DATA
{
	volatile LONG64 lData;
	volatile LONG64 lCount;

	st_TEST_DATA()
	{
		lData = 0x0000000055555555;
		lCount = 0;
		//printf("생성자 호출!\n");
	}

};

bool close = false;

//LockFreeQueue<st_TEST_DATA*> g_Queue;
LockFreeStack<st_TEST_DATA*> g_Stack(10, false);

LockFreeQueue<st_TEST_DATA*> g_Queue(dfTHREAD_ALLOC* dfTHREAD, false);
int g_tps;

DWORD WINAPI QueueTestWorker(LPVOID param);

DWORD WINAPI worker3(LPVOID param);


DWORD WINAPI Giver(LPVOID param);
DWORD WINAPI Getter(LPVOID param);

int main()
{

	HANDLE thread[dfTHREAD];

	for (int i = 0; i < dfTHREAD_ALLOC * dfTHREAD; i++)
	{
		g_Queue.Enqueue(new st_TEST_DATA);
	}


	for (int i = 0; i < dfTHREAD; i++)
	{
		thread[i] = CreateThread(NULL, 0, QueueTestWorker, 0, 0, NULL);

	}
	//thread[0] = CreateThread(NULL, 0, Giver, 0, 0, NULL);
	//thread[1] = CreateThread(NULL, 0, Getter, 0, 0, NULL);

	while (1)
	{
		if (_kbhit()) {
			char c = _getch();
			if (c == 'q' || c == 'Q')
			{
				close = true;
				break;
			}
		}
		Sleep(1000);

		int tps = InterlockedExchange((LONG*)&g_tps, 0);
		wprintf(L"Dequeue TPS : %d\n", tps);
		wprintf(L"Queue Use : %lld\n", g_Queue.GetSize());
	}


	WaitForMultipleObjects(dfTHREAD, thread, TRUE, INFINITE);


	return 0;
}

//DWORD WINAPI Giver(LPVOID param)
//{
//	while (1)
//	{
//		int* value = new int;
//		g_Queue.Enqueue(value);
//
//	}
//
//	return 0;
//}
//
//DWORD WINAPI Getter(LPVOID param)
//{
//	while (1)
//	{
//		int* ret;
//		if (g_Queue.Dequeue(&ret)) {
//			delete ret;
//			g_tps++;
//		}
//
//	}
//
//	return 0;
//}

//DWORD WINAPI worker3(LPVOID param)
//{
//
//	DWORD id = GetCurrentThreadId();
//
//
//	st_TEST_DATA* arr[dfTHREAD_ALLOC];
//
//	while (!close)
//	{
//		for (int i = 0; i < dfTHREAD_ALLOC; i++) {
//
//			arr[i] = g_MemoryPool.Alloc();
//
//			if (arr[i] == nullptr)
//				Crash();
//			if (arr[i]->lData != 0x0000000055555555)
//				Crash();
//			if (arr[i]->lCount != 0)
//				Crash();
//
//		}
//		
//		for (int i = 0; i < dfTHREAD_ALLOC; i++)
//		{
//			InterlockedIncrement64(&arr[i]->lData);
//			InterlockedIncrement64(&arr[i]->lCount);
//
//		}
//		
//		Sleep(0);
//
//		for (int i = 0; i < dfTHREAD_ALLOC; i++)
//		{
//			if (arr[i]->lData != 0x0000000055555556)
//			{
//				Crash();
//			}
//			if (arr[i]->lCount != 1)
//			{
//				Crash();
//			}
//		}
//
//		for (int i = 0; i < dfTHREAD_ALLOC; i++)
//		{
//			InterlockedDecrement64(&arr[i]->lData);
//			InterlockedDecrement64(&arr[i]->lCount);
//
//		}
//
//
//		
//		Sleep(0);
//
//		for (int i = 0; i < dfTHREAD_ALLOC; i++)
//		{
//			if (arr[i]->lData != 0x0000000055555555)
//			{
//				Crash();
//			}
//			if (arr[i]->lCount != 0)
//			{
//				Crash();
//			}
//		}
//
//		for (int i = dfTHREAD_ALLOC - 1; i >= 0; i--) {
//			g_MemoryPool.Free(arr[i]);
//		}
//
//	}
//
//	return 0;
//
//}

DWORD WINAPI QueueTestWorker(LPVOID param)
{

	DWORD id = GetCurrentThreadId();


	st_TEST_DATA* arr[dfTHREAD_ALLOC];

	while (!close)
	{
		for (int i = 0; i < dfTHREAD_ALLOC; i++) {

			while (!g_Queue.Dequeue(&arr[i]))
			{

			}

			if (arr[i]->lData != 0x0000000055555555)
				Crash();
			if (arr[i]->lCount != 0)
				Crash();
			InterlockedIncrement64(&arr[i]->lCount);

		}

		for (int i = 0; i < dfTHREAD_ALLOC; i++)
		{
			InterlockedIncrement64(&arr[i]->lData);
		}

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

		Sleep(0);

		for (int i = 0; i < dfTHREAD_ALLOC; i++)
		{
			InterlockedDecrement64(&arr[i]->lData);

		}



		Sleep(0);

		for (int i = 0; i < dfTHREAD_ALLOC; i++)
		{
			if (arr[i]->lData != 0x0000000055555555)
			{
				Crash();
			}
		}
		

		for (int i = dfTHREAD_ALLOC -1; i >= 0; i--) {
			InterlockedDecrement64(&arr[i]->lCount);
			g_Queue.Enqueue(arr[i]);
		}

	}

	return 0;

}