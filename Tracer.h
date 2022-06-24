#pragma once

struct DebugNode
{
	unsigned int id;
	unsigned int seq;
	char info;
	PVOID l_node;
	PVOID r_node;
};

void trace(char code, PVOID node1, PVOID node2);
