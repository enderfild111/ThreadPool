#include <iostream>
#include"ThreadPool.h"
#include<thread>

/*
有些场景是希望获取到线程执行的返回值的

例子:
1+...+30000的和
thread1 计算1+...+10000的和
thread2 计算10001+...+20000的和
thread3 计算20001+...+30000的和
mainthread 等待所有线程执行完毕, 然后计算所有线程的结果的和
*/
class MyTask : public Task {
public:
   MyTask(int begin, int end) 
   : begin_(begin) 
   , end_(end)
   {}
   //问题1: 怎么设计run的返回值，可以让其表示任意的类型?
   //java python 中都有obeject作为所有类的顶尖父类，可以作为返回值类型
   //C++17 中引用了Any类型，可以作为任意类型
   Any run() {
      std::cout << "tid:" << std::this_thread::get_id() << "runing..." << std::endl;
      //std::this_thread::sleep_for(std::chrono::seconds(5));
      int sum = 0;
      for (int i = begin_; i <= end_; ++i) {
         sum += i;
      }
      std::cout << "tid:" << std::this_thread::get_id() << "finished." << std::endl;
      return sum;
   }
private:
   int begin_;
   int end_;
};
int main()
{
   ThreadPool pool;
   pool.start();
   //如何设计这里的Result类型?
   Result res = pool.submitTask(std::make_shared<MyTask>());
   //如果线程没有执行完毕，get方法是要阻塞起来的
   //get返回了一个Any类型，如何转换为具体类型?
   res.get().cast_<int>();
   pool.submitTask(std::make_shared<MyTask>());
   pool.submitTask(std::make_shared<MyTask>());
   pool.submitTask(std::make_shared<MyTask>());
   pool.submitTask(std::make_shared<MyTask>());
   pool.submitTask(std::make_shared<MyTask>());
   getchar();
   return 0;
}

