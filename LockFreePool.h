#pragma once
#include <Windows.h>
#include "Tracer.h"

template <class DATA>
class LockFreePool
{
	struct BLOCK_NODE
	{
		__int64 pad1;
		DATA data;
		BLOCK_NODE* next;
		__int64 pad2;

		BLOCK_NODE()
		{
			pad1 = -1;
			pad2 = -1;
		}
	};

public:
	/// <summary>
	/// 생성자 
	/// </summary>
	/// <param name="block_num"> 초기 블럭 수 </param>
	/// <param name="placement_new"> Alloc 또는 Free 시 생성자 호출 여부(미구현) </param>
	LockFreePool(int block_num, bool placement_new = false);

	virtual ~LockFreePool();

	/// <summary>
	/// 블럭 하나 할당 
	/// 
	/// </summary>
	/// <param name=""></param>
	/// <returns>할당된 DATA 포인터</returns>
	DATA* Alloc(void);

	/// <summary>
	/// 사용중이던 블럭 해제
	/// </summary>
	/// <param name="data"> 반환할 블럭 포인터</param>
	/// <returns> 해당 Pool의 블럭이 아닐 시 false 그 외 true </returns>
	bool Free(DATA* data);

	/// <summary>
	/// 
	/// 
	/// </summary>
	/// <returns>오브젝트 풀에서 생성한 전체 블럭 수</returns>
	int GetCapacity()
	{
		return capacity;
	}
	/// <summary>
	/// 
	/// </summary>
	/// <returns>오브젝트 풀에서 제공한 블럭 수</returns>
	int GetUseCount()
	{
		return use_count;
	}

protected:
	BLOCK_NODE* top;

	BLOCK_NODE** pool;

	int capacity;
	alignas(64) unsigned int use_count;

};

template<class DATA>
LockFreePool<DATA>::LockFreePool(int _capacity, bool _placement_new)
{
	capacity = _capacity;
	use_count = 0;


	pool = new BLOCK_NODE * [capacity];

	BLOCK_NODE* next = nullptr;
	for (int i = 0; i < capacity; i++)
	{
		BLOCK_NODE* temp = new BLOCK_NODE;
		temp->next = next;
		next = temp;

		pool[i] = temp;

	}
	top = next;
}

template<class DATA>
LockFreePool<DATA>::~LockFreePool()
{
	for (int i = 0; i < capacity; i++)
	{
		delete pool[i];
	}
	delete[] pool;

}

template<class DATA>
DATA* LockFreePool<DATA>::Alloc()
{
	BLOCK_NODE* old_top;
	BLOCK_NODE* next;
	
	trace(40, NULL, NULL);

	while (1)
	{
		old_top = top;
		trace(41, old_top, NULL);

		next = old_top->next;
		trace(42, next, NULL);

		if (old_top == (BLOCK_NODE*)InterlockedCompareExchangePointer((PVOID*)&top, (PVOID)next, (PVOID)old_top))
		{
			trace(43, old_top, next);
			break;
		}


	}
	InterlockedIncrement((LONG*)&use_count);

	return (DATA*)old_top;
}

template<class DATA>
bool LockFreePool<DATA>::Free(DATA* data)
{
	BLOCK_NODE* old_top;
	BLOCK_NODE* node = (BLOCK_NODE*)data;

	trace(60, node, NULL);

	while (1)
	{
		old_top = top;
		trace(61, old_top, NULL);
		node->next = old_top;
		trace(62, node->next, old_top);
		if (old_top == (BLOCK_NODE*)InterlockedCompareExchangePointer((PVOID*)&top, (PVOID)node, (PVOID)old_top))
		{
			trace(63, old_top, node);
			break;
		}

	}
	
	InterlockedDecrement((LONG*)&use_count);

	return true;
}