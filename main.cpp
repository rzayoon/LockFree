
#include <Windows.h>
#include <conio.h>
#include <stdio.h>
#include "LockFreePool.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include "MemoryPoolTls.h"
#include "Tracer.h"
#include "Queue.h"
#include "SpinLock.h"

#include "ProfileTls.h"
#include "TextParser.h"

LockFreeStack<int> stack;

#define dfTHREAD_MAX 32
#define dfTHREAD_ALLOC_MAX 5000
//#define dfMEMORY_POOL_MAX dfTHREAD * dfTHREAD_ALLOC 

int dfTHREAD;
int dfTHREAD_ALLOC;
int dfMEMORY_POOL_MAX;


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

LockFreeQueue<st_TEST_DATA*> g_Queue(1, true);
Queue<st_TEST_DATA*> g_Q;
long long g_tps;

int g_interval;

DWORD WINAPI QueueTestWorker(LPVOID param);

DWORD WINAPI worker3(LPVOID param);


DWORD WINAPI Giver(LPVOID param);
DWORD WINAPI Getter(LPVOID param);

DWORD WINAPI PerfTest(LPVOID param);   // lockfree
DWORD WINAPI PerfTest2(LPVOID param);  // srw
DWORD WINAPI PerfTest3(LPVOID param);  // spin

DWORD WINAPI Interrupter(LPVOID param)
{
	while (1)
	{
		int a = 0;
		a++;
	}
	return 0;

}
int main()
{
	DWORD(WINAPI *funcptr[])(LPVOID) = { PerfTest, PerfTest2, PerfTest3 };


	TextParser parser;
	parser.LoadFile("config.txt");
	
	int thread_num;
	int thread_alloc;
	int inter_num;
	int test_code;

	parser.GetValue("num", &thread_num);
	parser.GetValue("thread_alloc", &thread_alloc);
	parser.GetValue("code", &test_code);
	parser.GetValue("Interrupter", &inter_num);
	parser.GetValue("interval", &g_interval);

	dfMEMORY_POOL_MAX = thread_alloc * thread_num;
	dfTHREAD = thread_num;
	dfTHREAD_ALLOC = thread_alloc;




	HANDLE thread[dfTHREAD_MAX];

	for (int i = 0; i < dfMEMORY_POOL_MAX; i++)
	{
		g_Queue.Enqueue(new st_TEST_DATA);
		g_Q.Push(new st_TEST_DATA);
	}

	for (int i = 0; i < inter_num; i++)
	{
		CreateThread(NULL, 0, Interrupter, 0, 0, NULL);

	}

	for (int i = 0; i < dfTHREAD; i++)
	{
		thread[i] = CreateThread(NULL, 0, funcptr[test_code], 0, 0, NULL);

	}
	//thread[0] = CreateThread(NULL, 0, Giver, 0, 0, NULL);
	//thread[1] = CreateThread(NULL, 0, Getter, 0, 0, NULL);


	wchar_t filename[64];
	wsprintf(filename, L"Profile_%dTh_%d_%dw", thread_num, test_code, inter_num);
	long long total_tps = 0;
	long long tick = GetTickCount64();
	long long cnt = 0;
	while (1)
	{
		if (_kbhit()) {
			char c = _getch();
			if (c == 'q' || c == 'Q')
			{
				close = true;
				break;
			}
			if (c == 'r' || c == 'R')
			{
				ProfileReset();
			}
			if (c == 'o' || c == 'O')
			{
				ProfileDataOutText(filename);
			}

		}
		Sleep(1000);

		long long tps = InterlockedExchange((ULONG64*)&g_tps, 0);
		long long now_tick = GetTickCount64();
		cnt++;
		tps /= double(now_tick - tick) / 1000;
		total_tps += tps;

		

		wprintf(L"Test code : %d | thread_alloc : %d | Wack thread : %d\n", test_code, thread_alloc, inter_num);
		wprintf(L"Dequeue avg TPS : %lld... [%lld Tick]\n", (total_tps / cnt * 100), now_tick - tick);
		tick = now_tick;

		ProfilePrint();
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

	int cnt = 0;
	st_TEST_DATA* arr[dfTHREAD_ALLOC_MAX];

	while (1)
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

		Sleep(0);

		for (int i = 0; i <= dfTHREAD_ALLOC; i++)
		{
			InterlockedIncrement64(&arr[i]->lData);
		}

		for (int i = 0; i <= dfTHREAD_ALLOC; i++)
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

		for (int i = 0; i <= dfTHREAD_ALLOC; i++)
		{
			InterlockedDecrement64(&arr[i]->lData);

		}



		Sleep(0);

		for (int i = 0; i <= dfTHREAD_ALLOC; i++)
		{
			if (arr[i]->lData != 0x0000000055555555)
			{
				Crash();
			}
		}
		

		for (int i = dfTHREAD_ALLOC - 1; i >= 0; i--) {
			InterlockedDecrement64(&arr[i]->lCount);
			g_Queue.Enqueue(arr[i]);
		}

		cnt++;
	}

	return 0;

}

