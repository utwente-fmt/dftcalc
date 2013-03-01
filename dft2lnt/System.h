/*
 * System.h
 * 
 * Part of a general library.
 * 
 * Adapted by Gerjan Stokkink to support Mac OS X.
 *
 * @author Freark van der Berg
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#define SYSTEM_TIMER_BACKEND_MONOTONIC     1
#define SYSTEM_TIMER_BACKEND_MONOTONIC_RAW 2
#define SYSTEM_TIMER_BACKEND_WINDOWS       3
#define SYSTEM_TIMER_BACKEND_APPLE         4

#include <time.h>
#include <string>
#include <random>
#ifdef WIN32
#	include <windows.h>
#	define SYSTEM_TIMER_BACKEND SYSTEM_TIMER_BACKEND_WINDOWS
#elif defined __APPLE__
#	define SYSTEM_TIMER_BACKEND SYSTEM_TIMER_BACKEND_APPLE
#elif defined CLOCK_MONOTONIC_RAW && defined CLOCKS_MONO && CLOCKS_MONO == CLOCK_MONOTONIC_RAW
#	define SYSTEM_TIMER_BACKEND SYSTEM_TIMER_BACKEND_MONOTONIC_RAW
#else
#	define SYSTEM_TIMER_BACKEND SYSTEM_TIMER_BACKEND_MONOTONIC
#endif

class System {
public:
	class Timer {
	private:
	#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_WINDOWS
		LARGE_INTEGER freq;
		LARGE_INTEGER start;
	#endif
	#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_APPLE
		timeval start;
	#endif
	#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_MONOTONIC_RAW
		timespec start;
	#endif
	#if SYSTEM_TIMER_BACKEND == SYSTEM_TIMER_BACKEND_MONOTONIC
		timespec start;
	#endif
	public:
		Timer();
		Timer* create();
		void reset();
		double getElapsedSeconds();
	};

	static void sleep(uint64_t ms);
	
	static void generateUUID(size_t bytes,std::string& uuid);
	
	static uint64_t getCurrentTimeMillis();
	static uint64_t getCurrentTimeMicros();
};

#endif
