#pragma once

struct TimerEvent
{
	uint32_t intervalMs;
	bool repeat;
	std::function<void()> callback;
	std::chrono::system_clock::time_point wakeUpTime;

	TimerEvent(std::function<void()> cb, uint32_t interval, bool repeat_)
		: intervalMs(interval), repeat(repeat_), callback(cb)
	{
		wakeUpTime = std::chrono::system_clock::now() + std::chrono::milliseconds(interval);
	}

	bool operator<(const TimerEvent& other) const
	{
		return wakeUpTime > other.wakeUpTime;
	}
};

class TimerQueue
{
public:
	static void TimerWorkerLoop();
	static void AddTimer(std::function<void()> callback, uint32_t intervalMs, bool repeat = false);
	
	static std::priority_queue<TimerEvent> _timerQueue;
	static std::mutex _mutex;
	static inline std::condition_variable _cv;
};

