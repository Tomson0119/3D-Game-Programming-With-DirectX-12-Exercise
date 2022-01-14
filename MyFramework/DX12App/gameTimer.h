#pragma once

class GameTimer
{
public:
	GameTimer();
	virtual ~GameTimer() { }

	void Start();
	void Stop();
	void Reset();
	void Tick();

	float TotalTime() const;
	float ElapsedTime() const;
	float CurrentTime() const;

private:
	double mSecondsPerCount;
	double mElapsedTime;

	float mFPS;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mCurrTime;
	__int64 mPrevTime;	
	
	bool mStopped;
};