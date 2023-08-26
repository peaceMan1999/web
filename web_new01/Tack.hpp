#pragma once
#include <iostream>
#include "Protocol.hpp"

class Tack{
    private:
        int sock;
        CallBack handler;
    public:
        Tack(){}

        Tack(int _sock)
            :sock(_sock)
        {}
        // 处理任务
        void Process(){
            LOG(WARNING, "procss success");
            handler(sock);
        }

        ~Tack(){}
};
