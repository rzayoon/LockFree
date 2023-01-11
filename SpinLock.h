#pragma once
#include <Windows.h>


class SpinLock
{
public:

	SpinLock()
	{
		lock_flag = false;
	}

	virtual ~SpinLock()
	{

	}

	void Lock()
	{
		while (InterlockedExchange8((char*)&lock_flag, true) != false)
		{
			YieldProcessor();
		}
		return;
	}

	void Unlock()
	{
		lock_flag = false;
		return;
	}

private:


	bool lock_flag;
};