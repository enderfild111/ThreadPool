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
	virtual void run() = 0;
};
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
	std::vector<Thread*> threads_;//�̳߳�
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
