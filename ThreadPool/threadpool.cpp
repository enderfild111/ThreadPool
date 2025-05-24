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
	//��ȡ������
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	//�߳�ͨ�ţ�������������,�ȴ�1�룬�����ʱ
	//wait ��ʱ��û�й�ϵ������һ������mtx����mtx��lambda���ʽ��lambda���ʽ����false������ȴ�������true�������ȴ�
	//wait_for �ȴ�ָ��ʱ�䣬��ʱ�򷵻�false�����򷵻�true
	//wait_unitl �ȴ�ָ��������������ʱ�򷵻�false�����򷵻�true
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool { return taskSize_ < taskQueMaxThreshHold_; })) 
	{
		throw std::runtime_error("task queue is full");
	}
	else {
		//����������������
		taskQue_.emplace(sp);
		taskSize_++;
	}
	//֪ͨ�̳߳���������Ҫ������Ϊ���������񣬶��п϶���Ϊ�գ�������notEmpty����������֪ͨ
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
//�̺߳������̳߳������̴߳������������������
void ThreadPool::ThreadFunc() {
	std::shared_ptr<Task> task;
	for (;;) {
		{
			//��ȡ������
			std::unique_lock<std::mutex>lock(taskQueMtx_);
			//�߳�ͨ�ţ�����������Ϊ��,�����ȴ���ֱ��������������
			//std::cout << "tid:" <<std::this_thread::get_id() << "wait for task..." << std::endl;
			notEmpty_.wait(lock, [&]()->bool { return !taskQue_.empty(); });
			//std::cout << "tid:" << std::this_thread::get_id() << "get task successfully" << std::endl;
			//���������ȡ������
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			//�����Ȼ������֪ͨ�̳߳������߳̿��Լ�����������
			if (!taskQue_.empty()) {
				notEmpty_.notify_all();
			}
			//ȡ��һ������֪ͨ���Լ����ύ����
			notFull_.notify_all();
		}
		//���������ͷ���
		if (task) {
			//ִ������
			task->run();
		}
	}
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