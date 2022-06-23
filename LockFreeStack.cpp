#include "LockFreeStack.h"


DebugNode buf[4096];

alignas(64) LONG pos = 0;
const unsigned int mask = 0xFFF;

void trace(const char* msg)
{
	unsigned int seq = InterlockedIncrement(&pos);
	unsigned int _pos = seq & mask;

	buf[_pos].id = GetCurrentThreadId();
	buf[_pos].seq = seq;
	memcpy(buf[_pos].msg, msg, 64);
}