#pragma once

#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sstream>
#include <unordered_map>
#include "Util.hpp"
#include "Log.hpp"

using std::cout;
using std::endl;
using std::string;

#define SEQ ": "

// 接收请求
class HttpRequest{
    public:
        string requset_line; // 请求行
        std::vector<string> request_header; // 请求属性
        string blank_line;  // 空行
        string request_body; //正文

        // 解析的正文
        string method;
        string uri;
        string version;

        // 哈希表
        std::unordered_map<string, string> header_kv;
};

// 响应
class HttpResponse{
    public:
        string response_line; // 请求行
        std::vector<string> response_header; // 请求属性
        string blank_line;  // 空行
        string response_body; //正文
};

// 对端
class EndPoint{
    private:
        int sock;
        HttpRequest http_request;   // 接收类
        HttpResponse http_response; // 响应类
    private:
        void RecvHttpRequestLine()
        {
            auto& line = http_request.requset_line;
            Util::ReadLine(sock, line);
            line.resize(line.size()-1); // 仅仅是为了输出美观，去掉'\n'
            LOG(INFO, http_request.requset_line);
        }
        void ParseHttpRequestLine(){
            auto& line = http_request.requset_line;
            std::stringstream ss(line);
            ss >> http_request.method >> http_request.uri >> http_request.version;
            // LOG(INFO, http_request.method);
            // LOG(INFO, http_request.uri);
            // LOG(INFO, http_request.version);
        }
        void ParseHttpRequestHeader(){
            string key;
            string value;
            for (auto& it : http_request.request_header){
                if (Util::CultString(it, key, value, SEQ)){
                    http_request.header_kv.insert({key, value});
                    cout << "DEBUG : " << key << " -- " << value << endl;
                }else{
                    LOG(FATAL, "request header error");
                }
            }
        }
        void RecvHttpRequestHeader()
        {
            string line;
            // 读取报头
            while (true){
                line.clear();
                Util::ReadLine(sock, line);
                if (line == "\n"){
                    http_request.blank_line = '\n';
                    break;
                }
                line.resize(line.size()-1);
                http_request.request_header.push_back(line);
                LOG(INFO, line);
            }
        }
    public:
        EndPoint(int _sock)
            :sock(_sock) 
        {}
        // 读取
        void RecvHttpRequest(){
            RecvHttpRequestLine();
            RecvHttpRequestHeader();
        }

        // 分析
        void ParseHttpRequest(){
            ParseHttpRequestLine();
            ParseHttpRequestHeader();
        }

        // 构建响应
        void BuildHttpResponse(){

        }

        // 发送响应
        void SendHttpResponse(){

        }

        ~EndPoint()
        {
            close(sock);
        }
};

// 入口类
class Entrance{
    public:
        static void* Handler(void *parm) // parameter
        {
            LOG(INFO, "########## get handler, request begin ... ##########");
            int sock = *(int*)parm;
            delete (int*)parm;

#ifdef DEBUG
            char buffer[1024];
            recv(sock, buffer, sizeof(buffer), 0);

            std::cout << "---------------new msg ----------------" << std::endl;
            std::cout << buffer << std::endl;
#else
            EndPoint *ep = new EndPoint(sock);  
            ep->RecvHttpRequest();
            ep->ParseHttpRequest();
            ep->BuildHttpResponse();
            ep->SendHttpResponse();
            delete(ep);

#endif
            LOG(INFO, "########## handler request end ... ##########");
            return nullptr;
        }
};