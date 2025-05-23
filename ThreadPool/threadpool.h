#ifndef  THREADPOOL_H
#define  THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <memory>

//线程池模式
enum class PoolMode {
	MODE_FIXED,
	MODE_CACHED,
};
//线程类
class Thread {
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void()>;
	//构造
	Thread(ThreadFunc func);
	//析构
	~Thread();
	void start();
private:
	ThreadFunc func_;
};
//用户可以继承Task类实现自己的任务
//重写run方法
class Task {
public:
	virtual void run() = 0;
};
//线程池类
class ThreadPool {
public:
	//构造函数
	ThreadPool();
	//析构函数
	~ThreadPool();
	//启动线程池
	void start(size_t initThreadPoolSize = 4);
	//设置线程池模式
	void setMode(PoolMode mode);
	//设置线程池任务队列最大阈值
	void setTaskQueMaxThreshHold(size_t maxThreshHold);
	//向线程池提交任务
	void submitTask(std::shared_ptr<Task>sp);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	void ThreadFunc();
private:
	std::vector<Thread*> threads_;//线程池
	size_t initThreadSize_;//初始线程池大小
	//防止用户传入临时对象,对象生命周期太短，导致线程池无法正常工作所以使用shared_ptr拉长生命周期
	std::queue<std::shared_ptr<Task>> taskQue_;//任务队列
	std::atomic_uint taskSize_;//任务队列数量
	size_t taskQueMaxThreshHold_;//任务队列最大阈值


	std::mutex taskQueMtx_;//任务队列互斥锁
	std::condition_variable notFull_; //未满条件变量
	std::condition_variable notEmpty_;//未空条件变量

	PoolMode poolMode_;//线程池模式
};




#endif
