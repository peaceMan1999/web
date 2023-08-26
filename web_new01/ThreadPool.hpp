#pragma once
#include <iostream>
#include <queue>
#include <pthread.h>
#include "Tack.hpp"
#include "Log.hpp"

#define NUM 5

// 单例
class ThreadPool{
    private:
        int num;
        bool stop;
        std::queue<Tack> tack_queue;
        pthread_mutex_t mutex;  // 锁
        pthread_cond_t cond;    // 条件变量

        static ThreadPool* Thread_instance; // 获取单例对象

    private:
        ThreadPool(int _num = NUM)
            :num(_num)
            ,stop(false)
        {
            pthread_mutex_init(&mutex, nullptr);
            pthread_cond_init(&cond, nullptr);
        }

        ThreadPool(const ThreadPool& tp) = delete;
    public:
        static ThreadPool* getinstance(){
            static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
            if (Thread_instance == nullptr){
                pthread_mutex_lock(&lock);
                if (Thread_instance == nullptr){
                    Thread_instance = new ThreadPool();
                    Thread_instance->InitThreadPool();
                }
                pthread_mutex_unlock(&lock);
            }
            return Thread_instance;
        }
        
        // 加锁
        void Lock(){
            pthread_mutex_lock(&mutex);
        }
        // 解锁
        void UnLock(){
            pthread_mutex_unlock(&mutex);
        }
        // 判断停止
        bool IsStop(){
            return stop;
        }

        bool IsQueueEmpty(){
            return tack_queue.size() == 0 ? true : false;
        }

        // 执行任务 -- 必须是静态方法，因为成员函数默认包含了this指针的，所以args就是this
        static void *ThreadRoutines(void *args){
            ThreadPool *tp = (ThreadPool*)args;

            // 执行任务
            while (true){
                Tack tack; // 空任务，获取任务需要加锁
                // LOG(WARNING, " In");
                tp->Lock();
                while (tp->IsQueueEmpty()){
                    // LOG(WARNING, " W");
                    // 说明没了，为防止虚假唤醒，所以要用while不用if，这样持有锁的醒来后再判断一次
                    tp->ThreadWait();
                }
                // LOG(WARNING, " pop");
                tp->PopTack(tack); 
                

                tp->UnLock();
                // LOG(WARNING, " Process");

                tack.Process();
            }
        }

        // 初始化线程池      
        bool InitThreadPool(){
            for (int i = 0; i < num; ++i){
                pthread_t tid;
                
                // 可能会创建失败
                if (pthread_create(&tid, nullptr, ThreadRoutines, this) != 0){
                    LOG(FATAL, "create thread pool error!");
                    return false;
                }
            }
            LOG(INFO, "create thread pool success!");
            
            return true;
        }


        
        // 添加线程
        void PushTack(const Tack& tack){
            // 临界资源加锁
            Lock();
            tack_queue.push(tack);
            UnLock();
            // 唤醒
            ThreadWakeup();
        }

        // 添加任务后弹出
        void PopTack(Tack& tack){
            tack = tack_queue.front();
            tack_queue.pop();
        }
        
        // 线程等待
        void ThreadWait(){
            // 让锁和条件变量进行等待,休眠释放锁，醒来获取锁
            pthread_cond_wait(&cond, &mutex);
        }

        // 唤醒线程
        void ThreadWakeup(){
            pthread_cond_signal(&cond);
        }

        ~ThreadPool(){
            pthread_mutex_destroy(&mutex);
            pthread_cond_destroy(&cond);
        }
};

// 静态成员类外初始化
ThreadPool* ThreadPool::Thread_instance = nullptr;

