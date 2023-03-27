#pragma once

#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include "Util.hpp"

using std::cout;
using std::endl;

// 入口类
class Entrance{
    public:
        static void* Handler(void *parm) // parameter
        {
            int sock = *(int*)parm;
            delete (int*)parm;

// #ifndef DEBUG
// #define DEBUG
//             char buffer[1024];
//             recv(sock, buffer, sizeof(buffer), 0);

//             std::cout << "---------------new msg ----------------" << std::endl;
//             std::cout << buffer << std::endl;
// #endif
            std::string line;
            int sz = Util::ReadLine(sock, line);
            cout << line << endl;

            close(sock);
            return nullptr;
        }
};