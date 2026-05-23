#include "pch.h"
#include "DBWorker.h"

void DBWorker::Start(int32 threadCount)
{
    _running = true;
    for (int32 i = 0; i < threadCount; ++i)
        _threads.emplace_back(&DBWorker::WorkerLoop, this);
    LOG_I("DBWorker started ({} threads)", threadCount);
}

void DBWorker::Shutdown()
{
    {
        std::lock_guard lock(_mutex);
        _running = false;
    }
    _cv.notify_all();
    for (auto& t : _threads)
        if (t.joinable()) t.join();
    _threads.clear();
}

void DBWorker::Push(std::function<void()> job)
{
    {
        std::lock_guard lock(_mutex);
        _jobs.push(std::move(job));
    }
    _cv.notify_one();
}

void DBWorker::WorkerLoop()
{
    while (true)
    {
        std::function<void()> job;
        {
            std::unique_lock lock(_mutex);
            _cv.wait(lock, [this] { return !_jobs.empty() || !_running; });
            if (!_running && _jobs.empty())
                break;
            job = std::move(_jobs.front());
            _jobs.pop();
        }
        job();
    }
}
