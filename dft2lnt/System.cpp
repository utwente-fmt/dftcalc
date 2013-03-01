/*
 * System.cpp
 * 
 * Part of a general library.
 * 
 * Adapted by Gerjan Stokkink to support Mac OS X.
 *
 * @author Freark van der Berg
 */

#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
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
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_APPLE
	gettimeofday(&start, (void *)NULL);
#endif
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_MONOTONIC_RAW
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_MONOTONIC
	clock_gettime(CLOCK_MONOTONIC, &start);
#endif
}

double System::Timer::getElapsedSeconds() {
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_WINDOWS
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (double)(now.QuadPart-start.QuadPart) / (double)(freq.QuadPart) ;
#endif
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_APPLE
	timeval now;
	gettimeofday(&now, (void *)NULL);
	return (double)(now.tv_sec -start.tv_sec )
		 + (double)(now.tv_usec-start.tv_usec)*0.000001;
#endif
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_MONOTONIC_RAW
	timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	return (double)(now.tv_sec -start.tv_sec )
		 + (double)(now.tv_nsec-start.tv_nsec)*0.000000001;
#endif
#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_MONOTONIC
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
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

void System::generateUUID(size_t bytes,std::string& uuid) {
	static uint64_t lastTime;
	static uint64_t currentTime;
	static std::uniform_int_distribution<int> hex {0,15};  // distribution that maps to the ints 1..6
	static std::default_random_engine re {};                     // the default engine	uuid.empty;
	do {
		currentTime = getCurrentTimeMicros();
	} while(currentTime==lastTime);
	lastTime = currentTime;
	re.seed(currentTime);
	uuid.clear();
	uuid.reserve(bytes*2);
	for(unsigned int i=0; i<bytes*2; ++i) {
		int x = hex(re);
		if(x<10) uuid += ('0'+x);
		else     uuid += ('A'+x-10);
	}
}

uint64_t System::getCurrentTimeMillis() {
	timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec*1000 + now.tv_usec/1000;
}

uint64_t System::getCurrentTimeMicros() {
	timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec*1000000 + now.tv_usec;
}
