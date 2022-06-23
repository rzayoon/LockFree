#include <Windows.h>
#include <conio.h>
#include "LockFreeStack.h"



LockFreeStack<int> stack;

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

	int item = 1;
	int ret;

	while (!close)
	{
		for (int i = 0; i < 1000; i++) {
			stack.Push(item);
			++item;
		}

		for (int i = 0; i < 1000; i++) {
			stack.Pop(&ret);
		}
	}


	return 0;
}