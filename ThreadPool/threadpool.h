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



//Any类型：可以接受任意类型的数据类型
class Any {
public:
	Any() = default;
	~Any() = default;
	//禁止拷贝构造和赋值运算
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	//移动构造函数
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	//构造函数可以让Any接受任意其他类型的数据
	template<typename T>
	Any(T data) 
		:base_(std::make_unique<Derive<T>>(data))
	{ }

	//把Any对象存储的数据转化为指定类型
	/*
	T => int
	Derived<int>
	*/
	template<typename T>
	T cast_(){
		//怎么从Base_找到所指向的Derive<T>对象，取出data_成员变量
		//dynamic_cast把基类指针转换为派生类指针（具有RTTI识别）//RTTI识别：运行时类型识别
		Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd) {
			return pd->data_;
		}
		else {
			throw "type is incompatiable!";//抛出异常
		}
	}
private:
	class Base {
	public:
		//基类虚析构
		virtual ~Base() = default;
	};
	//派生类类型
	template<typename T>
	class Derive :public Base {
	public:
		//构造函数
		Derive(T data) :data_(data) {}
		//析构函数
		~Derive() override {}
		//获取数据
		T data_;
	};
private:
	std::unique_ptr<Base> base_;
};

//实现一个信号量类
class Semaphore {
public:
	Semaphore(int limit = 0)
		:resLimit_(limit)
	{ }
	~Semaphore() = default;
	//获取一个信号量资源
	void wait() {
		std::unique_lock<std::mutex> lock(mtx_);
		cond_.wait(lock, [&]() {return resLimit_ > 0; });
		resLimit_--;
	}
	//增加一个信号量资源
	void post() {
		std::unique_lock<std::mutex> lock(mtx_);
		resLimit_++;
		cond_.notify_all ();
	}
private:
	std::size_t resLimit_;//资源计数
	std::mutex mtx_;
	std::condition_variable cond_;
};

class Task;//前置声明
//Result类，接受提交到线程池的task结束后的返回值
class Result {
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;
	//setVal方法,如何实现? 获取返回值
	void setVal(Any any);
	//get方法，用户调用该函数获取返回值
	Any get();
private:
	Any any_;//存储返回值
	Semaphore sem_;//线程通信信号量
	std::shared_ptr<Task> task_;//指向对应的获取返回值的任务对象
	std::atomic_bool isValid_;//任务是否有效
};
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
	Task();
	~Task() = default;
	virtual Any run() = 0;
	void exec();
	void setResult(Result* res);
private:
	//如果使用强智能指针，会导致强智能指针的交叉引用
	Result* result_;//指向对应的结果对象
};


/*
example:
ThreadPool pool;
pool.start();
pool.submitTask(std::make_shared<MyTask>());
//继承Task类实现自己的任务
class MyTask : public Task {
public:
	void run() override {
		std::cout << "hello world" << std::endl;
	}
};
*/
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
	Result submitTask(std::shared_ptr<Task>sp);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	void ThreadFunc();
private:
	std::vector<std::unique_ptr<Thread>> threads_;//线程池
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
