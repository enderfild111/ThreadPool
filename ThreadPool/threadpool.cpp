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


}

void ThreadPool::start(size_t initThreadSize) {
	initThreadSize_ = initThreadSize;
	for (size_t i = 0; i < initThreadSize_; ++i) {
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::ThreadFunc, this)));
	}
	for (auto& thread : threads_) {
		thread->start();
	}
}

void ThreadPool::ThreadFunc() {
	std::cout << "ThreadFunc tid::" <<std::this_thread::get_id() << std::endl;


	std:: cout << "end tid::" << std::this_thread::get_id() << std::endl;
}





//����
Thread::Thread(ThreadFunc func)
	:func_(func)
{

}
//����
Thread::~Thread() {

}


void Thread::start() {
	//�����߳�ִ���̺߳���
	std::thread t(func_);
	//���÷����߳�
	t.detach();
}