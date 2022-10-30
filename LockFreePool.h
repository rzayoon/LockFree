#pragma once
#include <Windows.h>

#include <new>
#include <stdio.h>

//#define LOCKFREE_DEBUG


#define dfPAD -1
#define dfADDRESS_MASK 0x00007fffffffffff
#define dfADDRESS_BIT 47

template <class DATA>
class LockFreePool
{
#ifdef LOCKFREE_DEBUG
	enum
	{
		PAD = 0xABCDABCDABCDABCD
	};
#endif

	struct BLOCK_NODE
	{
#ifdef LOCKFREE_DEBUG
		unsigned long long front_pad;
#endif
		DATA data;
#ifdef LOCKFREE_DEBUG
		unsigned long long back_pad;
#endif
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
	/// <param name="free_list"> capacity �߰� ���� </param>
	LockFreePool(int _block_num, bool _free_list = true);

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
#ifdef LOCKFREE_DEBUG
	int padding_size;
#endif
	alignas(64) unsigned int use_count;
	alignas(64) unsigned int capacity;
	alignas(64) bool free_list;

};

template<class DATA>
LockFreePool<DATA>::LockFreePool(int _capacity, bool _free_list)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if ((_int64)si.lpMaximumApplicationAddress != 0x00007ffffffeffff)
	{
		wprintf(L"Maximum Application Address Fault\n");
		Crash();
	}

	capacity = _capacity;
	free_list = _free_list;
	use_count = 0;

#ifdef LOCKFREE_DEBUG
	padding_size = max(sizeof(BLOCK_NODE::front_pad), alignof(DATA));
#endif

	for (unsigned int i = 0; i < capacity; i++)
	{
		BLOCK_NODE* temp = (BLOCK_NODE*)_aligned_malloc(sizeof(BLOCK_NODE), alignof(BLOCK_NODE));
		ZeroMemory(&temp->data, sizeof(DATA));
#ifdef LOCKFREE_DEBUG
		temp->front_pad = PAD;
		temp->back_pad = PAD;
#endif

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

	

	while (1)
	{
		old_top = (unsigned long long)top;
		old_top_addr = (BLOCK_NODE*)(old_top & dfADDRESS_MASK);

		if (old_top_addr == nullptr)
		{
			if (free_list)
			{
				InterlockedIncrement(&capacity);

				old_top_addr = (BLOCK_NODE*)_aligned_malloc(sizeof(BLOCK_NODE), alignof(BLOCK_NODE));
				ZeroMemory(&old_top_addr->data, sizeof(DATA));
#ifdef LOCKFREE_DEBUG
				old_top_addr->front_pad = PAD;
				old_top_addr->back_pad = PAD;
#endif LOCKFREE_DEBUG
				old_top_addr->next = nullptr;

				break;
			}
			else
			{
				return nullptr;
			}
		}


		unsigned long long next_cnt = (old_top >> dfADDRESS_BIT) + 1;
		next = old_top_addr->next;

		new_top = (BLOCK_NODE*)((unsigned long long)next | (next_cnt << dfADDRESS_BIT));


		if (old_top == (unsigned long long)InterlockedCompareExchangePointer((PVOID*)&top, (PVOID)new_top, (PVOID)old_top))
		{
			
			break;
		}
	}

	InterlockedIncrement((LONG*)&use_count);

	return &old_top_addr->data;
	
}

template<class DATA>
bool LockFreePool<DATA>::Free(DATA* data)
{
	unsigned long long old_top;

#ifdef LOCKFREE_DEBUG
	// data�� 8����Ʈ �ʰ��ϸ� ������ �� ���� ����ü �е� ����
	BLOCK_NODE* node = (BLOCK_NODE*)((char*)data - padding_size); 
	if (node->front_pad != PAD || node->back_pad != PAD) // ��� �߿� ���� ������ �־����� üũ��
	{
		Crash();
	}
#else
	BLOCK_NODE* node = (BLOCK_NODE*)data;
#endif
	BLOCK_NODE* old_top_addr;
	PVOID new_top;


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