DWORD WINAPI PerfTest(LPVOID param)
{


	DWORD id = GetCurrentThreadId();


	st_TEST_DATA* arr[dfTHREAD_ALLOC_MAX];

	wchar_t sz_Deq[64];
	wchar_t sz_Enq[64];
	wsprintf(sz_Deq, L"Deq %d", dfTHREAD_ALLOC);
	wsprintf(sz_Enq, L"Enq %d", dfTHREAD_ALLOC);

	while (!close)
	{
		PRO_BEGIN(sz_Deq);
		for (int i = 0; i < dfTHREAD_ALLOC; i++) {

			while (!g_Queue.Dequeue(&arr[i]))
			{

			}
			for (int i = 0; i < g_interval; i++)
			{

			}
		}
		PRO_END(sz_Deq);
		InterlockedIncrement64(&g_tps);


		PRO_BEGIN(sz_Enq);

		for (int i = dfTHREAD_ALLOC - 1; i >= 0; i--)
		{

			g_Queue.Enqueue(arr[i]);
			for (int i = 0; i < g_interval; i++)
			{

			}
		}
		PRO_END(sz_Enq);
		InterlockedIncrement64(&g_tps);
	}

	return 0;


}

DWORD WINAPI PerfTest2(LPVOID param)
{


	DWORD id = GetCurrentThreadId();


	st_TEST_DATA* arr[dfTHREAD_ALLOC_MAX];

	wchar_t sz_Deq[64];
	wchar_t sz_Enq[64];
	wsprintf(sz_Deq, L"Deq %d", dfTHREAD_ALLOC);
	wsprintf(sz_Enq, L"Enq %d", dfTHREAD_ALLOC);

	while (!close)
	{
		//Sleep(0);
		PRO_BEGIN(sz_Deq);
		for (int i = 0; i < dfTHREAD_ALLOC; i++) {

			g_Q.Lock();
			g_Q.Pop(&arr[i]);
			g_Q.Unlock();
			for (int i = 0; i < g_interval; i++)
			{

			}
		}
		PRO_END(sz_Deq);
		InterlockedIncrement64(&g_tps);
		//Sleep(0);
		/*int a = 1000;
		while (a--)
		{

		}*/

		PRO_BEGIN(sz_Enq);

		for (int i = dfTHREAD_ALLOC - 1; i >= 0; i--)
		{
			g_Q.Lock();
			g_Q.Push(arr[i]);
			g_Q.Unlock();
			for (int i = 0; i < g_interval; i++)
			{

			}
		}
		PRO_END(sz_Enq);
		InterlockedIncrement64(&g_tps);

	}

	return 0;


}

SpinLock spin;

DWORD WINAPI PerfTest3(LPVOID param)
{


	DWORD id = GetCurrentThreadId();


	st_TEST_DATA* arr[dfTHREAD_ALLOC_MAX];

	wchar_t sz_Deq[64];
	wchar_t sz_Enq[64];
	wsprintf(sz_Deq, L"Deq %d", dfTHREAD_ALLOC);
	wsprintf(sz_Enq, L"Enq %d", dfTHREAD_ALLOC);

	while (!close)
	{
		//Sleep(0);
		PRO_BEGIN(sz_Deq);
		for (int i = 0; i < dfTHREAD_ALLOC; i++) {

			spin.Lock();
			g_Q.Pop(&arr[i]);
			spin.Unlock();
			
		}
		PRO_END(sz_Deq);
		InterlockedIncrement64(&g_tps);
		//Sleep(0);
		/*int a = 1000;
		while (a--)
		{

		}*/

		PRO_BEGIN(sz_Enq);

		for (int i = dfTHREAD_ALLOC - 1; i >= 0; i--)
		{
			spin.Lock();
			g_Q.Push(arr[i]);
			spin.Unlock();
			
		}
		PRO_END(sz_Enq);
		InterlockedIncrement64(&g_tps);
	}

	return 0;


}