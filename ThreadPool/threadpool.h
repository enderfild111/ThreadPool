#ifndef  THREADPOOL_H
#define  THREADPOOL_H
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

//�̳߳�ģʽ
enum class PoolMode {
	FIXED,
	CACHED,
};
//�߳���
class Thread {
public:
private:
};

class Task {
public:
	virtual void run() = 0;
};
//�̳߳���
class ThreadPool {
public:

private:
	std::vector<Thread*> threads_;//�̳߳�
	size_t initThreadPoolSize_;//��ʼ�̳߳ش�С
	
	std::queue<std::shared_ptr<Task>> taskQueue_;//�������
	size_t TaskQueMaxThreshHold_;//������������ֵ
	size_t TaskQueInitThreshHold_;//������г�ʼ��ֵ

};
#endif
