#pragma once

#include <Windows.h>

template <class DATA>
class Queue
{
	struct BLOCK_NODE
	{
		DATA data;
		BLOCK_NODE* next;
	};


public:
	Queue();
	virtual ~Queue();

	void Push(DATA in);
	bool Pop(DATA* out);
	void Lock();
	void Unlock();



private:

	BLOCK_NODE* head;
	BLOCK_NODE* tail;
	unsigned int size;
	SRWLOCK lock;

};

template<class DATA>
inline Queue<DATA>::Queue()
{
	
	InitializeSRWLock(&lock);
}

template<class DATA>
inline Queue<DATA>::~Queue()
{
	while (head)
	{
		BLOCK_NODE* temp = head->next;
		delete head;
		head = temp;
	}
}

template<class DATA>
inline void Queue<DATA>::Push(DATA in)
{
	BLOCK_NODE* node = new BLOCK_NODE;
	node->data = in;
	node->next = nullptr;

	if (head == nullptr)
	{
		head = node;
		tail = node;
	}
	else
	{
		tail->next = node;
		tail = node;
	}

	size++;
}

template<class DATA>
inline bool Queue<DATA>::Pop(DATA* out)
{
	if (head == nullptr)
		return false;

	*out = head->data;
	BLOCK_NODE* temp = head;
	head = temp->next;

	if (tail == temp)
	{
		tail = nullptr;
	}

	delete temp;
	size--;

	return true;
}

template<class DATA>
inline void Queue<DATA>::Lock()
{
	AcquireSRWLockExclusive(&lock);
	return;
}

template<class DATA>
inline void Queue<DATA>::Unlock()
{
	ReleaseSRWLockExclusive(&lock);
	return;
}
