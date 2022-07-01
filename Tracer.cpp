#include <Windows.h>
#include <stdio.h>
#include "Tracer.h"


DebugNode buf[65536];

alignas(64) LONG pos = 0;
const unsigned int mask = 0xFFFF;

void trace(char code, PVOID node1, PVOID node2)
{
	unsigned int seq = InterlockedIncrement(&pos);
	unsigned int _pos = seq & mask;

	buf[_pos].id = GetCurrentThreadId();
	buf[_pos].seq = seq;
	buf[_pos].info = code;
	buf[_pos].l_node = node1;
	buf[_pos].r_node = node2;
}

void Crash()
{
	trace(99, NULL, NULL);
	
	printf("Crash!");

	Sleep(INFINITE);

	int a = 0;
	// suspend other threads, and run to Write file

}