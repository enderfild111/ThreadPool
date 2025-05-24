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



//Any���ͣ����Խ����������͵���������
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
		else {
			throw "type is incompatiable!"//�׳��쳣
		}
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



//�̳߳�ģʽ
enum class PoolMode {
	MODE_FIXED,
	MODE_CACHED,
};
//�߳���
class Thread {
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void()>;
	//����
	Thread(ThreadFunc func);
	//����
	~Thread();
	void start();
private:
	ThreadFunc func_;
};
//�û����Լ̳�Task��ʵ���Լ�������
//��дrun����
class Task {
public:
	virtual Any run() = 0;
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
	void start(size_t initThreadPoolSize = 4);
	//�����̳߳�ģʽ
	void setMode(PoolMode mode);
	//�����̳߳�������������ֵ
	void setTaskQueMaxThreshHold(size_t maxThreshHold);
	//���̳߳��ύ����
	void submitTask(std::shared_ptr<Task>sp);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	void ThreadFunc();
private:
	std::vector<std::unique_ptr<Thread>> threads_;//�̳߳�
	size_t initThreadSize_;//��ʼ�̳߳ش�С
	//��ֹ�û�������ʱ����,������������̫�̣������̳߳��޷�������������ʹ��shared_ptr������������
	std::queue<std::shared_ptr<Task>> taskQue_;//�������
	std::atomic_uint taskSize_;//�����������
	size_t taskQueMaxThreshHold_;//������������ֵ


	std::mutex taskQueMtx_;//������л�����
	std::condition_variable notFull_; //δ����������
	std::condition_variable notEmpty_;//δ����������

	PoolMode poolMode_;//�̳߳�ģʽ
};




#endif
