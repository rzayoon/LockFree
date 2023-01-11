#pragma once

#include <Windows.h>

template <class DATA>
class Stack
{
	struct BLOCK_NODE
	{
		DATA data;
		BLOCK_NODE* next;
	};


public:
	Stack();
	virtual ~Stack();

	void Push(DATA in);
	bool Pop(DATA* out);
	void Lock();
	void Unlock();



private:

	BLOCK_NODE* top;
	unsigned int size;
	SRWLOCK lock;

};

template<class DATA>
inline Stack<DATA>::Stack()
{
	top = nullptr;
	InitializeSRWLock(&lock);
}

template<class DATA>
inline Stack<DATA>::~Stack()
{
	while (top)
	{
		BLOCK_NODE* temp = top->next;
		delete top;
		top = temp;
	}
}

template<class DATA>
inline void Stack<DATA>::Push(DATA in)
{
	BLOCK_NODE* node = new BLOCK_NODE;
	node->data = in;
	node->next = top;
	top = node;

	size++;
}

template<class DATA>
inline bool Stack<DATA>::Pop(DATA* out)
{
	if(top == nullptr)
		return false;

	*out = top->data;
	BLOCK_NODE* temp = top;
	top = top->next;
	
	delete temp;

	size--;

	return true;
}

template<class DATA>
inline void Stack<DATA>::Lock()
{
	AcquireSRWLockExclusive(&lock);
	return;
}

template<class DATA>
inline void Stack<DATA>::Unlock()
{
	ReleaseSRWLockExclusive(&lock);
	return;
}
