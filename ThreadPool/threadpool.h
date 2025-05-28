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
#include<chrono>
#include<unordered_map>

//Any ���ͣ����Խ����������͵���������
class Any {
public:
	Any() = default;
	~Any() = default;
	//��ֹ��������͸�ֵ����
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	//�ƶ����캯��
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	//���캯��������Any���������������͵�����
	template<typename T>
	Any(T data) 
		:base_(std::make_unique<Derive<T>>(data))
	{ }

	//��Any����洢������ת��Ϊָ������
	/*
	T => int
	Derived<int>
	*/
	template<typename T>
	T cast_(){
		//��ô��Base_�ҵ���ָ���Derive<T>����ȡ��data_��Ա����
		//dynamic_cast�ѻ���ָ��ת��Ϊ������ָ�루����RTTIʶ��//RTTIʶ������ʱ����ʶ��
		Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd) {
			return pd->data_;
		}
		try {
			throw "type is Incompatiable!";//�׳��쳣
		}
		catch (const char* msg) {
			std::cerr << msg << std::endl;
		}
		return T();
	}
private:
	class Base {
	public:
		//����������
		virtual ~Base() = default;
	};
	//����������
	template<typename T>
	class Derive :public Base {
	public:
		//���캯��
		Derive(T data) :data_(data) {}
		//��������
		~Derive() override {}
		//��ȡ����
		T data_;
	};
private:
	std::unique_ptr<Base> base_;
};

//ʵ��һ���ź�����
class Semaphore {
public:
	Semaphore(int limit = 0)
		:resLimit_(limit)
	{ }
	~Semaphore() = default;
	//��ȡһ���ź�����Դ
	void wait() {
		std::unique_lock<std::mutex> lock(mtx_);
		cond_.wait(lock, [&]() {return resLimit_ > 0; });
		resLimit_--;
	}
	//����һ���ź�����Դ
	void post() {
		std::unique_lock<std::mutex> lock(mtx_);
		resLimit_++;
		//linux��condition_varible����������ʲôҲû������������״̬ʧЧ���޹�����
		cond_.notify_all ();
	}
private:
	std::size_t resLimit_;//��Դ����
	std::mutex mtx_;
	std::condition_variable cond_;
};

class Task;//ǰ������
//Result �࣬�����ύ���̳߳ص�task������ķ���ֵ
class Result {
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;
	//setVal����,���ʵ��? ��ȡ����ֵ
	void setVal(Any any);
	//get �������û����øú�����ȡ����ֵ
	Any get();
private:
	Any any_;//�洢����ֵ
	Semaphore sem_;//�߳�ͨ���ź���
	std::shared_ptr<Task> task_;//ָ���Ӧ�Ļ�ȡ����ֵ���������
	std::atomic_bool isValid_;//�����Ƿ���Ч
};
//�̳߳�ģʽ
enum class PoolMode {
	MODE_FIXED,
	MODE_CACHED,
};
//�߳���
class Thread {
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void(size_t)>;
	//����
	Thread(ThreadFunc func);
	//����
	~Thread();
	void start();

	size_t getId() const;
private:
	ThreadFunc func_;
	static size_t generateId_;
	size_t threadId_; //�����߳�id
};
//�û����Լ̳�Task��ʵ���Լ�������
//��дrun����
class Task {
public:
	Task();
	~Task() = default;
	virtual Any run() = 0;
	void exec();
	void setResult(Result* res);
private:
	//���ʹ��ǿ����ָ�룬�ᵼ��ǿ����ָ��Ľ�������
	Result* result_;//ָ���Ӧ�Ľ������
};


/*
example:
ThreadPool pool;
pool.start();
pool.submitTask(std::make_shared<MyTask>());
//�̳�Task��ʵ���Լ�������
class MyTask : public Task {
public:
	void run() override {
		std::cout << "hello world" << std::endl;
	}
};
*/
//�̳߳���
class ThreadPool {
public:
	//���캯��
	ThreadPool();
	//��������
	~ThreadPool();
	//�����̳߳�
	void start(size_t initThreadPoolSize = std::thread::hardware_concurrency());//hardware_concurrency()��ȡ��ǰϵͳ��CPU������
	//�����̳߳�ģʽ
	void setMode(PoolMode mode);
	//�����̳߳�������������ֵ
	void setTaskQueMaxThreshHold(size_t maxThreshHold);

	void setThreadPoolThreshHold(size_t maxThreadSize = 10);
	//���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task>sp);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	void ThreadFunc(size_t threadId);

	bool checkRunningState()const;
private:
	//std::vector<std::unique_ptr<Thread>> threads_;//�̳߳�
	std::unordered_map<size_t,std::unique_ptr<Thread>> threads_;//�̳߳�
	size_t initThreadSize_;//��ʼ�̳߳ش�С
	size_t maxThreadSize_;//����̳߳ش�С
	std::atomic_int curThreadSize_;//��ǰ�̳߳ش�С
	std::atomic_int idelThreadSize_;//�����߳�����

	//��ֹ�û�������ʱ����,������������̫�̣������̳߳��޷�������������ʹ��shared_ptr������������
	std::queue<std::shared_ptr<Task>> taskQue_;//�������
	std::atomic_uint taskSize_;//�����������
	size_t taskQueMaxThreshHold_;//������������ֵ

	std::mutex threadMtx_;//�̻߳�����
	std::mutex taskQueMtx_;//������л�����
	std::condition_variable notFull_; //δ����������
	std::condition_variable notEmpty_;//δ����������
	std::condition_variable exitCond_;//�ȴ��̳߳��߳���Դȫ�����յ���������

	PoolMode poolMode_;//�̳߳�ģʽ
	std::atomic_bool isPoolRunning_;//�̳߳��Ƿ�����
};




#endif
