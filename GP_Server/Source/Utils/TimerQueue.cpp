#include "pch.h"
#include "TimerQueue.h"

std::priority_queue<TimerEvent> TimerQueue::_timerQueue;
std::mutex TimerQueue::_mutex;

void TimerQueue::TimerWorkerLoop()
{
	std::unique_lock<std::mutex> lock(_mutex);
	while (true)
	{
		if (_timerQueue.empty())
		{
			_cv.wait(lock);
			continue;
		}

		auto now = std::chrono::system_clock::now();
		auto& top = _timerQueue.top();

		if (now >= _timerQueue.top().wakeUpTime)
		{
			auto event = _timerQueue.top();
			_timerQueue.pop();
			lock.unlock();

			if (event.callback)
				event.callback();

			if (event.repeat)
				AddTimer(event.callback, event.intervalMs, true);

			lock.lock();
		}
		else
		{
			auto wakeTime = _timerQueue.top().wakeUpTime;
			_cv.wait_until(lock, wakeTime);
		}

	}
}

void TimerQueue::AddTimer(std::function<void()> callback, uint32_t intervalMs, bool repeat)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_timerQueue.push(TimerEvent(callback, intervalMs, repeat));
	_cv.notify_one();
}