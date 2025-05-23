#ifndef  THREADPOOL_H
#define  THREADPOOL_H
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

//线程池模式
enum class PoolMode {
	FIXED,
	CACHED,
};
//线程类
class Thread {
public:
private:
};

class Task {
public:
	virtual void run() = 0;
};
//线程池类
class ThreadPool {
public:

private:
	std::vector<Thread*> threads_;//线程池
	size_t initThreadPoolSize_;//初始线程池大小
	
	std::queue<std::shared_ptr<Task>> taskQueue_;//任务队列
	size_t TaskQueMaxThreshHold_;//任务队列最大阈值
	size_t TaskQueInitThreshHold_;//任务队列初始阈值

};
#endif
