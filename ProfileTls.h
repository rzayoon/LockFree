#pragma once
#include <Windows.h>

#define PROFILE_TLS

#define MEGA_ARG 1000000.0


#ifdef PROFILE_TLS
	#define PRO_BEGIN(TagName) ProfileBegin(TagName)
	#define PRO_END(TagName) ProfileEnd(TagName)
#else
	#define PRO_BEGIN(TagName)
	#define PRO_END(TagName)
#endif

#define TLS_MAX_THREAD 40
#define MAX_PROFILE 50
#define PRO_TAG_SIZE 64


struct stPROFILE
{
	alignas(64) bool flag;
	wchar_t name[PRO_TAG_SIZE];

	alignas(64) LARGE_INTEGER startTime;


	alignas(64) __int64 totalTime;
	alignas(64) __int64 min[2];
	alignas(64) __int64 max[2];
	alignas(64) __int64 call;
	alignas(64) char end;
};

struct THREAD_PROFILE
{
	int thread_id;
	stPROFILE profiles[MAX_PROFILE];
	int idx;
	SRWLOCK srw;
};


void ProfileBegin(const wchar_t* tag);
void ProfileEnd(const wchar_t* tag);

void ProfileDataOutText(const wchar_t* szFileName);
void ProfilePrint();

void ProfileReset(void);


class Profile
{
public:
	Profile(const wchar_t* tag)
	{
		PRO_BEGIN(tag);
		_tag = tag;
	}

	~Profile()
	{
		PRO_END(_tag);
	}


private:
	const wchar_t* _tag;

};

