#include "threadpool.h"



////////////////////////////////////////////////////////////////
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

Result ThreadPool::submitTask(std::shared_ptr<Task>sp) {
	//��ȡ������
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	//�߳�ͨ�ţ�������������,�ȴ�1�룬�����ʱ
	//wait ��ʱ��û�й�ϵ������һ������mtx����mtx��lambda���ʽ��lambda���ʽ����false������ȴ�������true�������ȴ�
	//wait_for �ȴ�ָ��ʱ�䣬��ʱ�򷵻�false�����򷵻�true
	//wait_unitl �ȴ�ָ��������������ʱ�򷵻�false�����򷵻�true
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool { return taskSize_ < taskQueMaxThreshHold_; })) 
	{
		std::cerr << "task queue is full, wait for 1 second,submit task failed" << std::endl;
		return Result(sp,false);
	}
	else {
		//����������������
		taskQue_.emplace(sp);
		taskSize_++;
	}
	//֪ͨ�̳߳���������Ҫ������Ϊ���������񣬶��п϶���Ϊ�գ�������notEmpty����������֪ͨ
	notEmpty_.notify_all();

	//��������Result����
	//return sp->getResult();�ǲ��еģ�
	// ��Ϊ�̳߳������������󣬲��ܷ��ؽ�����������������Task�Ѿ������ˣ�������ʹ���ˣ�
	// ������Task���������ڵ�Result����Ҳ��û��
	return Result(sp);
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
			task->exec();
			//������ķ���ֵͨ��setVal�������õ�Result������
		}
	}
}





////////////////////////////////////////////////////////////////
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


////////////////////////////////////////////////////////////////
//Result����ʵ��
Result::Result(std::shared_ptr<Task> task, bool isValid) 
	:isValid_(isValid)
	, task_(task)
{
	task_->setResult(this);
}

Any Result::get() {//�û�����
	if (!isValid_) {
		std::cerr << "task is invalid, get result failed" << std::endl;
		return "";
	}
	else {
		sem_.wait();//�ȴ�����ִ�����
		return std::move(any_);
	}
}

void Result::setVal(Any any) {//�̺߳�������
	//�洢task�ķ���ֵ
	any_ = std::move(any);
	sem_.post();//�Ѿ���ȡ���񷵻�ֵ�������ź�����Դ
}



////////////////////////////////////////////////////////////////
//Task����ʵ��
Task::Task() 
	:result_(nullptr)
{ }
void Task::exec() {
	//������̬����
	if (result_ != nullptr) {
		result_->setVal(run());
	}
}

void Task::setResult(Result* res) {
	result_ = res;
}