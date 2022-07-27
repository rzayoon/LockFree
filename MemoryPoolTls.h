#pragma once
#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>

#include "LockFreeQueue.h"
#include "LockFreeStack.h"

template <class DATA>
class MemoryPoolTls
{
	enum
	{
		MAX_THREAD = 200,
		DEFAULT_SIZE = 500
	};

	struct BLOCK_NODE
	{
		DATA data;
		BLOCK_NODE* next;
	};


	class POOL
	{
	public:

		POOL(int _block_num, int _default_size = DEFAULT_SIZE, bool _placement_new = false)
		{
			size = _block_num;
			default_size = _default_size;
			placement_new = _placement_new;



			if (placement_new)
			{
				for (int i = 0; i < size; i++)
				{
					BLOCK_NODE* temp = (BLOCK_NODE*)_aligned_malloc(sizeof(BLOCK_NODE), alignof(BLOCK_NODE));

					temp->next = top;
					top = temp;

				}
			}
			else
			{
				for (int i = 0; i < size; i++)
				{
					BLOCK_NODE* temp = new BLOCK_NODE;

					temp->next = top;
					top = temp;
				}
			}

		}

		~POOL()
		{
			BLOCK_NODE* node;
			if (placement_new)
			{
				while (top)
				{
					node = top;
					top = top->next;
					_aligned_free(node);
				}
			}
			else
			{
				while (top)
				{
					node = top;
					top = top->next;
					delete node;
				}

			}
		}
		BLOCK_NODE* Alloc()
		{
	
			size--;
			BLOCK_NODE* data = top;
			top = top->next;
		
			return data;
		}

		void Renew()
		{
			size = default_size;
			if (placement_new)
			{
				for (int i = 0; i < size; i++)
				{
					BLOCK_NODE* temp = (BLOCK_NODE*)_aligned_malloc(sizeof(BLOCK_NODE), alignof(BLOCK_NODE));

					temp->next = top;
					top = temp;

				}
			}
			else
			{
				for (int i = 0; i < size; i++)
				{
					BLOCK_NODE* temp = new BLOCK_NODE;

					temp->next = top;
					top = temp;
				}
			}

		}

		void Free(BLOCK_NODE* data)
		{
			size++;
			BLOCK_NODE* node = data;
			node->next = top;
			top = node;

			return;
		}

		int GetSize()
		{
			return size;
		}

		void Clear()
		{
			top = nullptr;
			size = 0;
		}

		BLOCK_NODE* top;

		int size;
		int default_size;
		bool placement_new;
	};


	struct THREAD_DATA
	{
		POOL* pool;
		POOL* chunk; // �ʰ��� ������ �뵵
	};

public:


	MemoryPoolTls(int _default_size = DEFAULT_SIZE, bool _placement_new = false)
	{
		tls_index = TlsAlloc();
		if (tls_index == TLS_OUT_OF_INDEXES)
			wprintf(L"%d tls error\n", GetLastError());
		placement_new = _placement_new;

		default_size = _default_size;
	}

	virtual ~MemoryPoolTls()
	{
		BLOCK_NODE* chunk;
		while (chunk_pool.Pop(&chunk))
		{
			BLOCK_NODE* temp;
			if (placement_new)
			{
				while (chunk)
				{
					temp = chunk;
					chunk = temp->next;
					_aligned_free(temp);
				}
			}
			else
			{
				while (chunk)
				{
					temp = chunk;
					chunk = temp->next;
					delete temp;
				}

			}
		}
	}

	DATA* Alloc()
	{
		// TLS�� ���� ������ �κ�
		THREAD_DATA* td = (THREAD_DATA*)TlsGetValue(tls_index);
		if (td == nullptr)
		{
			td = new THREAD_DATA;
			td->pool = new POOL(default_size, default_size);
			td->chunk = new POOL(0, default_size);
			TlsSetValue(tls_index, (LPVOID)td);
		}


		BLOCK_NODE* chunk_top;
		// �Ҵ�
		POOL* td_pool = td->pool;
		
		DATA* ret;
		if (td_pool->size == 0) // Ǯ �� �� ���
		{
			if (chunk_pool.Pop(&chunk_top))
			{ // ���� ûũ ������
				td_pool->top = chunk_top;
				td_pool->size = default_size;

			}
			else // ��Ƶ� ûũ�� ����.
			{
				td_pool->Renew();
				
			}
		}
		ret = (DATA*)td_pool->Alloc();
		
		
		

		return ret;
	}

	bool Free(DATA* data)
	{
		if (data == nullptr) return false;

		THREAD_DATA* td = (THREAD_DATA*)TlsGetValue(tls_index);
		if (td == nullptr)
		{
			td = new THREAD_DATA;
			td->pool = new POOL(default_size, default_size, placement_new);
			td->chunk = new POOL(0, default_size, placement_new);
			TlsSetValue(tls_index, (LPVOID)td);
		}
		int size = default_size;
		POOL* td_pool = td->pool;
		POOL* td_chunk = td->chunk;

		if (td_pool->size == size) //Ǯ �ʰ���
		{
			td_chunk->Free((BLOCK_NODE*)data);

			if (td_chunk->size == size) //ûũ�� ����
			{
				chunk_pool.Push(td_chunk->top);
				td_chunk->Clear();
			}
		}
		else
		{
			td_pool->Free((BLOCK_NODE*)data);
		}

		return true;
	}

private:

	LockFreeStack<BLOCK_NODE*> chunk_pool = LockFreeStack<BLOCK_NODE*>(0);

	int tls_index;
	bool placement_new;
	int default_size;
};