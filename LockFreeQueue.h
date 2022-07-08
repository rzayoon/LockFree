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
	unsigned long long old_tail;
	Node* tail;
	Node* new_tail;
	Node* next = nullptr;
	unsigned long long next_cnt;

	if (node == nullptr)
		return false;

	trace(10, node, (PVOID)data, 0);
	
	node->data = data;
	node->next = nullptr;
	while (true)
	{
		old_tail = (unsigned long long)_tail;
		tail = (Node*)(old_tail & dfADDRESS_MASK);
		next_cnt = (old_tail >> dfADDRESS_BIT) + 1;

		trace(11, tail, NULL, next_cnt - 1);

		next = tail->next;

		trace(12, next, NULL, next_cnt);
		if (tail == next)
			Crash();

		if (next == nullptr)
		{
			if (InterlockedCompareExchangePointer((PVOID*)&tail->next, node, next) == next)
			{
				trace(13, next, node, next_cnt);
				if (_tail == (PVOID)old_tail)
				{
					new_tail = (Node*)((unsigned long long)node | (next_cnt << dfADDRESS_BIT));
					InterlockedCompareExchangePointer((PVOID*)&_tail, new_tail, (PVOID)old_tail);
					trace(14, tail, node, next_cnt);
				}
				else
					trace(17, tail, node, next_cnt);
				break;
			}
		}
		else // 아직 tail을 밀어줘야 할 스레드가 안했으면 지금 한다.
		{
			new_tail = (Node*)((unsigned long long)next | (next_cnt << dfADDRESS_BIT));
			if ((PVOID)old_tail == InterlockedCompareExchangePointer((PVOID*)&_tail, new_tail, (PVOID)old_tail))
				trace(19, tail, new_tail, next_cnt);
			else
				trace(18, tail, new_tail, next_cnt);
		}
		
	}
	InterlockedIncrement(&_size);

	return true;
}

template<class T>
inline bool LockFreeQueue<T>::Dequeue(T* data)
{
	ULONG64 size = InterlockedDecrement(&_size);
	if (size < 0) {
		InterlockedIncrement(&_size);
		*data = nullptr;
		return false;
	}
	unsigned long long old_head;
	Node* head;
	Node* next;
	unsigned long long next_cnt;

	trace(30, NULL, NULL, _size);
	int loop = 0;
	while (true)
	{
		old_head = (unsigned long long)_head;
		head = (Node*)(old_head & dfADDRESS_MASK);
		next_cnt = (old_head >> dfADDRESS_BIT) + 1;

		trace(31, head, NULL, next_cnt - 1);

		next = head->next;

		trace(32, next, NULL, next_cnt);

		if (++loop > 500)
			Crash();

		if (next == nullptr)
		{
			// 정말 없었으면 안들어왔어야 함.
			// 그럼에도 원인이 될 수 있는 상황
			// 1. 지금 보고 있는 head가 다른 스레드에서 dequeue되었고 다시 enque과정에서 next가 null로 쓰여졌다.
			// 2. 한 스레드가 enque에서 노드 하나를 tail로 저장하고 잠시 안돌았음
			//    -> 해당 노드가 queue에서 빠져나감 -> 다시 enqueue에서 next = null 됨
			//    -> 멈춰있던 스레드가 동작하면서 next에 이어버림 -> 사이즈 증가
			//    -> 해당 노드를 연결해야하는 스레드가 정지중.
			//    -> 이 때 디큐 시도하는 스레드는 기다리느라 못하는 상태가 됨.
			//    이 문제는 enqueue 중인 스레드가 일을 마치면 해결이 된다.
			//    하지만 다른 스레드 때문에 해야할 일을 못하는 것은 락프리의 목적에 위배된다.
			//    next가 null로 읽히면 1, 2의 상황 구분하지 않고 그냥 없는 것으로 본다..
			continue;

		}
		else
		{
			unsigned long long old_tail = (unsigned long long)_tail;
			if (old_head == old_tail) // cnt까지 일치하면 같은 것
			{
				Node* tail = (Node*)(old_tail & dfADDRESS_MASK); 
				unsigned long long tail_cnt = (old_tail >> dfADDRESS_BIT) + 1;
				Node* new_tail = (Node*)((unsigned long long)next | (tail_cnt << dfADDRESS_BIT));
				InterlockedCompareExchangePointer((PVOID*)&_tail, new_tail, (PVOID)old_tail);
				trace(39, tail, next, 0);
			}

			Node* new_head = (Node*)((unsigned long long)next | (next_cnt << dfADDRESS_BIT));
			*data = next->data; // data가 객체인 경우.. 느려질 것 사용자의 문제. template type이 복사 비용이 적은 포인터나 일반 타입이었어야 한다.
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

	return true;
}

template<class T>
inline int LockFreeQueue<T>::GetSize()
{
	return _size;
}
