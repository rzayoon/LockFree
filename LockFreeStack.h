#pragma once
#include <Windows.h>
#include "LockFreePool.h"
#include "Tracer.h"



template<class T>
class LockFreeStack
{
	struct Node
	{
		__int64 front_padding;
		T data;
		Node* next;
		int del_cnt;
		__int64 rear_padding;

		Node()
		{
			char* f = (char*)this - sizeof(__int64);
			char* r = (char*)this + sizeof(Node);
			memcpy(&front_padding, f, sizeof(__int64));
			memcpy(&rear_padding, r, sizeof(__int64));

			next = nullptr;
			del_cnt = 0;
		}

		~Node()
		{

		}
	};


public:

	LockFreeStack();
	~LockFreeStack();

	void Push(T data);
	void Pop(T* data);

private:

	alignas(64) Node* _top;
	LockFreePool<Node> *_pool;

};

template<class T>
inline LockFreeStack<T>::LockFreeStack()
{
	_top = nullptr;
	_pool = new LockFreePool<Node>(10000);
}

template<class T>
inline LockFreeStack<T>::~LockFreeStack()
{
	delete _pool;
}

template<class T>
inline void LockFreeStack<T>::Push(T data)
{
	Node* temp = _pool->Alloc();
	temp->del_cnt = 0;
	//trace(0, temp, NULL);
	temp->data = data;
	Node* top;

	Node* ret;
	while (1)
	{
		top = _top;
		//trace(1, top, NULL);
		temp->next = top;
		//trace(2, NULL, top);

		if (top == (ret = (Node*)InterlockedCompareExchangePointer((PVOID*)&_top, (PVOID)temp, (PVOID)top))) {
			//trace(3, top, temp);
			break;
		}

	}

	return;

} 

template<class T>
inline void LockFreeStack<T>::Pop(T* data)
{
	//trace(20, NULL, NULL);
	Node* top;
	Node* next;
	Node* ret;

	while (1)
	{
		top = _top;
		//trace(21, top, NULL);
		if (top == nullptr)
		{
			int a = 0;
		};
		next = top->next;
		//trace(22, next, NULL);

		if (top == ( ret = (Node*)InterlockedCompareExchangePointer((PVOID*)&_top, (PVOID)next, (PVOID)top) ))
		{
			//trace(23, top, next);
			*data = top->data;
			top->del_cnt--;
			if (top->del_cnt == -2)
				int a = 0;
			_pool->Free(top);
			//trace(24, top, NULL);
			break;
		}
	}

	return;
}
