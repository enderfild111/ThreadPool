#include "threadpool.h"

const int TASK_MAX_THRESHOLD = 1024;

ThreadPool::ThreadPool()
	:initThreadSize_(0)
	, taskSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
{ }

ThreadPool::~ThreadPool() {

}

void ThreadPool::setMode(PoolMode mode) {

}

void ThreadPool::setTaskQueMaxThreshHold(size_t maxThreshHold) {

}

void ThreadPool::submitTask(std::shared_ptr<Task>sp) {
	//获取互斥锁
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	//线程通信，如果任务队列满,等待1秒，如果超时
	//wait 和时间没有关系，传入一个参数mtx或者mtx和lambda表达式，lambda表达式返回false则继续等待，返回true则跳出等待
	//wait_for 等待指定时间，超时则返回false，否则返回true
	//wait_unitl 等待指定条件成立，超时则返回false，否则返回true
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool { return taskSize_ < taskQueMaxThreshHold_; })) 
	{
		throw std::runtime_error("task queue is full");
	}
	else {
		//将任务放入任务队列
		taskQue_.emplace(sp);
		taskSize_++;
	}
	//通知线程池有任务需要处理，因为放了新任务，队列肯定不为空，所以在notEmpty条件变量上通知
	notEmpty_.notify_all();
}

void ThreadPool::start(size_t initThreadSize) {
	initThreadSize_ = initThreadSize;
	for (size_t i = 0; i < initThreadSize_; ++i) {
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this));
		threads_.emplace_back(std::move(ptr));
	}
	for (auto& thread : threads_) {
		thread->start();
	}
}
//线程函数，线程池所有线程从任务队列里消费任务
void ThreadPool::ThreadFunc() {
	std::shared_ptr<Task> task;
	for (;;) {
		{
			//获取互斥锁
			std::unique_lock<std::mutex>lock(taskQueMtx_);
			//线程通信，如果任务队列为空,持续等待，直到有任务放入队列
			//std::cout << "tid:" <<std::this_thread::get_id() << "wait for task..." << std::endl;
			notEmpty_.wait(lock, [&]()->bool { return !taskQue_.empty(); });
			//std::cout << "tid:" << std::this_thread::get_id() << "get task successfully" << std::endl;
			//从任务队列取出任务
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			//如果依然有任务，通知线程池其他线程可以继续消费任务
			if (!taskQue_.empty()) {
				notEmpty_.notify_all();
			}
			//取出一个任务，通知可以继续提交任务
			notFull_.notify_all();
		}
		//出作用域，释放锁
		if (task) {
			//执行任务
			task->run();
		}
	}
}





//构造
Thread::Thread(ThreadFunc func)
	:func_(func)
{

}
//析构
Thread::~Thread() {

}


void Thread::start() {
	//创建线程执行线程函数
	std::thread t(func_);
	//设置分离线程
	t.detach();
}