#include <Windows.h>
#include <stdio.h>
#include "Tracer.h"


DebugNode buf[65536];

alignas(64) LONG pos = 0;
const unsigned int mask = 0xFFFF;



void trace(char code, PVOID node1, PVOID node2, long long cnt)
{
	unsigned int seq = InterlockedIncrement(&pos);
	unsigned int _pos = seq & mask;

	buf[_pos].id = GetCurrentThreadId();
	buf[_pos].seq = seq;
	buf[_pos].info = code;
	buf[_pos].l_node = node1;
	buf[_pos].r_node = node2;
	buf[_pos].cnt = cnt;
}

void Crash()
{
	trace(99, NULL, NULL); // 99 로그 밑으로는 무시
	
	int a = 0;
	// suspend other threads, and run to Write file

	FILE* fp;

	fopen_s(&fp, "debug.csv", "w");
	if (fp == nullptr)
		return;
	fprintf_s(fp, "index,thread,seq,act,l_node,r_node,cnt\n");
	for (int i = 0; i < mask; i++)
	{
		fprintf_s(fp, "%d,%d,%d,%d,0x%016p,0x%016p,%lld\n",
			i, buf[i].id, buf[i].seq, buf[i].info, buf[i].l_node, buf[i].r_node, buf[i].cnt);
	}
	fclose(fp);

	a = 1;
}