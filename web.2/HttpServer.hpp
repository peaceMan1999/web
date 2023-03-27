#pragma once

#include <iostream>
#include <pthread.h>
#include "TcpServer.hpp"
#include "Protocol.hpp"

#define PORT 8081

class HttpServer{
    private:
        int port;
        TcpServer *tcp_server;
        bool stop; // 表示服务是否一直在跑
    public:
        HttpServer(int _port = PORT)
            :port(_port)
            ,tcp_server(nullptr)
            ,stop(false)
        {}

        // 初始化服务器
        void InitServer()
        {
            tcp_server = TcpServer::getinstance(port);
        }

        void Loop(){
            int listen_sock = tcp_server->Sock();
            // 服务器不停
            while (!stop){
                struct sockaddr_in peer;
                socklen_t len = sizeof(peer);
                int sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
                if (sock < 0){
                    continue;
                }
                int *sk = new int(sock);
                pthread_t tid;
                pthread_create(&tid, nullptr, Entrance::Handler, sk);
                pthread_detach(tid);
            }
        }


        ~HttpServer(){}
    public:    
};

