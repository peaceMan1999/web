#pragma once

#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "TcpServer.hpp"
#include "Protocol.hpp"
#include "ThreadPool.hpp"
#include "Tack.hpp"

#define PORT 8081

class HttpServer{
    private:
        int port;
        // TcpServer *tcp_server;
        // ThreadPool* thread_pool;
        bool stop; // 表示服务是否一直在跑
    public:
        HttpServer(int _port = PORT)
            :port(_port)
            // ,tcp_server(nullptr)
            ,stop(false)
        {
            // thread_pool = ThreadPool::getinstance();
        }

        // 初始化服务器
        void InitServer()
        {
            // 如果不忽略SIGPIPE信号，可能会导致读写时管道挂掉，以至于崩溃,服务器挂了
            // 在收到RST之后, 会生成SIGPIPE信号, 导致进程退出.
            signal(SIGPIPE, SIG_IGN);
            // tcp_server = TcpServer::getinstance(port);
        }

        void Loop(){
            LOG(INFO, "Loop begin ...");
            TcpServer *tcp_srv = TcpServer::getinstance(port); // listen_sock
            // 服务器不停
            while (!stop){
                struct sockaddr_in peer;
                socklen_t len = sizeof(peer);
                int sock = accept(tcp_srv->Sock(), (struct sockaddr*)&peer, &len);
                if (sock < 0){
                    continue;
                }
                LOG(INFO, "get a new link ... ");
                Tack tack(sock);
                ThreadPool *thr_pool = ThreadPool::getinstance();
                thr_pool->PushTack(tack); 
                // LOG(WARNING, "-----------------------????push over-------");

                // int *sk = new int(sock);
                // pthread_t tid;
                // pthread_create(&tid, nullptr, Entrance::Handler, sk);
                // pthread_detach(tid);
            }
        }


        ~HttpServer(){}
    public:    
};

