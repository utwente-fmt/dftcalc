/*
 * System.cpp
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#include <unistd.h>
#include <stdint.h>
#include "System.h"

System::Timer::Timer() {
	reset();
}

System::Timer* System::Timer::create() {
	return new Timer();
}

void System::Timer::reset() {
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_WINDOWS
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);
#endif
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_MONOTONIC_RAW
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif
}

double System::Timer::getElapsedSeconds() {
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_WINDOWS
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (double)(now.QuadPart-start.QuadPart) / (double)(freq.QuadPart) ;
#endif
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_MONOTONIC_RAW
	timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	return (double)(now.tv_sec -start.tv_sec )
		 + (double)(now.tv_nsec-start.tv_nsec)*0.000000001;
#endif
}

void System::sleep(uint64_t ms) {
#ifdef WIN32
	Sleep(ms);
#else
	usleep(ms*1000);
#endif
}