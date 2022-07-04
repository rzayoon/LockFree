#pragma once
#include <Windows.h>
#include "LockFreePool.h"
#include "Tracer.h"



template<class T>
class LockFreeQueue
{
	struct Node
	{
		T data;
		Node* next;
		int del_cnt;

		Node()
		{

			next = nullptr;
			del_cnt = 0;
		}

		~Node()
		{

		}
	};


public:

	LockFreeQueue(unsigned int size = 10000);
	~LockFreeQueue();

	bool Enqueue(T data);
	bool Dequeue(T* data);
	int GetSize();


private:

	alignas(64) Node* _head;
	alignas(64) Node* _tail;

	alignas(64) ULONG64 _size;

	LockFreePool<Node>* _pool;

};

template<class T>
inline LockFreeQueue<T>::LockFreeQueue(unsigned int size)
{
	_size = 0;
	_pool = new LockFreePool<Node>(size + 1);
	_head = _pool->Alloc();

	_tail = _head;
}

template<class T>
inline LockFreeQueue<T>::~LockFreeQueue()
{
	delete _pool;
}

template<class T>
inline bool LockFreeQueue<T>::Enqueue(T data)
{
	Node* node = _pool->Alloc();
	long long old_tail;
	Node* tail;
	Node* new_tail;
	Node* next = nullptr;

	if (node == nullptr)
		return false;

	trace(10, node, (PVOID)data, 0);
	
	node->data = data;
	node->next = nullptr;

	while (true)
	{
		old_tail = (long long)_tail;
		tail = (Node*)(old_tail & dfADDRESS_MASK);
		long long next_cnt = (old_tail >> dfADDRESS_BIT) + 1;

		trace(11, tail, NULL, next_cnt - 1);

		next = tail->next;

		trace(12, next, NULL, next_cnt);

		new_tail = (Node*)((long long)node | (next_cnt << dfADDRESS_BIT));

		if (next == nullptr)
		{
			if (InterlockedCompareExchangePointer((PVOID*)&tail->next, node, next) == next)
			{
				if (next != nullptr)
					Crash();
				trace(13, next, node, next_cnt);
				InterlockedCompareExchangePointer((PVOID*)&_tail, new_tail, (PVOID)old_tail);
				trace(14, tail, node, next_cnt);
				break;

			}

		}
	}
	InterlockedIncrement(&_size);

	return true;
}

template<class T>
inline bool LockFreeQueue<T>::Dequeue(T* data)
{
	if (_size == 0)
		return false;

	trace(30, NULL, NULL, _size);

	while (true)
	{
		long long old_head = (long long)_head;
		Node* head = (Node*)(old_head & dfADDRESS_MASK);
		long long next_cnt = (old_head >> dfADDRESS_BIT) + 1;

		trace(31, head, NULL, next_cnt - 1);

		Node* next = head->next;

		trace(32, next, NULL, next_cnt);

		Node* new_head = (Node*)((long long)next | (next_cnt << dfADDRESS_BIT));

		if (next == nullptr)
		{
			continue;
		}
		else
		{
			*data = next->data;
			trace(33, NULL, (PVOID)*data);
			if (InterlockedCompareExchangePointer((PVOID*)&_head, new_head, (PVOID)old_head) == (PVOID)old_head)
			{
				trace(34, head, next, next_cnt); // 더미 , 새 더미(새 head)

				_pool->Free(head);
				trace(35, head, NULL, 0); // 반환
				break;
			}
		}
	}

	InterlockedDecrement(&_size);
	return true;
}

template<class T>
inline int LockFreeQueue<T>::GetSize()
{
	return _size;
}
