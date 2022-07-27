#pragma once
#include <Windows.h>

#include <new>
#include <stdio.h>

#define dfPAD -1
#define dfADDRESS_MASK 0x00007fffffffffff
#define dfADDRESS_BIT 47

template <class DATA>
class LockFreePool
{
	enum
	{
		PAD = 0xABCDABCDABCDABCD
	};

	struct BLOCK_NODE
	{
		unsigned long long front_pad;
		DATA data;
		unsigned long long back_pad;
		BLOCK_NODE* next;
	

		BLOCK_NODE()
		{
			next = nullptr;
		}
	};

public:
	/// <summary>
	/// ������ 
	/// </summary>
	/// <param name="block_num"> �ʱ� �� �� </param>
	/// <param name="placement_new"> Alloc �Ǵ� Free �� ������ ȣ�� ����(�̱���) </param>
	LockFreePool(int block_num);

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

	void Crash()
	{
		int* a = (int*)0;
		*a = 0;

	}

	BLOCK_NODE* top;

	int padding_size;
	alignas(64) unsigned int use_count;
	alignas(64) unsigned int capacity;
	alignas(64) char b;

};

template<class DATA>
LockFreePool<DATA>::LockFreePool(int _capacity)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if ((_int64)si.lpMaximumApplicationAddress != 0x00007ffffffeffff)
	{
		wprintf(L"Maximum Application Address Fault\n");
		Crash();
	}

	capacity = _capacity;
	use_count = 0;

	padding_size = max(sizeof(BLOCK_NODE::front_pad), alignof(DATA));

	for (unsigned int i = 0; i < capacity; i++)
	{
		BLOCK_NODE* temp = (BLOCK_NODE*)_aligned_malloc(sizeof(BLOCK_NODE), alignof(BLOCK_NODE));
		ZeroMemory(&temp->data, sizeof(DATA));
		temp->front_pad = PAD;
		temp->back_pad = PAD;

		temp->next = top;
		top = temp;
	}

}

template<class DATA>
LockFreePool<DATA>::~LockFreePool()
{
	// ��ȯ �ȵ� ���� ����.. ������ ������ ���� ���Ŷ� 
	// �Ҹ� ������ ���α׷� ���� ��
	BLOCK_NODE* top_addr = (BLOCK_NODE*)((unsigned long long)top & dfADDRESS_MASK);
	BLOCK_NODE* temp;

	while (top_addr)
	{
		temp = top_addr->next;
		_aligned_free(top_addr);
		top_addr = temp;
	}

}

template<class DATA>
DATA* LockFreePool<DATA>::Alloc()
{
	unsigned long long old_top;  // �񱳿�
	BLOCK_NODE* old_top_addr; // ���� �ּ�
	BLOCK_NODE* next; // ���� top
	BLOCK_NODE* new_top;
	InterlockedIncrement((LONG*)&use_count);

	

	while (1)
	{
		old_top = (unsigned long long)top;
		old_top_addr = (BLOCK_NODE*)(old_top & dfADDRESS_MASK);

		if (old_top_addr == nullptr)
		{
			InterlockedIncrement(&capacity);
			
			old_top_addr = (BLOCK_NODE*)_aligned_malloc(sizeof(BLOCK_NODE), alignof(BLOCK_NODE));
			ZeroMemory(&old_top_addr->data, sizeof(DATA));
			old_top_addr->front_pad = PAD;
			old_top_addr->back_pad = PAD;
			old_top_addr->next = nullptr;

			break;
		}


		unsigned long long next_cnt = (old_top >> dfADDRESS_BIT) + 1;
		next = old_top_addr->next;

		new_top = (BLOCK_NODE*)((unsigned long long)next | (next_cnt << dfADDRESS_BIT));


		if (old_top == (unsigned long long)InterlockedCompareExchangePointer((PVOID*)&top, (PVOID)new_top, (PVOID)old_top))
		{
			
			break;
		}
	}


	return &old_top_addr->data;
	
}

template<class DATA>
bool LockFreePool<DATA>::Free(DATA* data)
{
	unsigned long long old_top;
	// data�� 8����Ʈ �ʰ��ϸ� ������ �� ���� ����ü �е� ����
	BLOCK_NODE* node = (BLOCK_NODE*)((char*)data - padding_size); 
	BLOCK_NODE* old_top_addr;
	PVOID new_top;

	if (node->front_pad != PAD || node->back_pad != PAD) // ��� �߿� ���� ������ �־����� üũ��
	{
		Crash();
	}
	


	while (1)
	{
		old_top = (unsigned long long)top;
		old_top_addr = (BLOCK_NODE*)(old_top & dfADDRESS_MASK);

		unsigned long long next_cnt = (old_top >> dfADDRESS_BIT) + 1;

		node->next = old_top_addr;

		new_top = (BLOCK_NODE*)((unsigned long long)node | (next_cnt << dfADDRESS_BIT));

		if (old_top == (unsigned long long)InterlockedCompareExchangePointer((PVOID*)&top, (PVOID)new_top, (PVOID)old_top))
		{
			InterlockedDecrement((LONG*)&use_count);
			break;
		}
	}
	
	return true;
}