#include "myGameTimer.h"
#include <Windows.h>

MyGameTimer::MyGameTimer()
	: mSecondsPerCount(0.0), mDeltaTime(0.0),
	  mBaseTime(0), mPausedTime(0), mStopTime(0),
	  mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
	mSecondsPerCount = 1.0 / static_cast<double>(countsPerSec);
}

float MyGameTimer::TotalTime() const
{
	if (mStopped)
		return static_cast<float>((mStopTime - mBaseTime - mPausedTime) * mSecondsPerCount);
	else
		return static_cast<float>((mCurrTime - mBaseTime - mPausedTime) * mSecondsPerCount);
}

float MyGameTimer::DeltaTime() const
{
	return static_cast<float>(mDeltaTime);
}

void MyGameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void MyGameTimer::Start()
{
	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	if (mStopped)
	{
		mPausedTime += (currTime - mStopTime);
		mPrevTime = currTime;
		mStopTime = 0;
		mStopped = false;
	}
}

void MyGameTimer::Stop()
{
	if (!mStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

		mStopTime = currTime;
		mStopped = true;
	}
}

void MyGameTimer::Tick()
{
	if (mStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));
	mCurrTime = currTime;

	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
	mPrevTime = mCurrTime;

	// Force nonnegative. The DXSDK's CDXUTTimer mention that if the
	// processor goes into a power save mode or we get shuffled to another
	// processor, then mDeltaTime can be negative.
	if (mDeltaTime < 0.0)
		mDeltaTime = 0.0;
}
