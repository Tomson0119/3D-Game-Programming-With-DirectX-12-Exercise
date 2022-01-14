#include "stdafx.h"
#include "gameTimer.h"

GameTimer::GameTimer()
	: mElapsedTime(0.0), mBaseTime(0),
	  mPausedTime(0), mStopTime(0), mFPS(0.0f),
	  mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 frequencyPerSec;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&frequencyPerSec));
	mSecondsPerCount = 1.0 / static_cast<double>(frequencyPerSec);
}

void GameTimer::Start()
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

void GameTimer::Stop()
{
	if (!mStopped)
	{
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&mStopTime));
		mStopped = true;
	}
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void GameTimer::Tick()
{
	if (mStopped)
	{
		mElapsedTime = 0;
		return;
	}

	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&mCurrTime));

	mElapsedTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
	mPrevTime = mCurrTime;

	// Prevent not to be negative
	if (mElapsedTime < 0.0)
		mElapsedTime = 0.0;
}

float GameTimer::TotalTime() const
{
	if (mStopped)
		return static_cast<float>((mStopTime - mPausedTime - mBaseTime) * mSecondsPerCount);
	else
		return static_cast<float>((mCurrTime - mPausedTime - mBaseTime) * mSecondsPerCount);
}

float GameTimer::ElapsedTime() const
{
	return static_cast<float>(mElapsedTime);
}

float GameTimer::CurrentTime() const
{
	return static_cast<float>(mCurrTime);
}


