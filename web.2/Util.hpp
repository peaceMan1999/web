#pragma once

#include <iostream>
#include <sys/socket.h>

class Util{
    public:
        // 对'\n','\r','\r\n'进行区分
        static int ReadLine(int sock, std::string& out)
        {
            char ch = 'a';
            while(ch != '\n'){
                ssize_t s = recv(sock, &ch, 1, 0);
                if (s > 0){
                    // 判断是否是'\r'
                    if (ch == '\r'){
                        // 窥探
                        recv(sock, &ch, 1, MSG_PEEK);
                        if (ch == '\n'){
                            // 直接向后读
                            recv(sock, &ch, 1, 0);
                        }else{
                            ch = '\n';
                        }
                    }
                    out.push_back(ch);
                }else if (s == 0){
                    return 0;
                }else{
                    // err
                    return -1;
                }
            }
            return out.size();
        }
};


