#pragma once

struct DebugNode
{
	unsigned int id;
	unsigned long long seq;
	char info;
	PVOID l_node;
	PVOID r_node;
	long long cnt;
};

void trace(char code, PVOID node1, PVOID node2, long long cnt = 0);


void Crash();