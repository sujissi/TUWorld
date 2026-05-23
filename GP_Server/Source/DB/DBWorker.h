#pragma once

class DBWorker {
public:
    static DBWorker& GetInst() {
        static DBWorker inst;
        return inst;
    }

    void Start(int32 threadCount = 2);
    void Shutdown();
    void Push(std::function<void()> job);

private:
    void WorkerLoop();

    std::vector<std::thread> _threads;
    std::queue<std::function<void()>> _jobs;
    std::mutex _mutex;
    std::condition_variable _cv;
    bool _running = false;
};
