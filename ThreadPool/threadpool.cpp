#include "threadpool.h"




////////////////////////////////////////////////////////////////
const int TASK_MAX_THRESHOLD = 1024;
const int THREAD_MAX_THRESHOLD = 1024;
const int THREAD_IDLE_TIME = 60; //单位秒

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
	//等待线程池中所有的线程返回，俩种状态:1.阻塞,2.执行
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
	//获取互斥锁
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	//线程通信，如果任务队列满,等待1秒，如果超时
	//wait 和时间没有关系，传入一个参数mtx或者mtx和lambda表达式，lambda表达式返回false则继续等待，返回true则跳出等待
	//wait_for 等待指定时间，超时则返回false，否则返回true
	//wait_unitl 等待指定条件成立，超时则返回false，否则返回true
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool { return taskSize_ < taskQueMaxThreshHold_; })) 
	{
		std::cerr << "task queue is full, wait for 1 second,submit task failed" << std::endl;
		return Result(sp,false);
	}
	else {
		//将任务放入任务队列
		taskQue_.emplace(sp);
		taskSize_++;
	}
	//通知线程池有任务需要处理，因为放了新任务，队列肯定不为空，所以在notEmpty条件变量上通知
	notEmpty_.notify_all();

	//如果模式为MODE_CACHED的话，任务多于线程的话，要动态的扩容线程池(小且快且多的任务)
	if (poolMode_ == PoolMode::MODE_CACHED 
		&& taskSize_ > idelThreadSize_ 
		&& curThreadSize_ < maxThreadSize_)
	{
		std::cout << "create new thread" << std::endl;
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this, std::placeholders::_1));
		size_t threadId = ptr->getId();
		threads_.emplace(threadId,std::move(ptr));
		//启动线程
		threads_[threadId]->start();
		//记录线程数量的相关值修改
		curThreadSize_++;
		idelThreadSize_++;
	}	
	//返回任务Result对象
	//return sp->getResult();是不行的，
	// 因为线程池里的任务结束后，才能返回结果，但是任务结束后，Task已经析构了，不能再使用了！
	// 依赖于Task的生命周期的Result对象也就没了
	return Result(sp);
}

void ThreadPool::start(size_t initThreadSize) {
	//设置线程池的启动状态
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
		pair.second->start();  //启动线程函数
		idelThreadSize_++;//记录初始空闲数量
	}
}
//线程函数，线程池所有线程从任务队列里消费任务
void ThreadPool::ThreadFunc(size_t threadId) 
{

	auto lastTime = std::chrono::high_resolution_clock().now();

	for(;;)
	{
		std::shared_ptr<Task> task;
		{
			//获取互斥锁
			std::unique_lock<std::mutex>lock(taskQueMtx_);
			std::cout << std::this_thread::get_id() << " 尝试获取任务... " << threadId << std::endl;
			//线程通信，如果任务队列为空,持续等待，直到有任务放入队列
			//cached 模式下,
			// 如果线程池已经创建了许多的线程，但是空闲时间超过了预设的最大空闲时间60s，则要将多余线程回收
			//当前时间 - 上次线程执行的时间 > 60s => 回收线程
				//每一秒返回一次，检查是否需要回收线程
				//怎么区分超时返回，和有任务待执行返回？
			while (taskQue_.empty()) 
			{
				if (!isPoolRunning_) 
				{
					{
						std::lock_guard<std::mutex> lock(threadMtx_);
						threads_.erase(threadId);//线程池结束，回收线程资源
						std::cout << std::this_thread::get_id() << " exit! " << threadId << std::endl;
						exitCond_.notify_all();
						return;
					}
				}
				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						//条件变量超时返回
						auto nowTime = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(nowTime - lastTime).count();
						if (dur >= THREAD_IDLE_TIME
							&& curThreadSize_ > initThreadSize_)
						{
							//超过最大空闲时间，回收线程
							//记录线程数量的相关值修改
							//线程对象从线程列表容器中移除
							//threadId => thread对象 =>删除
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
				//线程池要结束，回收线程资源
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
			idelThreadSize_--;//空闲数量减1
			std::cout << std::this_thread::get_id() << " 获取任务成功... " << threadId << std::endl;
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			//如果依然有任务，通知线程池其他线程可以继续消费任务
			if (!taskQue_.empty()) 
			{
				notEmpty_.notify_all();
			}
			//取出一个任务，通知可以继续提交任务
			notFull_.notify_all();
		}//释放互斥锁

		if (task)
		{
			task->exec();//执行任务
		}
		idelThreadSize_++;//空闲数量加1
		lastTime = std::chrono::high_resolution_clock().now();//更新上次执行时间
	}
}






////////////////////////////////////////////////////////////////
// 
size_t Thread::generateId_ = 0;
//构造
Thread::Thread(ThreadFunc func)
	:func_(func)
	, threadId_(generateId_++)
{

}
//析构
Thread::~Thread() {

}
size_t Thread::getId() const {
	return threadId_;
}

void Thread::start() {
	//创建线程执行线程函数
	std::thread t(func_, threadId_);
	//设置分离线程
	t.detach();
}


////////////////////////////////////////////////////////////////
//Result 方法实现
Result::Result(std::shared_ptr<Task> task, bool isValid) 
	:isValid_(isValid)
	, task_(task)
{
	task_->setResult(this);
}

Any Result::get() {//用户调用
	if (!isValid_) {
		std::cerr << "task is invalid, get result failed" << std::endl;
		return "";
	}
	else {
		sem_.wait();//等待任务执行完成
		return std::move(any_);
	}
}

void Result::setVal(Any any) {//线程函数调用
	//存储task的返回值
	any_ = std::move(any);
	sem_.post();//已经获取任务返回值，增加信号量资源
}



////////////////////////////////////////////////////////////////
//Task 方法实现
Task::Task() 
	:result_(nullptr)
{ }
void Task::exec() {
	//发生多态调用
	if (result_ != nullptr) {
		result_->setVal(run());
	}
}

void Task::setResult(Result* res) {
	result_ = res;
}