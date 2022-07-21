#pragma once
#include <Windows.h>

#include <new>


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

	void Crash()
	{
		int* a = (int*)0;
		a = 0;

	}

	BLOCK_NODE* top;

	bool placement_new;
	alignas(64) unsigned int use_count;
	alignas(64) unsigned int capacity;
	alignas(64) char b;

};

template<class DATA>
LockFreePool<DATA>::LockFreePool(int _capacity, bool _placement_new)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if ((_int64)si.lpMaximumApplicationAddress != 0x00007ffffffeffff)
	{
		printf("Maximum Application Address Fault\n");
		Crash();
	}

	placement_new = _placement_new;
	capacity = _capacity;
	use_count = 0;

	if (placement_new)
	{
		for (unsigned int i = 0; i < capacity; i++)
		{
			BLOCK_NODE* temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
			temp->front_pad = PAD;
			temp->back_pad = PAD;

			temp->next = top;
			top = temp;
		}
	}
	else
	{
		for (unsigned int i = 0; i < capacity; i++)
		{
			BLOCK_NODE* temp = new BLOCK_NODE;
			temp->front_pad = PAD;
			temp->back_pad = PAD;

			temp->next = top;
			top = temp;

		}
	}


}

template<class DATA>
LockFreePool<DATA>::~LockFreePool()
{
	// 반환 안된 노드는 포기.. 어차피 전역에 놓고 쓸거라 
	// 소멸 시점은 프로그램 종료 때
	BLOCK_NODE* top_addr = (BLOCK_NODE*)((unsigned long long)top & dfADDRESS_MASK);
	BLOCK_NODE* temp;
	if (placement_new)
	{
		while (top_addr)
		{
			temp = top_addr->next;
			free(top_addr);
			top_addr = temp;
		}
	}
	else
	{
		while (top_addr)
		{
			temp = top_addr->next;
			delete top_addr;
			top_addr = temp;
		}
	}
}

template<class DATA>
DATA* LockFreePool<DATA>::Alloc()
{
	unsigned long long old_top;  // 비교용
	BLOCK_NODE* old_top_addr; // 실제 주소
	BLOCK_NODE* next; // 다음 top
	BLOCK_NODE* new_top;
	InterlockedIncrement((LONG*)&use_count);

	

	while (1)
	{
		old_top = (unsigned long long)top;
		old_top_addr = (BLOCK_NODE*)(old_top & dfADDRESS_MASK);

		if (old_top_addr == nullptr)
		{
			InterlockedIncrement(&capacity);
			if (placement_new)
			{
				old_top_addr = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
				old_top_addr->front_pad = PAD;
				old_top_addr->back_pad = PAD;
			}
			else
			{
				old_top_addr = new BLOCK_NODE;
				old_top_addr->front_pad = PAD;
				old_top_addr->back_pad = PAD;
			}
		
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

	if (placement_new)
	{
		DATA* ret = &old_top_addr->data;
		new(ret) DATA;
		return ret;
	}
	else
	{
		return &old_top_addr->data;
	}
}

template<class DATA>
bool LockFreePool<DATA>::Free(DATA* data)
{
	unsigned long long old_top;
	// data가 8바이트 초과하면 문제될 수 있음 구조체 패딩 관련
	BLOCK_NODE* node = (BLOCK_NODE*)((char*)data - sizeof(BLOCK_NODE::front_pad)); 
	BLOCK_NODE* old_top_addr;
	PVOID new_top;

	if (placement_new)
	{
		data->~DATA();
	}

	if (node->front_pad != PAD || node->back_pad != PAD) // 사용 중에 버퍼 오버런 있었는지 체크용
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