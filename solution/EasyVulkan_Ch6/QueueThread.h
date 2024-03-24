#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class queueThread {
	using work_t = std::function<void()>;
	std::thread thread;
	bool joinable = true;
	std::queue<work_t> works;
	std::mutex mutex;
	std::condition_variable condition;
	//--------------------
	void DoWorks() {
		while (true) {
			std::unique_lock lock(mutex);
			condition.wait(lock, [this] { return !(works.empty() && joinable); });
			if (!joinable)
				return;
			works.front()();
			works.pop();
			condition.notify_one();
		}
	}
public:
	queueThread() {
		thread = std::thread(&queueThread::DoWorks, this);
	}
	queueThread(queueThread&&) = delete;
	~queueThread() {
		std::unique_lock lock(mutex);
		condition.wait(lock, [this] { return works.empty(); });
		joinable = false;
		lock.unlock();
		condition.notify_one();
		thread.join();
	}
	//Non-const Function
	void PushWork(work_t work) {
		mutex.lock();
		works.push(work);
		mutex.unlock();
		condition.notify_one();
	}
	void Wait() {
		std::unique_lock lock(mutex);
		condition.wait(lock, [this] { return works.empty(); });
	}
};