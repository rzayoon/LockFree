#pragma once
#include <Windows.h>


struct DebugNode
{
	DWORD id;
	unsigned int seq;
	char msg[64];
};

void trace(const char* msg);


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
			data = 0;
			next = nullptr;
			del_cnt = 0;
		}

		~Node()
		{

			del_cnt--;
		}
	};


public:

	LockFreeStack();
	~LockFreeStack();

	void Push(T data);
	void Pop(T* data);

private:

	alignas(64) Node* _top;
	CRITICAL_SECTION cs;


};

template<class T>
inline LockFreeStack<T>::LockFreeStack()
{
	_top = nullptr;
	InitializeCriticalSection(&cs);
}

template<class T>
inline LockFreeStack<T>::~LockFreeStack()
{
	while (_top)
	{
		Node* temp = _top;
		_top = _top->next;
		delete temp;
	}
}

template<class T>
inline void LockFreeStack<T>::Push(T data)
{
	Node* temp = new Node;
	temp->data = data;
	Node* top;

	Node* ret;
	while (1)
	{
		trace("push top = _top");
		top = _top;
		trace("push temp->nex = top");
		temp->next = top;

		trace("push begin _top = top");
		if (top == (ret = (Node*)InterlockedCompareExchangePointer((PVOID*)&_top, (PVOID)temp, (PVOID)top))) {
			trace("Push Complete _top = top");
			break;
		}

	}

	return;

}

template<class T>
inline void LockFreeStack<T>::Pop(T* data)
{

	Node* top;
	Node* next;

	Node* ret;
	while (1)
	{
		trace("pop top = _top");
		top = _top;
		if (top == nullptr)
		{
			int a = 0;
		}
		trace("pop next = top->next");
		next = top->next;
		trace("pop begin _top = next");

		if (top == ( ret = (Node*)InterlockedCompareExchangePointer((PVOID*)&_top, (PVOID)next, (PVOID)top) ))
		{
			trace("pop Complete _top = next");
			*data = top->data;
			//delete top;
			break;
		}
	}

	return;
}
