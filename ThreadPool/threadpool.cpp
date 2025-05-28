#include "threadpool.h"




////////////////////////////////////////////////////////////////
const int TASK_MAX_THRESHOLD = 1024;
const int THREAD_MAX_THRESHOLD = 1024;
const int THREAD_IDLE_TIME = 60; //��λ��

ThreadPool::ThreadPool()
	:initThreadSize_(0)
	, taskSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
	, maxThreadSize_(THREAD_MAX_THRESHOLD)
	, isPoolRunning_(false)
	, idelThreadSize_(0)
	, curThreadSize_(0)
{ 
	if (poolMode_ == PoolMode::MODE_FIXED) {
		
	}
}

ThreadPool::~ThreadPool() {
	isPoolRunning_ = false;
	//�ȴ��̳߳������е��̷߳��أ�����״̬:1.����,2.ִ��
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	notEmpty_.notify_all();
	exitCond_.wait(lock, [&]()->bool { return threads_.empty(); });

}

bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}


void ThreadPool::setThreadPoolThreshHold(size_t maxThreadSize) {
	if (checkRunningState()) {
		std::cerr << "thread pool is running, can not set thread pool thresh hold" << std::endl;
		return;
	}
	if (poolMode_ == PoolMode::MODE_FIXED) {
		std::cerr << "thread pool mode is fixed, can not set thread pool thresh hold" << std::endl;
		return;
	}
	maxThreadSize_ = maxThreadSize;
}

void ThreadPool::setMode(PoolMode mode) {
	if (checkRunningState()) {
		std::cerr << "thread pool is running, can not set mode" << std::endl;
		return;
	}
		poolMode_ = mode;	
}

void ThreadPool::setTaskQueMaxThreshHold(size_t maxThreshHold) {
	if (checkRunningState()) {
		std::cerr << "thread pool is running, can not set task que max thresh hold" << std::endl;
		return;
	}
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

	//���ģʽΪMODE_CACHED�Ļ�����������̵߳Ļ���Ҫ��̬�������̳߳�(С�ҿ��Ҷ������)
	if (poolMode_ == PoolMode::MODE_CACHED 
		&& taskSize_ > idelThreadSize_ 
		&& curThreadSize_ < maxThreadSize_)
	{
		std::cout << "create new thread" << std::endl;
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this, std::placeholders::_1));
		size_t threadId = ptr->getId();
		threads_.emplace(threadId,std::move(ptr));
		//�����߳�
		threads_[threadId]->start();
		//��¼�߳����������ֵ�޸�
		curThreadSize_++;
		idelThreadSize_++;
	}	
	//��������Result����
	//return sp->getResult();�ǲ��еģ�
	// ��Ϊ�̳߳������������󣬲��ܷ��ؽ�����������������Task�Ѿ������ˣ�������ʹ���ˣ�
	// ������Task���������ڵ�Result����Ҳ��û��
	return Result(sp);
}

void ThreadPool::start(size_t initThreadSize) {
	//�����̳߳ص�����״̬
	isPoolRunning_ = true;
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	for (size_t i = 0; i < initThreadSize_; ++i) {
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this,std::placeholders::_1));
		//threads_.emplace_back(std::move(ptr));
		size_t threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}
	for (auto& pair : threads_) {
		pair.second->start();  //�����̺߳���
		idelThreadSize_++;//��¼��ʼ��������
	}
}
//�̺߳������̳߳������̴߳������������������
void ThreadPool::ThreadFunc(size_t threadId) 
{

	auto lastTime = std::chrono::high_resolution_clock().now();

	for(;;)
	{
		std::shared_ptr<Task> task;
		{
			//��ȡ������
			std::unique_lock<std::mutex>lock(taskQueMtx_);
			std::cout << std::this_thread::get_id() << " ���Ի�ȡ����... " << threadId << std::endl;
			//�߳�ͨ�ţ�����������Ϊ��,�����ȴ���ֱ��������������
			//cached ģʽ��,
			// ����̳߳��Ѿ������������̣߳����ǿ���ʱ�䳬����Ԥ���������ʱ��60s����Ҫ�������̻߳���
			//��ǰʱ�� - �ϴ��߳�ִ�е�ʱ�� > 60s => �����߳�
				//ÿһ�뷵��һ�Σ�����Ƿ���Ҫ�����߳�
				//��ô���ֳ�ʱ���أ����������ִ�з��أ�
			while (taskQue_.empty()) 
			{
				if (!isPoolRunning_) 
				{
					{
						std::lock_guard<std::mutex> lock(threadMtx_);
						threads_.erase(threadId);//�̳߳ؽ����������߳���Դ
						std::cout << std::this_thread::get_id() << " exit! " << threadId << std::endl;
						exitCond_.notify_all();
						return;
					}
				}
				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						//����������ʱ����
						auto nowTime = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(nowTime - lastTime).count();
						if (dur >= THREAD_IDLE_TIME
							&& curThreadSize_ > initThreadSize_)
						{
							//����������ʱ�䣬�����߳�
							//��¼�߳����������ֵ�޸�
							//�̶߳�����߳��б��������Ƴ�
							//threadId => thread���� =>ɾ��
							{
								std::lock_guard<std::mutex> lock(threadMtx_);
								threads_.erase(threadId);
							}
							curThreadSize_--;
							idelThreadSize_--;

							std::cout << std::this_thread::get_id() << " exit! " << threadId << " idle for " << dur << " seconds, recycle it" << std::endl;
							return;
						}
					}
				}
				else
				{
					notEmpty_.wait(lock);
				}
				//�̳߳�Ҫ�����������߳���Դ
			/*	if (!isPoolRunning_)
				{
					{
						std::lock_guard<std::mutex> lock(threadMtx_);
						threads_.erase(threadId);
						std::cout << std::this_thread::get_id() << " exit! " << threadId << std::endl;
						exitCond_.notify_all();
						return;
					}
				}*/
			}
			idelThreadSize_--;//����������1
			std::cout << std::this_thread::get_id() << " ��ȡ����ɹ�... " << threadId << std::endl;
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			//�����Ȼ������֪ͨ�̳߳������߳̿��Լ�����������
			if (!taskQue_.empty()) 
			{
				notEmpty_.notify_all();
			}
			//ȡ��һ������֪ͨ���Լ����ύ����
			notFull_.notify_all();
		}//�ͷŻ�����

		if (task)
		{
			task->exec();//ִ������
		}
		idelThreadSize_++;//����������1
		lastTime = std::chrono::high_resolution_clock().now();//�����ϴ�ִ��ʱ��
	}
}






////////////////////////////////////////////////////////////////
// 
size_t Thread::generateId_ = 0;
//����
Thread::Thread(ThreadFunc func)
	:func_(func)
	, threadId_(generateId_++)
{

}
//����
Thread::~Thread() {

}
size_t Thread::getId() const {
	return threadId_;
}

void Thread::start() {
	//�����߳�ִ���̺߳���
	std::thread t(func_, threadId_);
	//���÷����߳�
	t.detach();
}


////////////////////////////////////////////////////////////////
//Result ����ʵ��
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
//Task ����ʵ��
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