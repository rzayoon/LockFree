#include <Windows.h>
#include <conio.h>
#include "LockFreeStack.h"
#include "LockFreePool.h"

struct TestNode
{
	__int64 front_padding;
	__int64 data;
	__int64 rear_padding;

	TestNode()
	{
		char* f = (char*)this - sizeof(__int64);
		char* r = (char*)this + sizeof(TestNode);
		memcpy(&front_padding, f, sizeof(__int64));
		memcpy(&rear_padding, r, sizeof(__int64));
		data = 0;
	}

	~TestNode()
	{

	}
};

LockFreePool<TestNode> testpool(4000);

const int NUM_THREAD = 4;

DWORD WINAPI worker(LPVOID param);
bool close = false;


int main()
{
	HANDLE thread[NUM_THREAD];

	for (int i = 0; i < NUM_THREAD; i++)
	{
		thread[i] = CreateThread(NULL, 0, worker, 0, 0, NULL);

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


	WaitForMultipleObjects(NUM_THREAD, thread, TRUE, INFINITE);


	return 0;
}

DWORD WINAPI worker(LPVOID param)
{
	DWORD id = GetCurrentThreadId();


	TestNode* arr[1000];

	while (!close)
	{
		for (int i = 0; i < 1000; i++) {
			
			arr[i] = testpool.Alloc();
		}

		Sleep(INFINITE);

		for (int i = 0; i < 1000; i++) {
			testpool.Free(arr[i]);
		}
	}


	return 0;
}