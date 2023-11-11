///////////////////////////////////////////////////////////////////////////////
//! @file		Timer.cpp
//! @brief		Implementation of the Timer class
//! @author		Chip Brommer
///////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////
//
//  Includes:
//          name                        reason included
//          --------------------        ---------------------------------------
#include	"Timer.h"					// Timer class header
//
///////////////////////////////////////////////////////////////////////////////

// Initialize static class variables.
Timer* Timer::mInstance = NULL;

Timer* Timer::GetInstance()
{
	if (mInstance == NULL)
	{
		mInstance = new Timer;
	}

	return mInstance;
}

void Timer::ReleaseInstance()
{
	if (mInstance != NULL)
	{
		delete mInstance;
		mInstance = NULL;
	}
}

void Timer::Reset()
{
	uint32_t now = GetMSecTicks();
	mTickOffset += now;
}

uint32_t Timer::GetMSecTicks()
{
	uint32_t now;
	bool firstTime;

	if (firstTime = !mInitialzied)
	{
		Initialize();
	}

#ifdef _WIN32
	now = mTickCount;
#else
	{
		static uint32_t prevNow;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		now = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		if (!firstTime)
		{
			int32_t elapsed = (int32_t)(now - prevNow);

			if (elapsed < 0)
			{
				tickOffset += elapsed;
#ifdef USE_STDIO
				printf("System time went backwards %d msec\n", -elapsed);
#else
				Log* log = Log::GetInstance();
				log->AddEntry(LOG_WARN, mUser, "System time went backwards %d msec", -elapsed);
#endif // USE_STDIO
			}
		}
		prevNow = now;
	}
#endif

	if (firstTime)
	{
		mTickOffset = now;
	}

	return now - mTickOffset;
}

uint32_t Timer::GetUSecTicks()
{
	// Catch not initialized
	if (!mInitialzied)
	{
		int time = GetMSecTicks();
	}

#if defined _WIN32
	LARGE_INTEGER currentCount;
	QueryPerformanceCounter(&currentCount);
	return (uint32_t)((uint64_t)((((uint64_t)currentCount.QuadPart - mUSecStartTime) * mTimerFactor + 0.5)) & 0xffffffff);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);

	unit64_t uSecs = tv.tv_sec * 1000000 + tv.tv_usec;
	return (uSecs & 0xffffffff);
#endif
}

void Timer::MSecSleep(const uint32_t mSecs)
{
#ifdef _WIN32
	Sleep(mSecs);
#else
	usleep(mSecs * 1000);
#endif
}

void Timer::USecSleep(const uint32_t uSecs)
{
#ifdef _WIN32
	Sleep(uSecs / 1000);
#else
	usleep(uSecs);
#endif
}

Timer::Timer() {}

Timer::~Timer()
{
	// Notify close and wait for thread
#ifdef USE_STDIO
	printf_s("Timer Closing.\n");
#else
	AddEntry(LOG_INFO, mUser, "Closing.");
#endif // USE_STDIO

	mInitialzied = false;
	mClosing = true;
	mThread->join();
	delete mThread;
}

void Timer::Initialize()
{
	this->mUser = "Timer";

	if (mInitialzied)
	{
		return;
	}

#ifdef _WIN32
	LARGE_INTEGER hrInfo{};
	LARGE_INTEGER curCount{};

	// Set up high-res pollable timer.
	if (!QueryPerformanceFrequency(&hrInfo)
		|| !QueryPerformanceCounter(&curCount))
	{
		Fatal("high resolution timer not available");
	}

	mTimerFactor = 1000000.0 / hrInfo.QuadPart;
	mUSecStartTime = (uint64_t)curCount.QuadPart;

#else

#endif

	mThread = new std::thread(&Timer::HandleTrueMSec, this);

	// Wait for timer thread to become ready
	while (!mTimerThreadReady)
	{
		MSecSleep(1);
	}

	mInitialzied = true;

#ifdef USE_STDIO
	printf("Timer Initialization complete\n");
#else
	Log* log = Log::GetInstance();
	log->AddEntry(LOG_INFO, mUser, "Initialization complete.");
#endif // USE_STDIO
}

int Timer::HandleTrueMSec()
{
#ifdef _WIN32
	HANDLE eventHandle;
	MMRESULT timer;

	// Set up the event which the timer will use to signal us.
	eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (eventHandle == NULL)
	{
		return 0;
	}

	// Request high-resolution timing.
	timeBeginPeriod(1);

	// Set up multimedia timer.
	timer = timeSetEvent(1, 0, (LPTIMECALLBACK)eventHandle, 0, TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
	if (timer == (MMRESULT)0)
	{
		timeEndPeriod(1);
		CloseHandle(eventHandle);
		return 0;
	}

	// Let parent know we're running.
	mTimerThreadReady = TRUE;

	// Enter main timer processing loop.
	while (!mClosing)
	{
		// Wait for callback to signal us.
		DWORD result;
		result = WaitForSingleObject(eventHandle, 10);
		if (result == WAIT_OBJECT_0)
		{
			// Increment tick count.
			mTickCount++;
		}
	}
#else
#endif

	return 0;
}

void Timer::Fatal(std::string msg)
{
#ifdef USE_STDIO
	fprintf_s(stderr, "%s\n", msg.c_str());
#else
	Log* log = Log::GetInstance();
	log->AddEntry(LOG_ERROR, mUser, "Fatal Error: %s", msg.c_str());
#endif
	exit(1);
}