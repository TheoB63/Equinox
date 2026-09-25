#pragma once

#include "equinox/resources/FileSystem.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <iostream>

// TODO: Integrate Tracy Profiler here
// Empty macros to prepare the codebase

using namespace std::chrono;
using Clock = high_resolution_clock;

struct TraceEvent
{
	std::string name;
	std::string phase;
	int64_t timeStamp;
	std::thread::id threadID;

};

class Profiler
{
private:
	inline static Profiler* instance = nullptr;
	Profiler() {}; // Use Get Only
	~Profiler() { SaveTrace(); }
public:
	static Profiler* get()
	{
		if (!instance)
			instance = new Profiler();

		return instance;
	}

	void AddTrace(TraceEvent event)
	{
		traceEvents.push_back(event);
	}

	void SaveTrace()
	{
		std::string filename = Equinox::FileSystem::AssetsPath().string();
		filename += "ProfilerTrace.json";

		std::ofstream outFile(filename);
		if (!outFile.is_open())
		{
			std::cerr << "Error : Cound not open " << filename << std::endl;
			return;
		}

		outFile << "{\n";
		outFile << "  \"traceEvents\": [\n";

		for (size_t i = 0; i < traceEvents.size(); ++i)
		{
			const TraceEvent& event = traceEvents[i];

			outFile << "    {\n";
			outFile << "      \"name\": \"" << event.name << "\",\n";
			outFile << "      \"ph\": \"" << event.phase << "\",\n";
			outFile << "      \"ts\": " << event.timeStamp << ",\n";
			outFile << "      \"tid\": " << event.threadID << ",\n";
			outFile << "      \"pid\": 0\n";
			outFile << "    }";

			if (i + 1 < traceEvents.size())
			{
				outFile << ",";
			}
			outFile << "\n";
		}

		outFile << "  ]\n";
		outFile << "}\n";

		outFile.flush();
		outFile.close();

		std::cout << "Save Trace to " << outFile.getloc().name() << std::endl;
	}

private:
	std::vector<TraceEvent> traceEvents;

};

inline int64_t GetRelativeTimeMicroseconds()
{
	using namespace std::chrono;
	static const auto start_time = high_resolution_clock::now();

	auto now = high_resolution_clock::now();
	return duration_cast<microseconds>(now - start_time).count();
}

class ProfileTimer
{
public:
	ProfileTimer(const char* _name)
		: m_name(_name),
		m_start(Clock::now())
	{
		TraceEvent traceEvent =
		{
			m_name,
			"B",
			GetRelativeTimeMicroseconds(),
			std::this_thread::get_id()
		};

		Profiler::get()->AddTrace(traceEvent);
	}

	~ProfileTimer()
	{
		const Clock::time_point end = Clock::now();
		const microseconds duration = duration_cast<microseconds>(end - m_start);
		//std::cout << m_name << ": " << duration.count() * 1e-3f << " ms\n" << std::endl;

		TraceEvent traceEvent =
		{
			m_name,
			"E",
			GetRelativeTimeMicroseconds(),
			std::this_thread::get_id()
		};

		Profiler::get()->AddTrace(traceEvent);
	}

	void Start(const char* _name)
	{
		m_name = _name;
		m_start = Clock::now();
	}

private:
	const char* m_name{ nullptr };
	Clock::time_point m_start;
};

//#define PROFILE_TIMER_SCOPE(_name) ProfileTimer profileTimer##__LINE__(_name);
//#define EQN_PROFILE_FUNCTION() PROFILE_TIMER_SCOPE(__func__);


#if 0 // Set to 1 when Tracy is included
#include <tracy/Tracy.hpp>
#define EQN_PROFILE_FRAME(name)      FrameMarkNamed(name)
#define EQN_PROFILE_FUNCTION()       ZoneScoped
#define EQN_PROFILE_SCOPE(name)      ZoneScopedN(name)
#define EQN_PROFILE_TAG(key, val)    ZoneText(val, strlen(val))
#else
#define EQN_PROFILE_FRAME(name)
#define EQN_PROFILE_FUNCTION()
#define EQN_PROFILE_SCOPE(name)
#define EQN_PROFILE_TAG(key, val)
#endif