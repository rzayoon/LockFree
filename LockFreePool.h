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
	/// ������ 
	/// </summary>
	/// <param name="block_num"> �ʱ� �� �� </param>
	/// <param name="placement_new"> Alloc �Ǵ� Free �� ������ ȣ�� ����(�̱���) </param>
	LockFreePool(int block_num, bool placement_new = false);

	virtual ~LockFreePool();

	/// <summary>
	/// �� �ϳ� �Ҵ� 
	/// 
	/// </summary>
	/// <param name=""></param>
	/// <returns>�Ҵ�� DATA ������</returns>
	DATA* Alloc(void);

	/// <summary>
	/// ������̴� �� ����
	/// </summary>
	/// <param name="data"> ��ȯ�� �� ������</param>
	/// <returns> �ش� Pool�� ���� �ƴ� �� false �� �� true </returns>
	bool Free(DATA* data);

	/// <summary>
	/// 
	/// 
	/// </summary>
	/// <returns>������Ʈ Ǯ���� ������ ��ü �� ��</returns>
	int GetCapacity()
	{
		return capacity;
	}
	/// <summary>
	/// 
	/// </summary>
	/// <returns>������Ʈ Ǯ���� ������ �� ��</returns>
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