/*
This file is part of Stopwatch, a project by Tommaso Urli.

Stopwatch is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Stopwatch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stopwatch.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

#ifndef WIN32
	#include <sys/time.h>
#else
	#include <Windows.h>
	#include <iomanip>
#endif

#include "Stopwatch.h"

using std::map;
using std::string;
using std::ostringstream;

// Initialize singleton pointer
map<string, Stopwatch::PerformanceData >* Stopwatch::records_of = new map<string, Stopwatch::PerformanceData >();

// Activate clock
bool Stopwatch::active = true;

// Set stopwatch mode to none, force user to decide how to take times
StopwatchMode Stopwatch::mode = NONE;

// Initialize clock with a mode, if done between a start and a stop, results will be wrong
void Stopwatch::init(StopwatchMode new_mode) {
	/*
	if (mode != NONE)
		return;
	else
	*/
	mode = new_mode;
}

// Take time, depends on mode
long double Stopwatch::take_time() {

	if ( mode == CPU_TIME ) {
		
		// Use ctime
		return clock();
		
	} else if ( mode == REAL_TIME ) {
		
		// Query operating system
		
#ifdef WIN32
		/*	In case of usage under Windows */
		FILETIME ft;
		LARGE_INTEGER intervals;

		// Get the amount of 100 nanoseconds intervals elapsed since January 1, 1601 (UTC)
		GetSystemTimeAsFileTime(&ft);
		intervals.LowPart = ft.dwLowDateTime;
		intervals.HighPart = ft.dwHighDateTime;

		long double measure = intervals.QuadPart;
		measure -= 116444736000000000.0;			// Convert to UNIX epoch time
		measure /= 10000000.0;						// Convert to seconds

		return measure;
#else
		/* Linux, MacOS, ... */
		struct timeval tv;
		gettimeofday(&tv, NULL);

		long double measure = tv.tv_usec;
		measure /= 1000000.0;						// Convert to seconds
		measure += tv.tv_sec;						// Add seconds part

		return measure;
#endif

	} else {
		// If mode == NONE, clock has not been initialized, then throw exception
		throw StopwatchException("Clock not initialized to a time taking mode!");
	}
}

// Start the counting
void Stopwatch::start(string perf_name)  {

	if (!active) return;

	// Just works if not already present
	records_of->insert(make_pair(perf_name, PerformanceData()));

	PerformanceData& perf_info = records_of->find(perf_name)->second;

	// Take ctime
	perf_info.clock_start = take_time();
}

// Stops the counting
void Stopwatch::stop(string perf_name) {
	
	if (!active) return;
	
	long double clock_end = take_time();
	
	// Try to recover performance data
	if ( records_of->find(perf_name) == records_of->end() )
		throw StopwatchException("Performance not initialized.");
	
	PerformanceData& perf_info = records_of->find(perf_name)->second;
	
	perf_info.stops++;
	long double  lapse = clock_end - perf_info.clock_start;
	
	if ( mode == CPU_TIME )
		lapse /= (double) CLOCKS_PER_SEC;
	
	// Update min/max time
	if ( lapse >= perf_info.max_time )	perf_info.max_time = lapse;
	if ( lapse <= perf_info.min_time || perf_info.min_time == 0 )	perf_info.min_time = lapse;
	
	// Update total time
	perf_info.total_time += lapse;
}

// Stops the counting
void Stopwatch::pause(string perf_name) {
	
	if (!active) return;
	
	long double  clock_end = clock();
	
	// Try to recover performance data
	if ( records_of->find(perf_name) == records_of->end() )
		throw StopwatchException("Performance not initialized.");
	
	PerformanceData& perf_info = records_of->find(perf_name)->second;
	
	long double  lapse = clock_end - perf_info.clock_start;
	
	// Update total time
	perf_info.total_time += lapse;
}

// Reset all performance records
void Stopwatch::reset_all() {
	
	if (!active) return;

	map<string, PerformanceData>::iterator it;
	
	for (it = records_of->begin(); it != records_of->end(); it++) {
		reset(it->first);
	}
}

// Report all data
void Stopwatch::report_all(std::ostream& output) {

	if (!active) return;

	map<string, PerformanceData>::iterator it;
	
	for (it = records_of->begin(); it != records_of->end(); it++) {
		report(it->first, output);
	}
}

// Reset the stopwatch
void Stopwatch::reset(string perf_name) {

	if (!active) return;

	// Try to recover performance data
	if ( records_of->find(perf_name) == records_of->end() )
		throw StopwatchException("Performance not initialized.");
	
	PerformanceData& perf_info = records_of->find(perf_name)->second;
	
	perf_info.clock_start = 0;
	perf_info.total_time = 0;
	perf_info.min_time = 0;
	perf_info.max_time = 0;
	perf_info.stops = 0;
}

void Stopwatch::turn_on() {
	std::cout << "Stopwatch active." << std::endl;
	active = true;
}

void Stopwatch::turn_off() {
	std::cout << "Stopwatch inactive." << std::endl;
	active = false;
}

void Stopwatch::report(string perf_name, std::ostream& output) {

	if (!active) return;
	
	// Try to recover performance data
	if ( records_of->find(perf_name) == records_of->end() )
		throw StopwatchException("Performance not initialized.");
	
	PerformanceData& perf_info = records_of->find(perf_name)->second;

	// To support Windows (otherwise string("=", perf_name.length() + 1) would do the job
	string bar = "";
	for (int i = 0; i < perf_name.length(); i++)
		bar.append("=");

	output << std::endl;
	output << "======================" << bar << std::endl;
	output << "Tracking performance: " << perf_name << std::endl;
	output << "======================" << bar << std::endl;
	output << "  *  Avg. time " << (perf_info.total_time / (double) perf_info.stops) << " sec" << std::endl;
	output << "  *  Min. time " << (perf_info.min_time) << " sec" << std::endl;
	output << "  *  Max. time " << (perf_info.max_time) << " sec" << std::endl;
	output << "  *  Tot. time " << (perf_info.total_time) << " sec" << std::endl;

	ostringstream stops;
	stops << perf_info.stops;

	output << "  *  Stops " << stops.str() << std::endl;
	output << std::endl;
	
}
